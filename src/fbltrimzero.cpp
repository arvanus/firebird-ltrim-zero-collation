/*
 *	PROGRAM:	Firebird International support
 *	MODULE:		fbltrimzero.cpp
 *	DESCRIPTION:	Custom collation: LTRIM zeros and spaces, case-insensitive
 *
 *  This collation removes leading zeros and spaces before comparison
 *  Example: "00000A" = "0A" = "A" = "    A" = "a"
 */

#include <string.h>
#include <new>

// =============================================================================
// Platform-specific exports
// =============================================================================
#if defined(_WIN32) || defined(_WIN64)
    #define EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
    #define EXPORT __attribute__((visibility("default")))
#else
    #define EXPORT
#endif

// =============================================================================
// Type definitions matching Firebird's expectations EXACTLY
// =============================================================================
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef short SSHORT;
typedef unsigned long ULONG;
typedef const char ASCII;
typedef unsigned char BYTE;

// CRITICAL FIX: INTL_BOOL must be USHORT, not unsigned char!
// From Firebird src/common/intlobj_new.h:46
typedef USHORT INTL_BOOL;

#define TEXTTYPE_VERSION_1 1
#define INTL_BAD_STR_LENGTH ((ULONG) -1)
#define INTL_BAD_KEY_LENGTH ((USHORT) -1)
#define CC_INTL 255

// =============================================================================
// Forward declarations and function pointers
// =============================================================================
struct texttype;

typedef USHORT (*pfn_INTL_keylength)(texttype* tt, USHORT len);
typedef USHORT (*pfn_INTL_str2key)(texttype* tt, USHORT srcLen, const UCHAR* src,
                                    USHORT dstLen, UCHAR* dst, USHORT key_type);
typedef SSHORT (*pfn_INTL_compare)(texttype* tt, ULONG len1, const UCHAR* str1,
                                    ULONG len2, const UCHAR* str2, INTL_BOOL* error_flag);
typedef ULONG (*pfn_INTL_str2case)(texttype* tt, ULONG srcLen, const UCHAR* src,
                                    ULONG dstLen, UCHAR* dst);
typedef ULONG (*pfn_INTL_canonical)(texttype* t, ULONG srcLen, const UCHAR* src,
                                     ULONG dstLen, UCHAR* dst);
typedef void (*pfn_INTL_tt_destroy)(texttype* tt);

// texttype structure matching Firebird's layout
struct texttype
{
	USHORT texttype_version;
	void* texttype_impl;
	const ASCII* texttype_name;
	SSHORT texttype_country;
	BYTE texttype_canonical_width;
	USHORT texttype_flags;
	INTL_BOOL texttype_pad_option;

	pfn_INTL_keylength texttype_fn_key_length;
	pfn_INTL_str2key texttype_fn_string_to_key;
	pfn_INTL_compare texttype_fn_compare;
	pfn_INTL_str2case texttype_fn_str_to_upper;
	pfn_INTL_str2case texttype_fn_str_to_lower;
	pfn_INTL_canonical texttype_fn_canonical;
	pfn_INTL_tt_destroy texttype_fn_destroy;

	void* reserved_for_interface[5];
	void* reserved_for_driver[10];
};

// =============================================================================
// Constants
// =============================================================================
const ULONG MAX_STRING_LENGTH = 32000;  // Safety limit
const ULONG STACK_BUFFER_SIZE = 2048;   // Use stack for small strings

// =============================================================================
// Locale-independent character conversion
// =============================================================================

/**
 * Locale-independent uppercase conversion (ASCII only)
 * CRITICAL: Standard toupper() is locale-dependent!
 */
static inline UCHAR ascii_toupper(UCHAR c)
{
	return (c >= 'a' && c <= 'z') ? (c - 32) : c;
}

/**
 * Locale-independent lowercase conversion (ASCII only)
 */
static inline UCHAR ascii_tolower(UCHAR c)
{
	return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}

// =============================================================================
// String Normalization Algorithm
// =============================================================================

/**
 * Normalize string: remove leading zeros/spaces and convert to uppercase
 *
 * CRITICAL FIXES:
 * - Added max_output_len parameter for bounds checking
 * - Returns actual length, not via output parameter
 * - Uses locale-independent conversion
 * - Validates all inputs
 *
 * @param input          Input string
 * @param input_len      Input length in bytes
 * @param output         Output buffer
 * @param max_output_len Maximum output buffer size
 * @return               Actual output length, or 0 on error
 */
