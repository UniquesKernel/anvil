#ifndef ANVIL_ERROR_ASSERT_HPP
#define ANVIL_ERROR_ASSERT_HPP

#include <cstdlib>

/**
 *  @brief defines the INVARIANT macro.
 *
 *  A runtime check used to ensure an invariant is true.
 *  If the invariant is broken, the invariant returns the
 *  associated `error_code`.
 *
 *  Invariants are conditions that if broken are recoverable.
 *
 *  @param[in] condition        The invariant that should be true
 *  @param[in] error_code       The error_code the calling scope should return if the condition is false
 *
 *  @note The macro does not return an error_code like a function would, it either no-ops or
 *        it forces the calling function to return the error_code
 */
#define INVARIANT(condition, error_code)                                                                               \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        return error_code;                                                                             \
                }                                                                                                      \
        } while (0)

/**
 * @brief defines the GUARANTEE macro.
 *
 * A runtime check that ensures the program is in a healthy state.
 * If the associated check is false, the program is in an unhealthy
 * and unrecoverable state.
 *
 * If the GUARANTEE is broken, the program terminates because the state
 * is unrecoverable and dangerous to continue in.
 *
 * @param[in] condition         The condition that should be guaranteed to hold
 *
 * @note The macro does not return a value, it either no-ops or it aborts the program.
 */
#define GUARANTEE(condition)                                                                                           \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        abort();                                                                                       \
                }                                                                                                      \
        } while (0)

#endif // !ANVIL_ERROR_ASSERT_HPP
