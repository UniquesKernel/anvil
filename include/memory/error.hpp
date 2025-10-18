#ifndef ANVIL_ERROR_HPP
#define ANVIL_ERROR_HPP

#include "constants.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <utility>

namespace anvil::error {

using Error = std::uint16_t;

enum class Domain : std::uint8_t {
        None   = 0,
        Memory = 1,
        State  = 2,
        Value  = 3,
};

enum class Severity : std::uint8_t {
        Success = 0,
        Warning = 1,
        Failure = 2,
        Fatal   = 3,
};

struct Descriptor {
        Error       value;
        Domain      domain;
        Severity    severity;
        const char* message;
};

inline constexpr std::uint16_t DOMAIN_MASK   = 0x0F;
inline constexpr std::uint16_t SEVERITY_MASK = 0x0F;
inline constexpr std::uint16_t CODE_MASK     = 0xFF;
inline constexpr std::uint16_t DOMAIN_SHIFT  = 12;
inline constexpr std::uint16_t CODE_SHIFT    = 4;

constexpr Error                make_error(Domain domain, Severity severity, std::uint8_t code) noexcept {
        return (static_cast<Error>(domain) << DOMAIN_SHIFT) | (static_cast<Error>(code) << CODE_SHIFT) |
               static_cast<Error>(severity);
}

inline constexpr Error ERR_SUCCESS                      = 0;
inline constexpr Error INV_NULL_POINTER                 = make_error(Domain::Memory, Severity::Fatal, 0x01);
inline constexpr Error INV_ZERO_SIZE                    = make_error(Domain::Memory, Severity::Fatal, 0x02);
inline constexpr Error INV_BAD_ALIGNMENT                = make_error(Domain::Memory, Severity::Fatal, 0x03);
inline constexpr Error INV_INVALID_STATE                = make_error(Domain::State, Severity::Fatal, 0x01);
inline constexpr Error INV_OUT_OF_RANGE                 = make_error(Domain::Value, Severity::Fatal, 0x01);
inline constexpr Error INV_PRECONDITION                 = make_error(Domain::State, Severity::Fatal, 0x02);
inline constexpr Error ERR_OUT_OF_MEMORY                = make_error(Domain::Memory, Severity::Failure, 0x10);
inline constexpr Error ERR_MEMORY_PERMISSION_CHANGE     = make_error(Domain::Memory, Severity::Failure, 0x20);
inline constexpr Error ERR_MEMORY_DEALLOCATION          = make_error(Domain::Memory, Severity::Failure, 0x30);
inline constexpr Error ERR_STACK_OVERFLOW               = make_error(Domain::Memory, Severity::Failure, 0x40);

inline constexpr std::array<Descriptor, 11> DESCRIPTORS = {
    Descriptor{ERR_SUCCESS, Domain::None, Severity::Success, "Success"},
    Descriptor{INV_NULL_POINTER, Domain::Memory, Severity::Fatal, "Null pointer violation"},
    Descriptor{INV_ZERO_SIZE, Domain::Memory, Severity::Fatal, "Size must be positive"},
    Descriptor{INV_BAD_ALIGNMENT, Domain::Memory, Severity::Fatal, "Alignment not power of two"},
    Descriptor{INV_INVALID_STATE, Domain::State, Severity::Fatal, "Invalid state transition"},
    Descriptor{INV_OUT_OF_RANGE, Domain::Value, Severity::Fatal, "Value out of valid range"},
    Descriptor{INV_PRECONDITION, Domain::State, Severity::Fatal, "Precondition violation"},
    Descriptor{ERR_OUT_OF_MEMORY, Domain::Memory, Severity::Failure, "Memory allocation failed"},
    Descriptor{ERR_MEMORY_PERMISSION_CHANGE, Domain::Memory, Severity::Failure,
               "Failed to change permissions on virutal and physical memory"},
    Descriptor{ERR_MEMORY_DEALLOCATION, Domain::Memory, Severity::Failure,
               "Failed to properly deallocate virtual or physical memory"},
    Descriptor{ERR_STACK_OVERFLOW, Domain::Memory, Severity::Failure, "Stack exeeded it's maximum depth of 64"}};

constexpr Domain error_domain(Error err) noexcept {
        return static_cast<Domain>((err >> DOMAIN_SHIFT) & DOMAIN_MASK);
}

constexpr Severity error_severity(Error err) noexcept {
        return static_cast<Severity>(err & SEVERITY_MASK);
}

constexpr std::uint8_t error_code(Error err) noexcept {
        return static_cast<std::uint8_t>((err >> CODE_SHIFT) & CODE_MASK);
}

inline const Descriptor* find_descriptor(Error err) noexcept {
        for (const auto& descriptor : DESCRIPTORS) {
                if (descriptor.value == err) {
                        return &descriptor;
                }
        }
        return nullptr;
}

inline const char* error_message(Error err) noexcept {
        if (const auto* descriptor = find_descriptor(err)) {
                return descriptor->message;
        }

        switch (error_severity(err)) {
        case Severity::Fatal:
                return "Unknown invariant error";
        case Severity::Failure:
                return "Unknown runtime error";
        default:
                return "Unknown error";
        }
}

[[noreturn]] ANVIL_ATTR_COLD ANVIL_ATTR_NOINLINE void abort_invariant(const char* expr, const char* file, int line,
                                                                      Error err, const char* fmt, ...);

[[nodiscard]] ANVIL_ATTR_HOT ANVIL_ATTR_ALWAYS_INLINE inline bool is_error(Error err) noexcept {
        if (err != ERR_SUCCESS) [[unlikely]] {
                return true;
        }
        return false;
}

template <typename Condition, typename... Args>
ANVIL_ATTR_HOT ANVIL_ATTR_ALWAYS_INLINE inline void invariant(const char* expr, const char* file, int line,
                                                              Condition&& condition, Error err, Args&&... args) {
        if (static_cast<bool>(condition)) [[likely]] {
                return;
        }

        if constexpr (sizeof...(Args) == 0) {
                abort_invariant(expr, file, line, err, nullptr);
        } else {
                abort_invariant(expr, file, line, err, std::forward<Args>(args)...);
        }
}

[[nodiscard]] ANVIL_ATTR_HOT ANVIL_ATTR_ALWAYS_INLINE inline Error check(bool condition, Error err) noexcept {
        if (condition) [[likely]] {
                return ERR_SUCCESS;
        }
        return err;
}

template <typename Pointer>
[[nodiscard]] ANVIL_ATTR_HOT ANVIL_ATTR_ALWAYS_INLINE inline Error check_not_null(Pointer* ptr) noexcept {
        return check(ptr != nullptr, ERR_OUT_OF_MEMORY);
}

} // namespace anvil::error

