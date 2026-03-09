#ifndef ANVIL_ERROR_ASSERT_HPP
#define ANVIL_ERROR_ASSERT_HPP

#include <cstdlib>

/**
 *  @brief A requirement characterizes a condition that must hold under correct usage
 *
 *  Requirements define properties and conditions that must be satisfied and truthful
 *  at certain points during a program's execution. If a requirement is broken, it
 *  indicates a failure on the part of the invoker of a piece of code to follow the
 *  code's contract.
 *
 *  @invariant produces no side effects
 *  @invariant forces the return of an error_code
 *
 *  @param[in] condition        The condition or property that should be satisfied.
 *  @param[in] error_code       The error code that should be returned.
 *
 *  @note For functions that return void, set error_code = void
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
 *  An invariant defines a property or condition that the invokee of a piece of code wishes to
 *  mark as important, to the point that a failure should be considered grounds to halt a program.
 *
 *  @invariant halts a process if condition or property fails.
 *
 *  @param[in] condition        The condition that should be satisfied to continue execution of the program.
 *
 *  @note A condition need not be proven to always hold to be promoted to an invariant. An invokee can use an
 *  invariant to indicate that a failed condition is of no interest to the invoker because no reasonable
 *  actions can be taken by either the invokee or the invoker to address the issue caused by the breakage of
 *  the condition.
 */
#define INVARIANT(condition)                                                                                           \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        abort();                                                                                       \
                }                                                                                                      \
        } while (0)

#endif // !ANVIL_ERROR_ASSERT_HPP
