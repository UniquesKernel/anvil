#include "memory/error.hpp"
#include <cstdarg>


namespace anvil::error {

[[noreturn]] ANVIL_ATTR_COLD ANVIL_ATTR_NOINLINE void abort_invariant(const char* expr, const char* file, int line,
                                                                      Error err, const char* fmt, ...) {
        // Get timestamp
        time_t     now     = time(nullptr);
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
        const char* err_msg = error_message(err);
        ErrorDomain domain  = error_domain(err);
        uint8_t     code    = error_code(err);

#ifdef LOG_FILE
        FILE* log = fopen(LOG_FILE, "a");
        if (log) {
                fprintf(log, "[%s] INVARIANT VIOLATION\n", time_buf);
                fprintf(log, "  Expression: %s\n", expr);
                fprintf(log, "  Location: %s:%d\n", file, line);
                fprintf(log, "  Error: [%u:%02X] %s\n", static_cast<unsigned>(domain), code, err_msg);
                if (user_msg[0]) {
                        fprintf(log, "  Details: %s\n", user_msg);
                }
                fprintf(log, "\n");
                fclose(log);
        }
#endif

        // Print to stderr
        fprintf(stderr, "\n*** INVARIANT VIOLATION ***\n");
        fprintf(stderr, "Expression: %s\n", expr);
        fprintf(stderr, "Location: %s:%d\n", file, line);
        fprintf(stderr, "Error: [%u:%02X] %s\n", static_cast<unsigned>(domain), code, err_msg);
        if (user_msg[0]) {
                fprintf(stderr, "Details: %s\n", user_msg);
        }

        abort();
}

} // namespace anvil::error