/*
 * forge — convention-based build tool for C and C++ projects
 * Compile: cc -std=c11 -O2 -o forge forge.c
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <ctype.h>

/* ── error helpers ─────────────────────────────────────────────────────── */

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "forge: error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

/* ── string helpers ─────────────────────────────────────────────────────── */

static char *xstrdup(const char *s) {
    char *d = strdup(s);
    if (!d) die("out of memory");
    return d;
}

static char *xstrndup(const char *s, size_t n) {
    char *d = strndup(s, n);
    if (!d) die("out of memory");
    return d;
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)*(e-1))) e--;
    *e = '\0';
    return s;
}

/* strip surrounding quotes from a TOML string value */
static char *unquote(const char *s) {
    size_t len = strlen(s);
    if (len >= 2 && s[0] == '"' && s[len-1] == '"')
        return xstrndup(s+1, len-2);
    return xstrdup(s);
}

/* ── dynamic string array ───────────────────────────────────────────────── */

typedef struct {
    char **items;
    size_t count;
    size_t cap;
} StrArr;

static void sarr_push(StrArr *a, const char *s) {
    if (a->count == a->cap) {
        a->cap = a->cap ? a->cap * 2 : 8;
        a->items = realloc(a->items, a->cap * sizeof(char*));
        if (!a->items) die("out of memory");
    }
    a->items[a->count++] = xstrdup(s);
}

static void sarr_free(StrArr *a) {
    for (size_t i = 0; i < a->count; i++) free(a->items[i]);
    free(a->items);
    a->items = NULL;
    a->count = a->cap = 0;
}

/* ── project model ──────────────────────────────────────────────────────── */

typedef struct {
    char *name;
    char *src;       /* NULL = grouping lib */
    StrArr deps;
} Lib;

typedef struct {
    char *name;
    char *compiler;
    char *std;
    char *python;
    int   werror;    /* 1 = true, 0 = false */

    Lib  *libs;
    size_t nlibs;
    size_t libcap;

    Lib  *exes;
    size_t nexes;
    size_t execap;
} Project;

static void project_init(Project *p) {
    memset(p, 0, sizeof(*p));
    p->name     = xstrdup("project");
    p->compiler = xstrdup("cc");
    p->std      = xstrdup("c11");
    p->python   = xstrdup("python3");
    p->werror   = 1;
}

static Lib *project_add_lib(Project *p) {
    if (p->nlibs == p->libcap) {
        p->libcap = p->libcap ? p->libcap * 2 : 4;
        p->libs = realloc(p->libs, p->libcap * sizeof(Lib));
        if (!p->libs) die("out of memory");
    }
    Lib *l = &p->libs[p->nlibs++];
    memset(l, 0, sizeof(*l));
    return l;
}

static Lib *project_add_exe(Project *p) {
    if (p->nexes == p->execap) {
        p->execap = p->execap ? p->execap * 2 : 4;
        p->exes = realloc(p->exes, p->execap * sizeof(Lib));
        if (!p->exes) die("out of memory");
    }
    Lib *e = &p->exes[p->nexes++];
    memset(e, 0, sizeof(*e));
    return e;
}

/* ── TOML parser (subset: top-level scalars + [[lib]] arrays) ─────────── */

static void parse_toml(const char *path, Project *p) {
    FILE *f = fopen(path, "r");
    if (!f) die("cannot open %s: %s", path, strerror(errno));

    char line[4096];
    Lib *cur_lib = NULL;
    Lib *cur_exe = NULL;

    while (fgets(line, sizeof(line), f)) {
        char *s = trim(line);

        /* skip blank lines and comments */
        if (*s == '\0' || *s == '#') continue;

        /* [[lib]] section header */
        if (strcmp(s, "[[lib]]") == 0) {
            cur_lib = project_add_lib(p);
            cur_exe = NULL;
            continue;
        }

        /* [[exe]] section header */
        if (strcmp(s, "[[exe]]") == 0) {
            cur_exe = project_add_exe(p);
            cur_lib = NULL;
            continue;
        }

        /* key = value */
        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);

        Lib *cur = cur_lib ? cur_lib : cur_exe;
        if (cur) {
            /* inside a [[lib]] or [[exe]] entry */
            if (strcmp(key, "name") == 0) {
                cur->name = unquote(val);
            } else if (strcmp(key, "src") == 0) {
                cur->src = unquote(val);
            } else if (strcmp(key, "deps") == 0) {
                /* parse ["a", "b", ...] */
                char *p2 = val;
                while (*p2 && *p2 != '[') p2++;
                if (*p2 == '[') p2++;
                while (*p2) {
                    while (isspace((unsigned char)*p2) || *p2 == ',') p2++;
                    if (*p2 == ']' || *p2 == '\0') break;
                    if (*p2 == '"') {
                        p2++;
                        char *end = strchr(p2, '"');
                        if (end) {
                            char tmp[256];
                            size_t n = (size_t)(end - p2);
                            if (n >= sizeof(tmp)) n = sizeof(tmp)-1;
                            memcpy(tmp, p2, n);
                            tmp[n] = '\0';
                            sarr_push(&cur->deps, tmp);
                            p2 = end + 1;
                        }
                    } else {
                        p2++;
                    }
                }
            }
        } else {
            /* top-level fields */
            if (strcmp(key, "name") == 0) {
                free(p->name); p->name = unquote(val);
            } else if (strcmp(key, "compiler") == 0) {
                free(p->compiler); p->compiler = unquote(val);
            } else if (strcmp(key, "std") == 0) {
                free(p->std); p->std = unquote(val);
            } else if (strcmp(key, "python") == 0) {
                free(p->python); p->python = unquote(val);
            } else if (strcmp(key, "werror") == 0) {
                char *v = unquote(val);
                p->werror = (strcmp(v, "true") == 0) ? 1 : 0;
                free(v);
            }
        }
    }
    fclose(f);
}

