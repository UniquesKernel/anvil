#ifndef ANVIL_ERROR_HPP
#define ANVIL_ERROR_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

using std::size_t;
using std::uint16_t;
using std::uint8_t;

/**
 * @brief Attribute to automatically call a cleanup function when a variable goes out of scope.
 *
 * @param clean_up_func The function to call for cleanup when the variable goes out of scope.
 */
#define DEFER(clean_up_func) __attribute__((cleanup(clean_up_func)))

// Convinient Branch Prediction Hints
#define UNLIKELY(x)          __builtin_expect(!!(x), 0)
#define LIKELY(x)            __builtin_expect(!!(x), 1)

// Cold Path optimization for error_handlers
#define COLD_FUNC            __attribute__((cold, noinline))
#define HOT_FUNC             __attribute__((hot, always_inline))

typedef enum {
        ERR_DOMAIN_NONE    = 0,
        ERR_DOMAIN_MEMORY  = 1,
        ERR_DOMAIN_IO      = 2,
        ERR_DOMAIN_NETWORK = 3,
        ERR_DOMAIN_STATE   = 4,
        ERR_DOMAIN_VALUE   = 5,
        ERR_DOMAIN_MAX     = 16
} ErrorDomain;
static_assert(ERR_DOMAIN_MAX <= 16, "Domain count exceeds 4-bit limit");

typedef enum {
        ERR_SEVERITY_INFO    = 0,
        ERR_SEVERITY_WARNING = 1,
        ERR_SEVERITY_ERROR   = 2,
        ERR_SEVERITY_FATAL   = 3 // Invariant violations
} ErrorSeverity;

typedef struct {
        uint16_t severity : 2; // Severity level (0-3)
        uint16_t reserved : 2; // Future use
        uint16_t domain : 4;   // Error domain (0-15)
        uint16_t code : 8;     // Error code within domain (0-255)
} ErrorCode;
static_assert(sizeof(ErrorCode) == 2, "Error Code is expected to be 2 bytes");

// Convert to/from uint16_t for efficient passing
#define ERROR_TO_U16(e) (*(uint16_t*)&(e))
#define U16_TO_ERROR(u) (*(ErrorCode*)&(u))

// X-Macro definitions for invariant violations (fatal errors)
#define INVARIANT_ERRORS(X)                                                                                            \
        X(INV_NULL_POINTER, ERR_DOMAIN_MEMORY, 0x01, "Null pointer violation")                                         \
        X(INV_ZERO_SIZE, ERR_DOMAIN_MEMORY, 0x02, "Size must be positive")                                             \
        X(INV_BAD_ALIGNMENT, ERR_DOMAIN_MEMORY, 0x03, "Alignment not power of two")                                    \
        X(INV_ALIGN_TOO_LARGE, ERR_DOMAIN_MEMORY, 0x04, "Alignment exceeds maximum")                                   \
        X(INV_INVALID_STATE, ERR_DOMAIN_STATE, 0x01, "Invalid state transition")                                       \
        X(INV_OUT_OF_RANGE, ERR_DOMAIN_VALUE, 0x01, "Value out of valid range")                                        \
        X(INV_PRECONDITION, ERR_DOMAIN_STATE, 0x02, "Precondition violation")                                          \
        X(INV_POSTCONDITION, ERR_DOMAIN_STATE, 0x03, "Postcondition violation")

// X-Macro definitions for runtime errors (recoverable)
#define RUNTIME_ERRORS(X)                                                                                              \
        X(ERR_OUT_OF_MEMORY, ERR_DOMAIN_MEMORY, 0x10, "Memory allocation failed")                                      \
        X(ERR_MEMORY_PERMISSION_CHANGE, ERR_DOMAIN_MEMORY, 0x20,                                                       \
          "Failed to change permissions on virutal and physical memory")                                               \
        X(ERR_MEMORY_DEALLOCATION, ERR_DOMAIN_MEMORY, 0x30,                                                            \
          "Failed to properly deallocate virtual or physical memory")                                                  \
        X(ERR_MEMORY_WRITE_ERROR, ERR_DOMAIN_MEMORY, 0x40, "Failed to write to physical memory")                       \
        X(ERR_FILE_NOT_FOUND, ERR_DOMAIN_IO, 0x01, "File not found")                                                   \
        X(ERR_PERMISSION, ERR_DOMAIN_IO, 0x02, "Permission denied")                                                    \
        X(ERR_NETWORK_DOWN, ERR_DOMAIN_NETWORK, 0x01, "Network unreachable")                                           \
        X(ERR_TIMEOUT, ERR_DOMAIN_NETWORK, 0x02, "Operation timeout")                                                  \
        X(ERR_BUSY, ERR_DOMAIN_STATE, 0x10, "Resource busy")                                                           \
        X(ERR_NOT_INITIALIZED, ERR_DOMAIN_STATE, 0x11, "Not initialized")                                              \
        X(ERR_STACK_DEPTH, ERR_DOMAIN_STATE, 0x12, "Stack depth exceeded")

// Generate error code enum
#define X(name, domain, code, msg) name = ((domain) << 12) | ((code) << 4) | ERR_SEVERITY_FATAL,
typedef enum { ERR_SUCCESS = 0, INVARIANT_ERRORS(X) } InvariantError;
#undef X

#define X(name, domain, code, msg) name = ((domain) << 12) | ((code) << 4) | ERR_SEVERITY_ERROR,
typedef enum { RUNTIME_ERRORS(X) } RuntimeError;
#undef X

typedef uint16_t             Error;

// #define X(name, domain, code, msg) [name] = msg,
// static const char* const invariant_messages[] = {[ERR_SUCCESS] = "Success", INVARIANT_ERRORS(X)};