using Error           = anvil::error::Error;
using ErrorDomain     = anvil::error::Domain;
using ErrorSeverity   = anvil::error::Severity;
using ErrorDescriptor = anvil::error::Descriptor;

using anvil::error::ERR_MEMORY_DEALLOCATION;
using anvil::error::ERR_MEMORY_PERMISSION_CHANGE;
using anvil::error::ERR_OUT_OF_MEMORY;
using anvil::error::ERR_STACK_OVERFLOW;
using anvil::error::ERR_SUCCESS;
using anvil::error::INV_BAD_ALIGNMENT;
using anvil::error::INV_INVALID_STATE;
using anvil::error::INV_NULL_POINTER;
using anvil::error::INV_OUT_OF_RANGE;
using anvil::error::INV_PRECONDITION;
using anvil::error::INV_ZERO_SIZE;

static inline constexpr ErrorDomain   ERR_DOMAIN_MEMORY  = ErrorDomain::Memory;
static inline constexpr ErrorDomain   ERR_DOMAIN_STATE   = ErrorDomain::State;
static inline constexpr ErrorDomain   ERR_DOMAIN_VALUE   = ErrorDomain::Value;

static inline constexpr ErrorSeverity ERR_SEVERITY_ERROR = ErrorSeverity::Failure;
static inline constexpr ErrorSeverity ERR_SEVERITY_FATAL = ErrorSeverity::Fatal;

static inline constexpr std::uint16_t ERR_DOMAIN_MAX     = 16;
static_assert(static_cast<std::uint16_t>(ERR_DOMAIN_MAX) <= 16, "Domain count exceeds 4-bit limit");

static inline constexpr ErrorDomain anvil_error_domain(Error err) noexcept {
        return anvil::error::error_domain(err);
}

static inline constexpr ErrorSeverity anvil_error_severity(Error err) noexcept {
        return anvil::error::error_severity(err);
}

static inline constexpr std::uint8_t anvil_error_code(Error err) noexcept {
        return anvil::error::error_code(err);
}

ANVIL_ATTR_COLD ANVIL_ATTR_NOINLINE static inline const char* anvil_error_message(Error err) noexcept {
        return anvil::error::error_message(err);
}

#define ANVIL_INVARIANT(expr, err, ...)                                                                                \
        ::anvil::error::invariant(#expr, __FILE__, __LINE__, (expr), (err), ##__VA_ARGS__)

#define ANVIL_INVARIANT_NOT_NULL(ptr)                                                                                  \
        ::anvil::error::invariant(#ptr, __FILE__, __LINE__, (ptr) != nullptr, ::anvil::error::INV_NULL_POINTER, "%s",  \
                                  #ptr)

#define ANVIL_INVARIANT_POSITIVE(val)                                                                                  \
        ::anvil::error::invariant(#val, __FILE__, __LINE__, (val) > 0, ::anvil::error::INV_ZERO_SIZE, "%s = %zu",      \
                                  #val, static_cast<std::size_t>(val))

#define ANVIL_INVARIANT_RANGE(val, min, max)                                                                           \
        ::anvil::error::invariant(#val, __FILE__, __LINE__, ((val) >= (min) && (val) <= (max)),                        \
                                  ::anvil::error::INV_OUT_OF_RANGE, "%s = %lld not in [%lld, %lld]", #val,             \
                                  static_cast<long long>(val), static_cast<long long>(min),                            \
                                  static_cast<long long>(max))

#endif // ANVIL_ERROR_HPP