static ULONG normalize_string(const UCHAR* input, ULONG input_len,
                              UCHAR* output, ULONG max_output_len)
{
	if (!input || !output || input_len == 0 || max_output_len == 0)
		return 0;

	const UCHAR* p = input;
	const UCHAR* end = input + input_len;
	UCHAR* out = output;
	const UCHAR* out_end = output + max_output_len;

	// Skip leading zeros and spaces (not tabs - tabs removed per reviewer comment)
	while (p < end && (*p == '0' || *p == ' ')) {
		p++;
	}

	// If entire string was zeros/spaces, keep the last character
	if (p >= end) {
		if (out < out_end) {
			// Safely get last character
			*out++ = ascii_toupper(input[input_len - 1]);
			return 1;
		}
		return 0;
	}

	// Copy remaining characters in uppercase with bounds checking
	while (p < end && out < out_end) {
		*out++ = ascii_toupper(*p);
		p++;
	}

	return (ULONG)(out - output);
}

// =============================================================================
// Collation Functions
// =============================================================================

/**
 * Compare two strings according to LTRIM_ZERO rules
 *
 * FIXES:
 * - Added NULL pointer checks
 * - Added length validation
 * - Uses safe heap allocation instead of huge stack buffers
 * - Proper error handling
 */
static SSHORT compare_function(texttype* obj,
                               ULONG len1, const UCHAR* str1,
                               ULONG len2, const UCHAR* str2,
                               INTL_BOOL* error_flag)
{
	// Validate inputs
	if (!error_flag)
		return 0;

	*error_flag = 0;

	if (!str1 || !str2) {
		*error_flag = 1;
		return 0;
	}

	// Validate lengths
	if (len1 > MAX_STRING_LENGTH || len2 > MAX_STRING_LENGTH) {
		*error_flag = 1;
		return 0;
	}

	// Use stack for small strings, heap for large
	UCHAR stack_norm1[STACK_BUFFER_SIZE];
	UCHAR stack_norm2[STACK_BUFFER_SIZE];

	UCHAR* norm1 = (len1 <= STACK_BUFFER_SIZE) ? stack_norm1 : new (std::nothrow) UCHAR[len1];
	UCHAR* norm2 = (len2 <= STACK_BUFFER_SIZE) ? stack_norm2 : new (std::nothrow) UCHAR[len2];

	if (!norm1 || !norm2) {
		if (norm1 && norm1 != stack_norm1) delete[] norm1;
		if (norm2 && norm2 != stack_norm2) delete[] norm2;
		*error_flag = 1;
		return 0;
	}

	// Normalize both strings
	ULONG norm_len1 = normalize_string(str1, len1, norm1, len1);
	ULONG norm_len2 = normalize_string(str2, len2, norm2, len2);

	// Compare up to minimum length
	const ULONG min_len = (norm_len1 < norm_len2) ? norm_len1 : norm_len2;
	int result = 0;

	if (min_len > 0) {
		result = memcmp(norm1, norm2, min_len);
	}

	// Cleanup heap allocations
	if (norm1 != stack_norm1) delete[] norm1;
	if (norm2 != stack_norm2) delete[] norm2;

	// Return comparison result
	if (result != 0)
		return (result < 0) ? -1 : 1;

	// If prefixes are equal, shorter string comes first
	if (norm_len1 < norm_len2)
		return -1;
	if (norm_len1 > norm_len2)
		return 1;

	return 0;
}

/**
 * Generate sort key for index
 *
 * FIXES:
 * - Safe buffer allocation
 * - Proper error handling
 * - Returns INTL_BAD_KEY_LENGTH if key too long
 */
static USHORT str_to_key_function(texttype* obj,
                                  USHORT srcLen, const UCHAR* src,
                                  USHORT dstLen, UCHAR* dst,
                                  USHORT key_type)
{
	if (!src || !dst || srcLen == 0 || dstLen == 0)
		return 0;

	if (srcLen > MAX_STRING_LENGTH)
		return INTL_BAD_KEY_LENGTH;

	// Use stack buffer for reasonable sizes, heap for large
	UCHAR stack_buf[STACK_BUFFER_SIZE];
	UCHAR* normalized = (srcLen <= STACK_BUFFER_SIZE) ? stack_buf : new (std::nothrow) UCHAR[srcLen];

	if (!normalized)
		return INTL_BAD_KEY_LENGTH;

	// Normalize the string
	ULONG norm_len = normalize_string(src, srcLen, normalized, srcLen);

	// Check if key fits in destination
	if (norm_len > dstLen) {
		if (normalized != stack_buf)
			delete[] normalized;
		return INTL_BAD_KEY_LENGTH;
	}

	// Copy to destination buffer
	if (norm_len > 0)
		memcpy(dst, normalized, norm_len);

	// Pad with zeros if needed
	if (norm_len < dstLen)
		memset(dst + norm_len, 0, dstLen - norm_len);

	// Cleanup
	if (normalized != stack_buf)
		delete[] normalized;

	return (USHORT)norm_len;
}