static void validate_project(Project *p) {
    /* every lib must have a name */
    for (size_t i = 0; i < p->nlibs; i++) {
        if (!p->libs[i].name || p->libs[i].name[0] == '\0')
            die("[[lib]] entry #%zu has no 'name' field", i+1);
    }

    /* every dep must refer to a declared lib */
    for (size_t i = 0; i < p->nlibs; i++) {
        Lib *l = &p->libs[i];
        for (size_t d = 0; d < l->deps.count; d++) {
            const char *dep = l->deps.items[d];
            int found = 0;
            for (size_t j = 0; j < p->nlibs; j++) {
                if (strcmp(p->libs[j].name, dep) == 0) { found = 1; break; }
            }
            if (!found)
                die("lib '%s' depends on '%s' which is not declared", l->name, dep);
        }
    }

    /* every [[exe]] must have a name, and its deps must refer to declared libs */
    for (size_t i = 0; i < p->nexes; i++) {
        if (!p->exes[i].name || p->exes[i].name[0] == '\0')
            die("[[exe]] entry #%zu has no 'name' field", i+1);
        Lib *e = &p->exes[i];
        for (size_t d = 0; d < e->deps.count; d++) {
            const char *dep = e->deps.items[d];
            int found = 0;
            for (size_t j = 0; j < p->nlibs; j++) {
                if (strcmp(p->libs[j].name, dep) == 0) { found = 1; break; }
            }
            if (!found)
                die("exe '%s' depends on '%s' which is not declared", e->name, dep);
        }
    }
}

/* ── topological sort (Kahn's algorithm) ───────────────────────────────── */

static size_t *topo_sort(Project *p) {
    size_t n = p->nlibs;
    size_t *order = malloc(n * sizeof(size_t));
    int    *indeg  = calloc(n, sizeof(int));
    if (!order || !indeg) die("out of memory");

    /* build in-degree */
    for (size_t i = 0; i < n; i++) {
        for (size_t d = 0; d < p->libs[i].deps.count; d++) {
            const char *dep = p->libs[i].deps.items[d];
            for (size_t j = 0; j < n; j++) {
                if (strcmp(p->libs[j].name, dep) == 0) {
                    indeg[i]++;
                    break;
                }
            }
        }
    }

    size_t *queue = malloc(n * sizeof(size_t));
    if (!queue) die("out of memory");
    size_t qhead = 0, qtail = 0, odx = 0;

    for (size_t i = 0; i < n; i++)
        if (indeg[i] == 0) queue[qtail++] = i;

    while (qhead < qtail) {
        size_t cur = queue[qhead++];
        order[odx++] = cur;
        Lib *l = &p->libs[cur];
        /* find libs that depend on cur and decrement their in-degree */
        for (size_t i = 0; i < n; i++) {
            for (size_t d = 0; d < p->libs[i].deps.count; d++) {
                if (strcmp(p->libs[i].deps.items[d], l->name) == 0) {
                    if (--indeg[i] == 0) queue[qtail++] = i;
                }
            }
        }
    }

    free(queue);
    free(indeg);

    if (odx != n)
        die("dependency cycle detected among [[lib]] entries");

    return order;
}

/* ── filesystem helpers ─────────────────────────────────────────────────── */

static int dir_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/* join two path components */
static char *path_join(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *out = malloc(la + 1 + lb + 1);
    if (!out) die("out of memory");
    memcpy(out, a, la);
    out[la] = '/';
    memcpy(out+la+1, b, lb);
    out[la+1+lb] = '\0';
    return out;
}

/* ── clean command ──────────────────────────────────────────────────────── */

/* recursive remove: files first, then dirs (post-order) */
static void rmrf(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (d) {
            struct dirent *e;
            while ((e = readdir(d)) != NULL) {
                if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                    continue;
                char *child = path_join(path, e->d_name);
                rmrf(child);
                free(child);
            }
            closedir(d);
        }
        rmdir(path);
    } else {
        remove(path);
    }
}

