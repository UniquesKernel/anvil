#ifndef ANVIL_ERROR_ASSERT_HPP
#define ANVIL_ERROR_ASSERT_HPP

#include <cstdlib>

#define INVARIANT(condition, error_value)                                                                              \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        return error_value;                                                                            \
                }                                                                                                      \
        } while (0)

#define GUARANTEE(condition)                                                                                           \
        do {                                                                                                           \
                if (!(condition)) [[unlikely]] {                                                                       \
                        abort();                                                                                       \
                }                                                                                                      \
        } while (0)

#endif // !ANVIL_ERROR_ASSERT_HPP