/**
 * Calculate maximum key length for given string length
 */
static USHORT key_length_function(texttype* obj, USHORT len)
{
	// Worst case: no leading zeros/spaces removed
	return len;
}

/**
 * Convert string to uppercase
 */
static ULONG to_upper_function(texttype* obj,
                               ULONG srcLen, const UCHAR* src,
                               ULONG dstLen, UCHAR* dst)
{
	if (!src || !dst)
		return 0;

	const ULONG copy_len = (srcLen < dstLen) ? srcLen : dstLen;

	for (ULONG i = 0; i < copy_len; i++)
		dst[i] = ascii_toupper(src[i]);

	return copy_len;
}

/**
 * Convert string to lowercase
 */
static ULONG to_lower_function(texttype* obj,
                               ULONG srcLen, const UCHAR* src,
                               ULONG dstLen, UCHAR* dst)
{
	if (!src || !dst)
		return 0;

	const ULONG copy_len = (srcLen < dstLen) ? srcLen : dstLen;

	for (ULONG i = 0; i < copy_len; i++)
		dst[i] = ascii_tolower(src[i]);

	return copy_len;
}

/**
 * Destroy function - cleanup allocated resources
 */
static void destroy_function(texttype* tt)
{
	if (!tt)
		return;

	if (tt->texttype_impl) {
		// Free implementation-specific data if any (currently unused)
		tt->texttype_impl = nullptr;
	}

	if (tt->texttype_name) {
		delete[] const_cast<char*>(tt->texttype_name);
		tt->texttype_name = nullptr;
	}
}

// =============================================================================
// Firebird Entry Point
// =============================================================================

extern "C" {

/**
 * Main initialization function for Firebird
 *
 * CRITICAL FIX: Return type must be INTL_BOOL, not ULONG!
 * From Firebird src/common/intlobj_new.h:346
 *
 * @param tt                        Texttype structure to initialize
 * @param name                      Collation name (e.g., "WIN1252_LTRIM_ZERO")
 * @param charSetName               Character set name (e.g., "WIN1252")
 * @param attributes                Collation attributes
 * @param specificAttributes        Specific attributes (optional)
 * @param specificAttributesLength  Length of specific attributes
 * @param ignoreAttributes          Whether to ignore attributes
 * @param configInfo                Configuration info from .conf file
 * @return                          TRUE (1) = success, FALSE (0) = failure
 */
EXPORT INTL_BOOL LD_lookup_texttype(
	texttype* tt,
	const ASCII* name,
	const ASCII* charSetName,
	USHORT attributes,
	const UCHAR* specificAttributes,
	ULONG specificAttributesLength,
	ULONG ignoreAttributes,
	const ASCII* configInfo)
{
	// Validate inputs
	if (!tt || !name)
		return 0;  // FALSE = error

	try {
		// Allocate and copy name for debugging
		size_t name_len = strlen(name);
		char* name_copy = new char[name_len + 1];
		memcpy(name_copy, name, name_len + 1);

		// Clear entire structure first (important!)
		memset(tt, 0, sizeof(texttype));

		// Initialize texttype structure
		tt->texttype_version = TEXTTYPE_VERSION_1;
		tt->texttype_impl = nullptr;
		tt->texttype_name = name_copy;
		tt->texttype_country = CC_INTL;
		tt->texttype_canonical_width = 1;  // Single-byte charset
		tt->texttype_flags = 0;
		tt->texttype_pad_option = 1;  // PAD SPACE

		// Set function pointers
		tt->texttype_fn_key_length = key_length_function;
		tt->texttype_fn_string_to_key = str_to_key_function;
		tt->texttype_fn_compare = compare_function;
		tt->texttype_fn_str_to_upper = to_upper_function;
		tt->texttype_fn_str_to_lower = to_lower_function;
		tt->texttype_fn_canonical = nullptr;  // Use default
		tt->texttype_fn_destroy = destroy_function;

		return 1;  // TRUE = success
	}
	catch (const std::bad_alloc&) {
		return 0;  // Failed to allocate
	}
	catch (...) {
		return 0;  // Unknown error
	}
}

} // extern "C"