/* walk tree and remove *.so files inside any tests/ directory */
static void remove_test_so(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return;
        const char *base = strrchr(path, '/');
        base = base ? base+1 : path;
        int in_tests = strcmp(base, "tests") == 0;
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            char *child = path_join(path, e->d_name);
            if (in_tests) {
                /* check if it's a .so file */
                const char *dot = strrchr(e->d_name, '.');
                if (dot && strcmp(dot, ".so") == 0) {
                    remove(child);
                } else {
                    /* could be .cpython-312-x86_64-linux-gnu.so etc. */
                    if (strstr(e->d_name, ".so")) {
                        remove(child);
                    }
                }
            } else {
                remove_test_so(child);
            }
            free(child);
        }
        closedir(d);
    }
}

static void cmd_clean(void) {
    rmrf(".build");
    remove_test_so(".");
    /* idempotent: ignore errors */
}

/* ── build helpers: mkdir -p ────────────────────────────────────────────── */

static void copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) die("cannot open %s: %s", src, strerror(errno));
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); die("cannot open %s: %s", dst, strerror(errno)); }
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in); fclose(out);
            die("write error copying to %s", dst);
        }
    }
    fclose(in);
    fclose(out);
}

static void mkdirp(const char *path) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp+1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

/* ── compiler family detection ──────────────────────────────────────────── */

typedef enum { CC_GCC, CC_CLANG, CC_OTHER } CCFamily;

static CCFamily compiler_family(const char *cc) {
    const char *b = strrchr(cc, '/');
    b = b ? b+1 : cc;
    if (strstr(b, "clang")) return CC_CLANG;
    if (strstr(b, "gcc") || strstr(b, "g++") || strcmp(b, "cc") == 0)
        return CC_GCC;
    return CC_OTHER;
}

/* ── warning flags ──────────────────────────────────────────────────────── */

static const char *WARN_COMMON =
    "-Wall -Wextra -Wpedantic "
    "-Wconversion -Wsign-conversion -Wshadow "
    "-Wformat=2 -Wformat-security "
    "-Wnull-dereference -Wdouble-promotion "
    "-Wcast-align -Wcast-qual "
    "-Wimplicit-fallthrough "
    "-Wswitch-enum -Wswitch-default "
    "-Wmissing-declarations -Wmissing-noreturn "
    "-Wvla -Wstrict-overflow=2 -Wfloat-equal -Wundef "
    "-Wpointer-arith -Wwrite-strings "
    "-fstack-protector-strong -O3";

static const char *WARN_GCC =
    "-Wduplicated-cond -Wduplicated-branches "
    "-Wlogical-op -Wformat-signedness "
    "-Warray-bounds=2 -Wuseless-cast";

static const char *WARN_CLANG =
    "-Wunreachable-code -Wunreachable-code-break "
    "-Wunreachable-code-return -Wloop-analysis "
    "-Wconditional-uninitialized -Wcomma "
    "-Wover-aligned -Warray-bounds";

static void append_warn_flags(char *buf, size_t bufsz, CCFamily fam) {
    strncat(buf, WARN_COMMON, bufsz - strlen(buf) - 1);
    if (fam == CC_GCC) {
        strncat(buf, " ", bufsz - strlen(buf) - 1);
        strncat(buf, WARN_GCC, bufsz - strlen(buf) - 1);
    } else if (fam == CC_CLANG) {
        strncat(buf, " ", bufsz - strlen(buf) - 1);
        strncat(buf, WARN_CLANG, bufsz - strlen(buf) - 1);
    }
}

/* ── object file name from source path ─────────────────────────────────── */
/* replace '/' and '.' with '_', append .o */

static char *obj_name(const char *src) {
    char buf[PATH_MAX];
    size_t i = 0;
    for (const char *p = src; *p && i < sizeof(buf)-3; p++) {
        if (*p == '/' || *p == '.') buf[i++] = '_';
        else buf[i++] = *p;
    }
    buf[i++] = '.'; buf[i++] = 'o'; buf[i] = '\0';
    return xstrdup(buf);
}

/* ── collect source files ───────────────────────────────────────────────── */

static int is_src_ext(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return strcmp(dot, ".c")   == 0 ||
           strcmp(dot, ".cpp") == 0 ||
           strcmp(dot, ".cc")  == 0 ||
           strcmp(dot, ".cxx") == 0;
}

/* recursive directory walk collecting source files */
static void collect_sources(const char *dir, StrArr *out) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char *full = path_join(dir, e->d_name);
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                collect_sources(full, out);
            } else if (S_ISREG(st.st_mode) && is_src_ext(e->d_name)) {
                sarr_push(out, full);
            }
        }
        free(full);
    }
    closedir(d);
}

/* ── run a shell command, die on failure ────────────────────────────────── */

