#ifndef ANVIL_ERROR_ASSERT_HPP
#define ANVIL_ERROR_ASSERT_HPP

#define INVARIANT(condition, error_value)                                                                              \
        do {                                                                                                           \
                if (!(condition))                                                                                      \
                        return error_value;                                                                            \
        } while (0)

#endif // !ANVIL_ERROR_ASSERT_HPP
