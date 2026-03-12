#ifndef ANVIL_ERROR_ASSERT_HPP
#define ANVIL_ERROR_ASSERT_HPP

#include <cstdlib>

/**
 *  @brief A requirement characterizes a condition that must hold under correct usage
 *
 *  Requirements define properties and conditions that must hold true
 *  at certain points during a program's execution. If a requirement is broken, it
 *  indicates a failure on the part of the invoker of a piece of code to follow the
 *  code's contract.
 *
 *  @invariant Produces no side effects.
 *  @invariant Forces the return of an error code.
 *
 *  @param[in] condition        The condition or property that should be satisfied.
 *  @param[in] error_code       The error code that should be returned.
 *
 *  @note For functions that return `void`, set `error_code = void`.
 */
#define REQUIRE(condition, error_code)                                                                                 \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        return error_code;                                                                             \
                }                                                                                                      \
        } while (0)

/**
 *  @brief An invariant characterizes a condition or property that the invokee guarantees to be satisfied
 *
 *  An invariant defines a property or condition that the invokee of a piece of code
 *  marks as important, to the point that a failure should be considered grounds to halt the program.
 *
 *  @invariant Halts the process if the condition or property fails.
 *
 *  @param[in] condition        The condition that should be satisfied to continue execution of the program.
 *
 *  @note A condition need not be proven to always hold before it is promoted to an invariant. An invokee can use an
 *  invariant to indicate that a failed condition is of no interest to the invoker because no reasonable
 *  action can be taken by either the invokee or the invoker to address the issue caused by the failed
 *  condition.
 */
#define INVARIANT(condition)                                                                                           \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        abort();                                                                                       \
                }                                                                                                      \
        } while (0)

#endif // !ANVIL_ERROR_ASSERT_HPP