static void run(const char *cmd) {
    int ret = system(cmd);
    if (ret != 0)
        die("command failed (exit %d):\n  %s", ret, cmd);
}

/* ── compile one source file to object ─────────────────────────────────── */

static void compile_source(Project *p, CCFamily fam,
                            const char *src, const char *obj,
                            const char *extra_flags,
                            int suppress_werror,
                            const char *ext_inc_flags) {
    char warn[4096] = {0};
    append_warn_flags(warn, sizeof(warn), fam);

    /* derive dep file name from obj basename */
    const char *obj_base = strrchr(obj, '/');
    obj_base = obj_base ? obj_base + 1 : obj;
    char dep_path[PATH_MAX];
    snprintf(dep_path, sizeof(dep_path), ".build/deps/%s.d", obj_base);

    const char *inc_flag = dir_exists("include") ? "-I include " : "";

    char cmd[16384];
    /* FIXED: Added explicit spaces around the injected flags to prevent merging */
    int n = snprintf(cmd, sizeof(cmd),
        "%s -std=%s -O2 %s %s %s %s "  /* Added space after last %s */
        "-MMD -MF %s "
        "-c %s -o %s",
        p->compiler, p->std,
        warn,
        (p->werror && !suppress_werror) ? "-Werror" : "",
        inc_flag,
        ext_inc_flags ? ext_inc_flags : "",
        dep_path,
        src, obj);

    if (extra_flags && extra_flags[0])
        snprintf(cmd + n, sizeof(cmd) - (size_t)n, " %s", extra_flags);

    fprintf(stdout, "  cc %s\n", src);
    int ret = system(cmd);
    if (ret != 0)
        die("compilation failed for %s (exit %d)", src, ret);
}/* ── lib root from src dir ─────────────────────────────────────────────── */

static char *lib_root(const char *src) {
    /* strip last path component */
    char *s = xstrdup(src);
    char *slash = strrchr(s, '/');
    if (slash) *slash = '\0';
    else { free(s); return xstrdup("."); }
    return s;
}

/* ── infer src dir for a lib ────────────────────────────────────────────── */

static char *lib_src_dir(Project *p, Lib *l) {
    if (l->src) return xstrdup(l->src);
    /* default: libs/NAME/src */
    char *d = malloc(strlen("libs/") + strlen(l->name) + strlen("/src") + 1);
    sprintf(d, "libs/%s/src", l->name);
    return d;
}

/* ── infer src dir for an exe ───────────────────────────────────────────── */

static char *exe_src_dir(Lib *e) {
    if (e->src) return xstrdup(e->src);
    /* default: apps/NAME/src */
    char *d = malloc(strlen("apps/") + strlen(e->name) + strlen("/src") + 1);
    sprintf(d, "apps/%s/src", e->name);
    return d;
}

/* ── transitive dep closure (returns indices into p->libs, in reverse topo) */

static void dep_closure(Project *p, size_t idx, int *visited, StrArr *libs_out) {
    if (visited[idx]) return;
    visited[idx] = 1;
    Lib *l = &p->libs[idx];
    for (size_t d = 0; d < l->deps.count; d++) {
        for (size_t j = 0; j < p->nlibs; j++) {
            if (strcmp(p->libs[j].name, l->deps.items[d]) == 0) {
                dep_closure(p, j, visited, libs_out);
            }
        }
    }
    /* add self after deps (reverse topo: deepest dep last) */
    sarr_push(libs_out, l->name);
}

/* ── build one library ──────────────────────────────────────────────────── */

