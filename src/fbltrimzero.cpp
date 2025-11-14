/*
 *	PROGRAM:	Firebird International support
 *	MODULE:		fbltrimzero_fixed.cpp
 *	DESCRIPTION:	Custom collation: LTRIM zeros and spaces, case-insensitive
 *
 *  This collation removes leading zeros and spaces before comparison
 *  Example: "00000A" = "0A" = "A" = "    A" = "a"
 */

// Usar os headers reais do Firebird
#define TEXTTYPE_VERSION_1 1
#define INTL_BAD_STR_LENGTH ((unsigned long) -1)
#define INTL_BAD_KEY_LENGTH ((unsigned short) -1)

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef short SSHORT;
typedef unsigned long ULONG;
typedef const char ASCII;
typedef unsigned char INTL_BOOL;
typedef unsigned char BYTE;

// Forward declarations
struct texttype;

// Function pointer types matching Firebird's expectations
typedef USHORT (*pfn_INTL_keylength)(texttype* tt, USHORT len);
typedef USHORT (*pfn_INTL_str2key)(texttype* tt, USHORT srcLen, const UCHAR* src, USHORT dstLen, UCHAR* dst, USHORT key_type);
typedef SSHORT (*pfn_INTL_compare)(texttype* tt, ULONG len1, const UCHAR* str1, ULONG len2, const UCHAR* str2, INTL_BOOL* error_flag);
typedef ULONG (*pfn_INTL_str2case)(texttype* tt, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst);
typedef ULONG (*pfn_INTL_canonical)(texttype* t, ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst);
typedef void (*pfn_INTL_tt_destroy)(texttype* tt);

// Correct texttype structure matching Firebird's layout
struct texttype
{
	// Data which needs to be initialized by collation driver
	USHORT texttype_version;	// version ID of object
	void* texttype_impl;		// collation object implemented in driver

	const ASCII* texttype_name;	// debugging name

	SSHORT texttype_country;	// ID of base country values
	BYTE texttype_canonical_width;  // number bytes in canonical character representation

	USHORT texttype_flags; // Misc texttype flags filled by driver

	INTL_BOOL texttype_pad_option; // pad with spaces for comparison

	pfn_INTL_keylength texttype_fn_key_length;
	pfn_INTL_str2key texttype_fn_string_to_key;
	pfn_INTL_compare texttype_fn_compare;
	pfn_INTL_str2case texttype_fn_str_to_upper;
	pfn_INTL_str2case texttype_fn_str_to_lower;
	pfn_INTL_canonical texttype_fn_canonical;
	pfn_INTL_tt_destroy texttype_fn_destroy;

	// Reserved space
	void* reserved_for_interface[5];
	void* reserved_for_driver[10];
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

// Normalize string: remove leading zeros/spaces and convert to uppercase
static void normalize_string(const UCHAR* input, ULONG input_len,
                             UCHAR* output, ULONG* output_len)
{
	const UCHAR* p = input;
	const UCHAR* end = input + input_len;
	UCHAR* out = output;

	// Skip leading zeros and spaces
	while (p < end && (*p == '0' || *p == ' ' || *p == '\t')) {
		p++;
	}

	// If entire string was zeros/spaces, keep last character
	if (p >= end && input_len > 0) {
		p = input + input_len - 1;
		*out++ = (UCHAR)toupper(*p);
		*output_len = 1;
		return;
	}

	// Copy rest in uppercase
	while (p < end) {
		*out++ = (UCHAR)toupper(*p);
		p++;
	}

	*output_len = (ULONG)(out - output);
}

// Compare function - NEW SIGNATURE with error_flag
static SSHORT compare_function(texttype* obj,
                               ULONG len1, const UCHAR* str1,
                               ULONG len2, const UCHAR* str2,
                               INTL_BOOL* error_flag)
{
	UCHAR norm1[32767];
	UCHAR norm2[32767];
	ULONG norm_len1, norm_len2;

	*error_flag = 0; // No error

	normalize_string(str1, len1, norm1, &norm_len1);
	normalize_string(str2, len2, norm2, &norm_len2);

	const ULONG min_len = (norm_len1 < norm_len2) ? norm_len1 : norm_len2;

	if (min_len > 0) {
		const int result = memcmp(norm1, norm2, min_len);
		if (result != 0)
			return (result < 0) ? -1 : 1;
	}

	if (norm_len1 < norm_len2)
		return -1;
	if (norm_len1 > norm_len2)
		return 1;

	return 0;
}

// Key generation for index
static USHORT str_to_key_function(texttype* obj,
                                  USHORT srcLen, const UCHAR* src,
                                  USHORT dstLen, UCHAR* dst,
                                  USHORT key_type)
{
	UCHAR normalized[32767];
	ULONG norm_len;

	normalize_string(src, srcLen, normalized, &norm_len);

	const USHORT copy_len = (norm_len < dstLen) ? (USHORT)norm_len : dstLen;

	if (copy_len > 0)
		memcpy(dst, normalized, copy_len);

	// Pad with zeros if needed
	if (copy_len < dstLen)
		memset(dst + copy_len, 0, dstLen - copy_len);

	return copy_len;
}

// Calculate key length
static USHORT key_length_function(texttype* obj, USHORT len)
{
	return len; // Worst case: no leading zeros/spaces removed
}

// Uppercase function - NEW SIGNATURE with ULONG
static ULONG to_upper_function(texttype* obj,
                               ULONG srcLen, const UCHAR* src,
                               ULONG dstLen, UCHAR* dst)
{
	const ULONG copy_len = (srcLen < dstLen) ? srcLen : dstLen;

	for (ULONG i = 0; i < copy_len; i++)
		dst[i] = (UCHAR)toupper(src[i]);

	return copy_len;
}

// Lowercase function - NEW SIGNATURE with ULONG
static ULONG to_lower_function(texttype* obj,
                               ULONG srcLen, const UCHAR* src,
                               ULONG dstLen, UCHAR* dst)
{
	const ULONG copy_len = (srcLen < dstLen) ? srcLen : dstLen;

	for (ULONG i = 0; i < copy_len; i++)
		dst[i] = (UCHAR)tolower(src[i]);

	return copy_len;
}

// Destroy function
static void destroy_function(texttype* tt)
{
	if (tt && tt->texttype_name) {
		delete[] const_cast<char*>(tt->texttype_name);
		tt->texttype_name = nullptr;
	}
}

// ============================================================================
// FIREBIRD ENTRY POINT
// ============================================================================

extern "C" {

/**
 * Main initialization function for Firebird
 * IMPORTANT: Function name MUST be LD_lookup_texttype
 */
EXPORT ULONG LD_lookup_texttype(
	texttype* tt,
	const ASCII* name,
	const ASCII* charSetName,
	USHORT attributes,
	const UCHAR* specificAttributes,
	ULONG specificAttributesLength,
	ULONG ignoreAttributes,
	const ASCII* configInfo)
{
	if (!tt || !name)
		return 0;  // FALSE = error

	// Allocate and copy name
	size_t name_len = strlen(name);
	char* name_copy = new char[name_len + 1];
	strcpy(name_copy, name);

	// Clear entire structure first
	memset(tt, 0, sizeof(texttype));

	// Configure texttype structure
	tt->texttype_version = TEXTTYPE_VERSION_1;
	tt->texttype_impl = nullptr;  // No implementation-specific data
	tt->texttype_name = name_copy;
	tt->texttype_country = 255;  // CC_INTL
	tt->texttype_canonical_width = 1;  // Single byte
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

} // extern "C"
