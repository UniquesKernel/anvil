#include "memory/error.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef ANVIL_ERROR_STATS
ErrorStats g_error_stats = {0};
#endif
__thread ErrorContext* g_error_context = NULL;

COLD_FUNC void anvil_abort_invariant(const char* expr, const char* file, int line, InvariantError err, const char* fmt,
                                     ...) {
        STAT_INC(invariant_checks);

        // Get timestamp
        time_t     now     = time(NULL);
        struct tm* tm_info = localtime(&now);
        char       time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

        // Format user message
        char user_msg[512] = {0};
        if (fmt) {
                va_list args;
                va_start(args, fmt);
                vsnprintf(user_msg, sizeof(user_msg), fmt, args);
                va_end(args);
        }

        // Get error details
        const char* err_msg = anvil_error_message(err);
        ErrorDomain domain  = anvil_error_domain(err);
        uint8_t     code    = anvil_error_code(err);

#ifdef LOG_FILE
        FILE* log = fopen(LOG_FILE, "a");
        if (log) {
                fprintf(log, "[%s] INVARIANT VIOLATION\n", time_buf);
                fprintf(log, "  Expression: %s\n", expr);
                fprintf(log, "  Location: %s:%d\n", file, line);
                fprintf(log, "  Error: [%d:%02X] %s\n", domain, code, err_msg);
                if (user_msg[0]) {
                        fprintf(log, "  Details: %s\n", user_msg);
                }

                if (g_error_context) {
                        fprintf(log, "\nCall Stack:\n");
                        ErrorContext* ctx   = g_error_context;
                        int           depth = 0;
                        while (ctx && depth < 20) { // Limit depth to prevent infinite loops
                                fprintf(log, "  [%d] %s:%d", depth, ctx->file, ctx->line);
                                if (ctx->error != ERR_SUCCESS) {
                                        fprintf(log, " (error %04X: %s)", ctx->error, anvil_error_message(ctx->error));
                                }
                                fprintf(log, "\n");
                                ctx = ctx->parent;
                                depth++;
                        }
                }
                fprintf(log, "\n");
                fclose(log);
        }
#endif

        // Print to stderr
        fprintf(stderr, "\n*** INVARIANT VIOLATION ***\n");
        fprintf(stderr, "Expression: %s\n", expr);
        fprintf(stderr, "Location: %s:%d\n", file, line);
        fprintf(stderr, "Error: [%d:%02X] %s\n", domain, code, err_msg);
        if (user_msg[0]) {
                fprintf(stderr, "Details: %s\n", user_msg);
        }

        // Print context chain if available

        if (g_error_context) {
                fprintf(stderr, "\nError Context:\n");
                ErrorContext* ctx   = g_error_context;
                int           depth = 0;
                while (ctx && depth < 10) {
                        fprintf(stderr, "  [%d] %s:%d in %s()\n", depth, ctx->file, ctx->line,
                                ctx->expr ? ctx->expr : "unknown");
                        ctx = ctx->parent;
                        depth++;
                }
        }

        abort();
}

COLD_FUNC Error anvil_set_error(Error err, const char* file, int line) {
        STAT_INC(errors_set);
        STAT_INC(runtime_checks);

        if (g_error_context) {
                g_error_context->error = err;
                g_error_context->file  = file;
                g_error_context->line  = line;
        }

        return err;
}