static void build_lib(Project *p, CCFamily fam, size_t idx, int single_lib, int pic) {
    Lib *l = &p->libs[idx];
    char *srcdir = lib_src_dir(p, l);

    if (!dir_exists(srcdir)) {
        free(srcdir);
        return;
    }

    mkdirp(".build/obj");
    mkdirp(".build/deps");
    mkdirp(".build/lib");

    StrArr sources = {0};
    collect_sources(srcdir, &sources);

    StrArr objects = {0};
    for (size_t i = 0; i < sources.count; i++) {
        char *oname = obj_name(sources.items[i]);
        char *opath = path_join(".build/obj", oname);
        free(oname);

        /* Pass NULL for external includes to keep strict warnings on your code */
        compile_source(p, fam, sources.items[i], opath, pic ? "-fPIC" : NULL, 0, NULL);
        sarr_push(&objects, opath);
        free(opath);
    }

    if (objects.count == 0) {
        sarr_free(&sources);
        sarr_free(&objects);
        free(srcdir);
        return;
    }

    int has_main = 0;
    if (single_lib) {
        const char *mains[] = {"src/main.c","src/main.cpp","src/main.cc", NULL};
        for (int mi = 0; mains[mi]; mi++) {
            if (file_exists(mains[mi])) { has_main = 1; break; }
        }
    }

    char ar_cmd[16384];
    char lib_path[PATH_MAX];

    if (single_lib && has_main) {
        mkdirp(".build/bin");
        snprintf(lib_path, sizeof(lib_path), ".build/bin/%s", p->name);
        char *obj_list = xstrdup("");
        for (size_t i = 0; i < objects.count; i++) {
            size_t cur = strlen(obj_list);
            obj_list = realloc(obj_list, cur + strlen(objects.items[i]) + 2);
            strcat(obj_list, " ");
            strcat(obj_list, objects.items[i]);
        }
        snprintf(ar_cmd, sizeof(ar_cmd), "%s%s -o %s",
                 p->compiler, obj_list, lib_path);
        fprintf(stdout, "  link %s\n", lib_path);
        run(ar_cmd);
        free(obj_list);
    } else {
        snprintf(lib_path, sizeof(lib_path), ".build/lib/lib%s.a", l->name);
        snprintf(ar_cmd, sizeof(ar_cmd), "ar rcs %s", lib_path);
        for (size_t i = 0; i < objects.count; i++) {
            strncat(ar_cmd, " ", sizeof(ar_cmd)-strlen(ar_cmd)-1);
            strncat(ar_cmd, objects.items[i], sizeof(ar_cmd)-strlen(ar_cmd)-1);
        }
        fprintf(stdout, "  ar %s\n", lib_path);
        run(ar_cmd);
    }

    sarr_free(&sources);
    sarr_free(&objects);
    free(srcdir);
}

/* ── build one executable ───────────────────────────────────────────────── */

static void build_exe(Project *p, CCFamily fam, size_t idx) {
    Lib *e = &p->exes[idx];
    char *srcdir = exe_src_dir(e);

    if (!dir_exists(srcdir)) {
        free(srcdir);
        return;
    }

    mkdirp(".build/obj");
    mkdirp(".build/deps");
    mkdirp(".build/bin");

    StrArr sources = {0};
    collect_sources(srcdir, &sources);

    StrArr objects = {0};
    for (size_t i = 0; i < sources.count; i++) {
        char *oname = obj_name(sources.items[i]);
        char *opath = path_join(".build/obj", oname);
        free(oname);
        /* Strict warnings for app sources */
        compile_source(p, fam, sources.items[i], opath, NULL, 0, NULL);
        sarr_push(&objects, opath);
        free(opath);
    }

    if (objects.count == 0) {
        sarr_free(&sources);
        sarr_free(&objects);
        free(srcdir);
        return;
    }

    int *visited = calloc(p->nlibs, sizeof(int));
    if (!visited) die("out of memory");
    StrArr link_libs = {0};
    for (size_t d = 0; d < e->deps.count; d++) {
        for (size_t j = 0; j < p->nlibs; j++) {
            if (strcmp(p->libs[j].name, e->deps.items[d]) == 0) {
                dep_closure(p, j, visited, &link_libs);
                break;
            }
        }
    }
    free(visited);

    char link_args[8192] = {0};
    snprintf(link_args, sizeof(link_args), "-L.build/lib");
    for (size_t i = link_libs.count; i-- > 0;) {
        char apath[PATH_MAX];
        snprintf(apath, sizeof(apath), ".build/lib/lib%s.a", link_libs.items[i]);
        if (file_exists(apath)) {
            char tmp[256];
            snprintf(tmp, sizeof(tmp), " -l%s", link_libs.items[i]);
            strncat(link_args, tmp, sizeof(link_args)-strlen(link_args)-1);
        }
    }
    sarr_free(&link_libs);

    char obj_list[8192] = {0};
    for (size_t i = 0; i < objects.count; i++) {
        strncat(obj_list, " ", sizeof(obj_list)-strlen(obj_list)-1);
        strncat(obj_list, objects.items[i], sizeof(obj_list)-strlen(obj_list)-1);
    }

    char bin_path[PATH_MAX];
    snprintf(bin_path, sizeof(bin_path), ".build/bin/%s", e->name);

    char link_cmd[16384];
    snprintf(link_cmd, sizeof(link_cmd), "%s%s %s -o %s",
             p->compiler, obj_list, link_args, bin_path);

    fprintf(stdout, "  link %s\n", bin_path);
    run(link_cmd);

    sarr_free(&sources);
    sarr_free(&objects);
    free(srcdir);
}

/* ── binding type detection ─────────────────────────────────────────────── */

typedef enum { BIND_PYBIND11, BIND_CFFI, BIND_CTYPES } BindType;

