/**
 * @file utility_internal.h
 * @brief Internal utility functions for Anvil Memory system.
 *
 * This header provides utility macros and functions used throughout the Anvil Memory
 * library for tasks like error handling, memory poisoning, and performance optimization.
 * These are internal implementation details and not part of the public API.
 */

#ifndef ANVIL_MEMORY_UTILITY_H
#define ANVIL_MEMORY_UTILITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

/**
 * @brief Attribute to automatically call a cleanup function when a variable goes out of scope.
 *
 * @param clean_up_func The function to call for cleanup when the variable goes out of scope.
 */
#define DEFER(clean_up_func)  __attribute__((cleanup(clean_up_func)))

/**
 * @brief Optimization hint indicating that a condition is likely to be true.
 *
 * @param x The condition to evaluate.
 */
#define likely(x)             __builtin_expect(!!(x), 1)

/**
 * @brief Optimization hint indicating that a condition is unlikely to be true.
 *
 * @param x The condition to evaluate.
 */
#define unlikely(x)           __builtin_expect(!!(x), 0)

/**
 * @brief Pattern used to poison freed or uninitialized memory for debugging.
 */
#define MEMORY_POISON_PATTERN 0xDEADC0DE

/**
 * @brief Assert an invariant condition with formatted error message.
 *
 * This macro checks if an expression evaluates to true. If not, it calls
 * log_and_crash with the source expression, file, line and formatted error message.
 * This implements the fail-fast design philosophy of the library.
 *
 * @param expr The expression that must be true.
 * @param fmt Format string for the error message.
 * @param ... Arguments for the format string.
 */
#define INVARIANT(expr, fmt, ...)                                                                                      \
	do {                                                                                                           \
		if (!(expr)) {                                                                                         \
			log_and_crash(#expr, __FILE__, __LINE__, fmt, ##__VA_ARGS__);                                  \
		}                                                                                                      \
	} while (0)

#ifdef DEBUG
/**
 * @brief Debug version of mmap that poisons allocated memory.
 *
 * In debug builds, this function replaces the standard mmap and fills
 * newly allocated memory with the poison pattern for easier debugging.
 *
 * @param addr Suggested address for the mapping.
 * @param size Size of the memory region to map.
 * @param __prod Protection flags.
 * @param __flags Mapping flags.
 * @param __fd File descriptor.
 * @param __offset Offset within the file.
 * @return Pointer to the mapped memory region.
 */
inline void *debug_mmap(void *addr, size_t size, int __prod, __flags, __fd, __off_t __offset) {
	void *result = mmap(addr, size, __prod, __flags, __fd, __off_t__offset);
	if (result) {
		memset(addr, MEMOMEMORY_POISON_PATTERN, size);
	}
	return result;
}
#define mmap debug_mmap
#endif    // DEBUG

/**
 * @brief Logs the time, file and code line and then aborts.
 *
 * @param[in] `expr` that failed.
 * @param[in] `file` filename for where a failure occured.
 * @param[in] `line` line that failed.
 * @param[in] `fmt`  formatted error message to log.
 * @param[in] ... formating arguments.
 */
void __attribute__((noreturn)) log_and_crash(const char *expr, const char *file, int line, const char *fmt, ...);

#endif    // !ANVIL_MEMORY_UTILITY_H
