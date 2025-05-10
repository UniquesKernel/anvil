/**
 * @file error.h
 * @brief Error message templates for the Anvil Memory system.
 *
 * This header defines standardized error messages and codes used throughout
 * the Anvil Memory library. The error codes follow a structured format (EXYZ)
 * where X represents the category, Y the subcategory, and Z the specific error.
 *
 * Categories:
 * - E1xx: Memory allocation errors
 * - E2xx: Pointer validation errors
 * - E3xx: Value range errors
 * - E4xx: State errors
 *
 * These error templates are used with the INVARIANT macro to provide
 * consistent and informative error messages when invariants are violated.
 */

#ifndef ERROR_H
#define ERROR_H

// Error code groups
// Format: EXYZ where:
// X: Category (1=Memory, 2=Pointer, 3=Value, 4=State)
// Y: Subcategory
// Z: Specific error

// Memory allocation errors (E1xx)
#define E101                                 "E101"    // Size zero
#define E102                                 "E102"    // Alignment not power of two
#define E103                                 "E103"    // Alignment too large
#define E104                                 "E104"    // Out of memory

#define ERR_ALLOC_SIZE_ZERO                  E101 ": Size must be positive but was 0"
#define ERR_ALLOC_ALIGNMENT_NOT_POWER_OF_TWO E102 ": Alignment must be a power of two but was %zu"
#define ERR_ALLOC_ALIGNMENT_TOO_LARGE        E103 ": Alignment must be <= %zu but was %zu"
#define ERR_OUT_OF_MEMORY                    E104 ": Failed to allocate %zu bytes (system out of memory)"

// Pointer validation errors (E2xx)
#define E201                                "E201"    // Null pointer
#define E202                                "E202"    // Null memory block
#define E203                                "E203"    // Null memory pointer

#define ERR_NULL_POINTER                    E201 ": %s pointer must not be NULL"
#define ERR_NULL_MEMORY_BLOCK               E202 ": Memory block pointer must not be NULL"
#define ERR_NULL_MEMORY                     E203 ": Memory pointer must not be NULL"

// Value range errors (E3xx)
#define E301                                "E301"    // Value out of range
#define E302                                "E302"    // Value below minimum
#define E303                                "E303"    // Value above maximum

#define ERR_VALUE_RANGE                     E301 ": Value %s must be between %d and %d but was %d"
#define ERR_VALUE_MIN                       E302 ": Value %s must be >= %d but was %d"
#define ERR_VALUE_MAX                       E303 ": Value %s must be <= %d but was %d"

// State errors (E4xx)
#define E401                                "E401"    // Invalid allocator type
#define E402                                "E402"    // Zero capacity
#define E403                                "E403"    // Alignment too small
#define E404                                "E404"    // Allocation too large

#define ERR_INVALID_ALLOCATOR_TYPE          E401 ": Allocator type must be < %d but was %d"
#define ERR_ZERO_CAPACITY                   E402 ": Arena capacity must be > 0 but was %zu"
#define ERR_ALIGNMENT_TOO_SMALL             E403 ": Alignment must be >= %zu but was %zu"
#define ERR_ALLOCATION_TOO_LARGE            E404 ": Allocation size %zu exceeds available capacity %zu"

#define E411                                "E411"    // Less than comparison failed
#define E412                                "E412"    // Less equal comparison failed
#define E413                                "E413"    // Greater than comparison failed
#define E414                                "E414"    // Greater equal comparison failed
#define E415                                "E415"    // Equality comparison failed
#define E416                                "E416"    // Inequality comparison failed

#define ERR_LESS_THAN                       E411 ": %s must be < %s: %zu not < %zu"
#define ERR_LESS_EQUAL                      E412 ": %s must be <= %s: %zu not <= %zu"
#define ERR_GREATER_THAN                    E413 ": %s must be > %s: %zu not > %zu"
#define ERR_GREATER_EQUAL                   E414 ": %s must be >= %s: %zu not >= %zu"
#define ERR_EQUAL                           E415 ": %s must == %s: %zu != %zu"
#define ERR_NOT_EQUAL                       E416 ": %s must != %s: %zu == %zu"

#define E417                                "E417"    // Invalid state
#define E418                                "E418"    // Operation invalid for state

#define ERR_INVALID_STATE                   E417 ": %s in invalid state: expected %s but was %s"
#define ERR_OPERATION_INVALID_FOR_STATE     E418 ": Operation %s not valid for %s in state %s"

// Example declaration to satisfy ISO C requirements
void error_dummy_function(void);

#endif    // ERROR_H