static BindType detect_binding(const char *binddir, char *module_name, size_t mnsz) {
    DIR *d = opendir(binddir);
    if (!d) return BIND_CTYPES;

    BindType bt = BIND_CTYPES;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!is_src_ext(e->d_name)) continue;
        char *path = path_join(binddir, e->d_name);
        FILE *f = fopen(path, "r");
        if (!f) { free(path); continue; }
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "PYBIND11_MODULE")) {
                bt = BIND_PYBIND11;
                /* extract module name: PYBIND11_MODULE(name, ...) */
                char *lp = strstr(line, "PYBIND11_MODULE");
                lp += strlen("PYBIND11_MODULE");
                while (*lp && (*lp == ' ' || *lp == '(')) lp++;
                char *end = lp;
                while (*end && *end != ',' && *end != ')' && !isspace((unsigned char)*end)) end++;
                size_t n = (size_t)(end - lp);
                if (n >= mnsz) n = mnsz-1;
                memcpy(module_name, lp, n);
                module_name[n] = '\0';
            } else if (strstr(line, "ffi.cdef") || strstr(line, "cffi")) {
                if (bt != BIND_PYBIND11) bt = BIND_CFFI;
            }
        }
        fclose(f);
        free(path);
        if (bt == BIND_PYBIND11) break;
    }
    closedir(d);
    return bt;
}

/* ── get Python EXT_SUFFIX ──────────────────────────────────────────────── */

static char *get_ext_suffix(const char *python) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "%s -c \"import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'), end='')\"",
        python);
    FILE *fp = popen(cmd, "r");
    if (!fp) die("cannot run python to get EXT_SUFFIX");
    char buf[256];
    if (!fgets(buf, sizeof(buf), fp)) {
        pclose(fp);
        die("python did not return EXT_SUFFIX");
    }
    pclose(fp);
    return xstrdup(trim(buf));
}

/* ── get pybind11 include path ──────────────────────────────────────────── */

static char *get_pybind11_includes(const char *python) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "%s -c \"import pybind11; print(pybind11.get_include(), end='')\" 2>/dev/null",
        python);
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    char buf[1024];
    char *result = NULL;
    if (fgets(buf, sizeof(buf), fp) && buf[0]) {
        result = xstrdup(trim(buf));
    }
    pclose(fp);
    return result;
}

/* ── get Python include path ────────────────────────────────────────────── */

static char *get_python_includes(const char *python) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "%s -c \"import sysconfig; print(sysconfig.get_path('include'), end='')\"",
        python);
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    char buf[1024];
    char *result = NULL;
    if (fgets(buf, sizeof(buf), fp) && buf[0]) {
        result = xstrdup(trim(buf));
    }
    pclose(fp);
    return result;
}

/* ── get Python LDFLAGS ─────────────────────────────────────────────────── */

static char *get_python_ldflags(const char *python) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "%s -c \""
        "import sysconfig; cfg=sysconfig.get_config_vars();"
        "print(cfg.get('BLDLIBRARY','') + ' ' + cfg.get('LIBS','') + ' ' + cfg.get('SYSLIBS',''), end='')"
        "\"",
        python);
    FILE *fp = popen(cmd, "r");
    if (!fp) return xstrdup("");
    char buf[1024] = {0};
    if (fgets(buf, sizeof(buf), fp)) { /* ok */ }
    pclose(fp);
    return xstrdup(trim(buf));
}

/* ── build bindings for a lib ───────────────────────────────────────────── */