// static const char* const runtime_messages[]   = {RUNTIME_ERRORS(X)};
// #undef X

COLD_FUNC static const char* anvil_error_message(Error err) {
        if (err == ERR_SUCCESS)
                return "Success";

        ErrorCode e = U16_TO_ERROR(err);

        if (e.severity == ERR_SEVERITY_FATAL) {
                switch (err) {
#define X(name, domain, code, msg)                                                                                     \
        case name:                                                                                                     \
                return msg;
                        INVARIANT_ERRORS(X)
#undef X
                default:
                        return "Unknown invariant error";
                }
        } else {
                switch (err) {
#define X(name, domain, code, msg)                                                                                     \
        case name:                                                                                                     \
                return msg;
                        RUNTIME_ERRORS(X)
#undef X
                default:
                        return "Unknown runtime error";
                }
        }
}

// Error context for rich diagnostics (stack-allocated)
typedef struct error_context {
        Error                 error;
        const char*           file;
        int                   line;
        const char*           expr;
        struct error_context* parent;
} ErrorContext;

extern __thread ErrorContext*            g_error_context;

// Core error handling functions
COLD_FUNC void __attribute__((noreturn)) anvil_abort_invariant(const char* expr, const char* file, int line,
                                                               InvariantError err, const char* fmt, ...);

COLD_FUNC Error                          anvil_set_error(Error err, const char* file, int line);

COLD_FUNC static Error                   anvil_get_last_error(void) {
        return g_error_context ? g_error_context->error : ERR_SUCCESS;
}

HOT_FUNC static inline bool anvil_is_error(Error err) {
        return UNLIKELY(err != ERR_SUCCESS);
}

HOT_FUNC static inline bool anvil_is_fatal(Error err) {
        ErrorCode e = U16_TO_ERROR(err);
        return e.severity == ERR_SEVERITY_FATAL;
}

// Invariant checking macros (abort on failure)
#define INVARIANT(expr, err, ...)                                                                                      \
        do {                                                                                                           \
                if (UNLIKELY(!(expr))) {                                                                               \
                        anvil_abort_invariant(#expr, __FILE__, __LINE__, err, ##__VA_ARGS__);                          \
                }                                                                                                      \
        } while (0)

// Common invariant checks with optimized messages
#define INVARIANT_NOT_NULL(ptr) INVARIANT((ptr) != NULL, INV_NULL_POINTER, "%s", #ptr)

#define INVARIANT_POSITIVE(val) INVARIANT((val) > 0, INV_ZERO_SIZE, "%s = %zd", #val, (size_t)(val))

#define INVARIANT_RANGE(val, min, max)                                                                                 \
        INVARIANT((val) >= (min) && (val) <= (max), INV_OUT_OF_RANGE, "%s = %d not in [%d, %d]", #val, (val), (min),   \
                  (max))

// Runtime error checking macros (graceful handling)
#define CHECK(expr, err) (LIKELY(expr) ? ERR_SUCCESS : anvil_set_error(err, __FILE__, __LINE__))

#define CHECK_NULL(ptr)  CHECK((ptr) != NULL, ERR_OUT_OF_MEMORY)

#define TRY(expr)                                                                                                      \
        do {                                                                                                           \
                Error _err = (expr);                                                                                   \
                if (UNLIKELY(anvil_is_error(_err))) {                                                                  \
                        return _err;                                                                                   \
                }                                                                                                      \
        } while (0)

#define TRY_CHECK(expr, err)                                                                                           \
        do {                                                                                                           \
                Error _err = CHECK(expr, err);                                                                         \
                if (UNLIKELY(anvil_is_error(_err))) {                                                                  \
                        return _err;                                                                                   \
                }                                                                                                      \
        } while (0)

// Error context management
#define WITH_ERROR_CONTEXT(ctx_name)                                                                                   \
        ErrorContext ctx_name = {                                                                                      \
            .error = ERR_SUCCESS, .file = __FILE__, .line = __LINE__, .expr = __func__, .parent = g_error_context};    \
        ErrorContext* DEFER(anvil_restore_context) _saved_ctx = g_error_context;                                       \
        g_error_context                                       = &ctx_name;

static inline void anvil_restore_context(ErrorContext** ctx) {
        if (CHECK(ctx && (*ctx), INV_NULL_POINTER) != ERR_SUCCESS) {
                return;
        }
        g_error_context = (*ctx);
}

// Error code analysis helpers
COLD_FUNC static ErrorDomain anvil_error_domain(Error err) {
        ErrorCode e = U16_TO_ERROR(err);
        return (ErrorDomain)e.domain;
}

COLD_FUNC static uint8_t anvil_error_code(Error err) {
        ErrorCode e = U16_TO_ERROR(err);
        return e.code;
}

/*
COLD_FUNC static const char* anvil_error_message(Error err) {
        if (err == ERR_SUCCESS)
                return "Success";

        ErrorCode e = U16_TO_ERROR(err);
        if (e.severity == ERR_SEVERITY_FATAL) {
                return invariant_messages[err];
        } else {
                return runtime_messages[err];
        }
}
*/
#ifdef ANVIL_ERROR_STATS
typedef struct {
        uint64_t invariant_checks;
        uint64_t runtime_checks;
        uint64_t errors_set;
        uint64_t branch_hints_correct;
} ErrorStats;

#define STAT_INC(field) __atomic_add_fetch(&g_error_stats.field, 1, __ATOMIC_RELAXED)
#else
#define STAT_INC(field) ((void)0)
#endif

#endif // ANVIL_ERROR_HPP