static void build_bindings(Project *p, CCFamily fam,
                            const char *binddir, const char *root,
                            const char *lib_name,
                            size_t lib_idx) {
    char module_name[256] = {0};
    BindType bt = detect_binding(binddir, module_name, sizeof(module_name));

    const char *extra = NULL;
    char extra_buf[256] = {0};
    char ext_inc_buf[2048] = {0};

    if (bt == BIND_PYBIND11) {
        char *pb = get_pybind11_includes(p->python);
        if (!pb) die("pybind11 not found; try: pip install pybind11");
        char *pyinc = get_python_includes(p->python);
        
        /* FIX: Use -isystem to suppress warnings in external headers */
        snprintf(ext_inc_buf, sizeof(ext_inc_buf), "-isystem %s -isystem %s", 
                 pb, pyinc ? pyinc : "");
        
        snprintf(extra_buf, sizeof(extra_buf),
                 "-fPIC -fexceptions -frtti");
        extra = extra_buf;
        free(pb);
        free(pyinc);
        if (module_name[0] == '\0')
            snprintf(module_name, sizeof(module_name), "%s", lib_name);
    } else {
        extra = "-fPIC";
    }

    DIR *d = opendir(binddir);
    if (!d) return;

    mkdirp(".build/obj");
    mkdirp(".build/deps");
    mkdirp(".build/lib");

    StrArr bind_objs = {0};
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!is_src_ext(e->d_name)) continue;
        char *src = path_join(binddir, e->d_name);
        char *oname = obj_name(src);
        char *opath = path_join(".build/obj", oname);
        free(oname);

        /* Pass ext_inc_buf to silence external warnings */
        compile_source(p, fam, src, opath, extra, 1, ext_inc_buf);
        sarr_push(&bind_objs, opath);
        free(opath);
        free(src);
    }
    closedir(d);

    if (bind_objs.count == 0) {
        sarr_free(&bind_objs);
        return;
    }

    int *visited = calloc(p->nlibs, sizeof(int));
    StrArr link_libs = {0};
    dep_closure(p, lib_idx, visited, &link_libs);
    free(visited);

    char link_args[8192] = {0};
    snprintf(link_args, sizeof(link_args), "-L.build/lib");
    for (size_t i = link_libs.count; i-- > 0;) {
        char apath[PATH_MAX];
        snprintf(apath, sizeof(apath), ".build/lib/lib%s.a", link_libs.items[i]);
        if (file_exists(apath)) {
            char tmp[256];
            snprintf(tmp, sizeof(tmp), " -l%s", link_libs.items[i]);
            strncat(link_args, tmp, sizeof(link_args)-strlen(link_args)-1);
        }
    }
    sarr_free(&link_libs);

    char *ext = get_ext_suffix(p->python);
    char so_name[PATH_MAX];
    char *pyldflags = get_python_ldflags(p->python);

    if (bt == BIND_PYBIND11) {
        snprintf(so_name, sizeof(so_name), ".build/lib/%s%s", module_name, ext);
    } else {
        snprintf(so_name, sizeof(so_name), ".build/lib/lib%s%s", lib_name, ext);
    }

    char obj_list[8192] = {0};
    for (size_t i = 0; i < bind_objs.count; i++) {
        strncat(obj_list, " ", sizeof(obj_list)-strlen(obj_list)-1);
        strncat(obj_list, bind_objs.items[i], sizeof(obj_list)-strlen(obj_list)-1);
    }

    char link_cmd[16384];
    snprintf(link_cmd, sizeof(link_cmd),
        "%s -shared%s %s %s -o %s",
        p->compiler, obj_list, link_args,
        pyldflags ? pyldflags : "", so_name);

    fprintf(stdout, "  link %s\n", so_name);
    run(link_cmd);

    free(ext);
    free(pyldflags);

    char tests_dir[PATH_MAX];
    snprintf(tests_dir, sizeof(tests_dir), "%s/tests", root);
    if (dir_exists(tests_dir)) {
        const char *fname = strrchr(so_name, '/');
        fname = fname ? fname+1 : so_name;
        char dest[PATH_MAX * 2];
        snprintf(dest, sizeof(dest), "%s/%s", tests_dir, fname);
        copy_file(so_name, dest);
    }

    sarr_free(&bind_objs);
}

/* ── forge build ────────────────────────────────────────────────────────── */

static void cmd_build(Project *p) {
    CCFamily fam = compiler_family(p->compiler);
    int single_lib = (p->nlibs == 0);

    size_t *order = NULL;

    if (single_lib) {
        /* single-lib / executable mode */
        /* create a fake lib entry */
        Lib *l = project_add_lib(p);
        l->name = xstrdup(p->name);
        l->src  = dir_exists("src") ? xstrdup("src") : NULL;
        order = malloc(sizeof(size_t));
        if (!order) die("out of memory");
        order[0] = 0;
        p->nlibs = 1;
    } else {
        order = topo_sort(p);
    }

    /* print build order */
    fprintf(stdout, "Build order:");
    for (size_t i = 0; i < p->nlibs; i++)
        fprintf(stdout, " %s", p->libs[order[i]].name);
    fprintf(stdout, "\n");

    for (size_t i = 0; i < p->nlibs; i++) {
        size_t idx = order[i];
        Lib *l = &p->libs[idx];
        fprintf(stdout, "[%s]\n", l->name);

        /* check for bindings before compiling so we can pass -fPIC */
        char *srcdir = lib_src_dir(p, l);
        char *root = lib_root(srcdir);
        free(srcdir);
        char *binddir = path_join(root, "bindings");
        int has_bindings = dir_exists(binddir);

        build_lib(p, fam, idx, single_lib, has_bindings);

        if (has_bindings) {
            build_bindings(p, fam, binddir, root, l->name, idx);
        }
        free(binddir);
        free(root);
    }

    free(order);

    /* build [[exe]] entries */
    for (size_t i = 0; i < p->nexes; i++) {
        fprintf(stdout, "[%s]\n", p->exes[i].name);
        build_exe(p, fam, i);
    }
}

/* ── forge test ─────────────────────────────────────────────────────────── */

static int run_test(const char *cmd, const char *label) {
    fprintf(stdout, "  run %s\n", label);
    int ret = system(cmd);
    if (ret == 0) {
        fprintf(stdout, "  PASS %s\n", label);
        return 1;
    } else {
        fprintf(stdout, "  FAIL %s (exit %d)\n", label, ret);
        return 0;
    }
}

static void cmd_test(Project *p) {
    cmd_build(p);

    CCFamily fam = compiler_family(p->compiler);
    int total = 0, passed = 0;

    size_t *order = topo_sort(p);

    for (size_t i = 0; i < p->nlibs; i++) {
        size_t idx = order[i];
        Lib *l = &p->libs[idx];

        char *srcdir = lib_src_dir(p, l);
        char *root = lib_root(srcdir);
        free(srcdir);

        char *testsdir = path_join(root, "tests");
        free(root);

        if (!dir_exists(testsdir)) {
            free(testsdir);
            continue;
        }

        int *visited = calloc(p->nlibs, sizeof(int));
        StrArr link_libs = {0};
        dep_closure(p, idx, visited, &link_libs);
        free(visited);

        char link_args[8192] = {0};
        snprintf(link_args, sizeof(link_args), "-L.build/lib");
        for (size_t li = link_libs.count; li-- > 0;) {
            char apath[PATH_MAX];
            snprintf(apath, sizeof(apath), ".build/lib/lib%s.a", link_libs.items[li]);
            if (file_exists(apath)) {
                char tmp[256];
                snprintf(tmp, sizeof(tmp), " -l%s", link_libs.items[li]);
                strncat(link_args, tmp, sizeof(link_args)-strlen(link_args)-1);
            }
        }
        sarr_free(&link_libs);

        mkdirp(".build/bin/tests");

        DIR *d = opendir(testsdir);
        if (d) {
            struct dirent *e;
            while ((e = readdir(d)) != NULL) {
                if (!is_src_ext(e->d_name)) continue;
                char *src = path_join(testsdir, e->d_name);
                char *oname = obj_name(src);
                char *opath = path_join(".build/obj", oname);
                free(oname);

                /* Maintain strictness for project tests */
                compile_source(p, fam, src, opath, NULL, 0, NULL);

                char binname[256];
                strncpy(binname, e->d_name, sizeof(binname)-1);
                binname[sizeof(binname)-1] = '\0';
                char *dot = strrchr(binname, '.');
                if (dot) *dot = '\0';

                char binpath[PATH_MAX];
                snprintf(binpath, sizeof(binpath), ".build/bin/tests/%s", binname);

                char link_cmd[16384];
                snprintf(link_cmd, sizeof(link_cmd),
                    "%s %s %s -o %s",
                    p->compiler, opath, link_args, binpath);

                fprintf(stdout, "  link %s\n", binpath);
                int r = system(link_cmd);
                if (r != 0) {
                    fprintf(stderr, "forge: error: failed to link test %s\n", src);
                    total++;
                    free(opath);
                    free(src);
                    continue;
                }

                total++;
                passed += run_test(binpath, binname);

                free(opath);
                free(src);
            }
            closedir(d);
        }

        DIR *d2 = opendir(testsdir);
        int checked_pytest = 0;
        if (d2) {
            struct dirent *e;
            StrArr pyfiles = {0};
            while ((e = readdir(d2)) != NULL) {
                if (strncmp(e->d_name, "test_", 5) != 0) continue;
                const char *dot = strrchr(e->d_name, '.');
                if (!dot || strcmp(dot, ".py") != 0) continue;
                sarr_push(&pyfiles, e->d_name);
            }
            closedir(d2);

            if (pyfiles.count > 0) {
                if (!checked_pytest) {
                    char check_cmd[512];
                    snprintf(check_cmd, sizeof(check_cmd),
                        "%s -m pytest --version > /dev/null 2>&1", p->python);
                    if (system(check_cmd) != 0)
                        die("pytest not found; try: pip install pytest");
                    checked_pytest = 1;
                }
                for (size_t pi = 0; pi < pyfiles.count; pi++) {
                    char pytest_cmd[PATH_MAX + 256];
                    snprintf(pytest_cmd, sizeof(pytest_cmd),
                        "cd %s && %s -m pytest -v --tb=short %s",
                        testsdir, p->python, pyfiles.items[pi]);
                    total++;
                    passed += run_test(pytest_cmd, pyfiles.items[pi]);
                }
            }
            sarr_free(&pyfiles);
        }

        free(testsdir);
    }

    free(order);
    fprintf(stdout, "\nResults: %d/%d passed\n", passed, total);
    if (passed < total) exit(1);
}

/* ── usage ──────────────────────────────────────────────────────────────── */

static void usage(void) {
    fprintf(stdout,
        "usage: forge <command>\n"
        "\n"
        "commands:\n"
        "  build   compile all libraries and bindings\n"
        "  test    build then run all test suites\n"
        "  clean   remove build outputs\n");
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "clean") == 0) {
        cmd_clean();
        return 0;
    }

    if (strcmp(cmd, "build") == 0 || strcmp(cmd, "test") == 0) {
        if (!file_exists("project.toml"))
            die("project.toml not found");

        Project p;
        project_init(&p);
        parse_toml("project.toml", &p);
        validate_project(&p);

        if (strcmp(cmd, "build") == 0)
            cmd_build(&p);
        else
            cmd_test(&p);

        return 0;
    }

    fprintf(stderr, "forge: unknown command '%s'\n", cmd);
    usage();
    return 1;
}
