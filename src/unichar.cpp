#include "unichar.h"
#include <iconv.h>
#include <unicode/unistr.h>
#include <unicode/ucnv.h>
#include <unicode/errorcode.h>
#include <cassert>
#include <iostream>
using icu::UnicodeString;
UConverter* g_utf16be;
UConverter* g_utf8;

#ifdef _MSC_VER
static void InitConverter();
class toto {
public:
    toto() { InitConverter();}
};
toto totoo;
#else
static void InitConverter() __attribute__((constructor));
#endif



static void ExitConverter()
#ifndef _MSC_VER
            __attribute__((destructor))
#endif
;

UnicodeString Utf8ToUnicodeString(const std::string& str2)
{
    UErrorCode error = U_ZERO_ERROR;
    UnicodeString ustr2 = UnicodeString((char*)str2.data(), str2.length(), g_utf8, error);
    assert(U_SUCCESS(error));
    return ustr2;
}

std::string UnicharToString(uint16_t length, const unichar* string)
{
	std::string result;
	UErrorCode error = U_ZERO_ERROR;

    if ( !g_utf16be ) InitConverter();
	UnicodeString str((char*) string, length*2, g_utf16be, error);
	
	assert(U_SUCCESS(error));
	str.toUTF8String(result);

	return result;
}

bool EqualNoCase(const HFSString& hfsStr1, const std::string& str2)
{
    if (!g_utf16be) InitConverter();
    UErrorCode error = U_ZERO_ERROR;
//	UnicodeString ustr2 = UnicodeString::fromUTF8(str2); // this way doesn't work on Windows
    UnicodeString ustr2 = UnicodeString((char*)str2.data(), str2.length(), g_utf8, error);
    assert(U_SUCCESS(error));
//std::string str2_2;
//ustr2_2.toUTF8String(str2_2);
//
//std::string ustr2_;
//ustr2.toUTF8String(ustr2_);
//char buf[100];
//ustr2.extract(0, 8, buf);
	UnicodeString hfsUStr = UnicodeString((char*)hfsStr1.string, be(hfsStr1.length)*2, g_utf16be, error);
//std::string hfsUStr_;
//hfsUStr.toUTF8String(hfsUStr_);

	assert(U_SUCCESS(error));
	
	return ustr2.caseCompare(hfsUStr, 0) == 0;
}

bool EqualCase(const HFSString& hfsStr1, const std::string& str2)
{
    if (!g_utf16be) InitConverter();
    UErrorCode error = U_ZERO_ERROR;
//    UnicodeString ustr2 = UnicodeString::fromUTF8(str2);
    UnicodeString ustr2 = UnicodeString((char*)str2.data(), str2.length(), g_utf8, error);
    assert(U_SUCCESS(error));
	UnicodeString ustr1 = UnicodeString((char*)hfsStr1.string, be(hfsStr1.length)*2, g_utf16be, error);
	assert(U_SUCCESS(error));
	
	return ustr2 == ustr1;
}

uint16_t StringToUnichar(const std::string& in, unichar* out, size_t maxLength)
{
    if (!g_utf16be) InitConverter();
    UErrorCode error = U_ZERO_ERROR;
//    UnicodeString str = UnicodeString::fromUTF8(in);
    UnicodeString str = UnicodeString((char*)in.data(), in.length(), g_utf8, error);
    assert(U_SUCCESS(error));
	auto bytes = str.extract((char*) out, maxLength*sizeof(unichar), g_utf16be, error);
	
	assert(U_SUCCESS(error));
	
	return bytes / sizeof(unichar);
}

void InitConverter()
{
	UErrorCode error = U_ZERO_ERROR;
    g_utf16be = ucnv_open("UTF-16BE", &error);
    assert(U_SUCCESS(error));
    g_utf8 = ucnv_open("UTF-8", &error);
	assert(U_SUCCESS(error));
}

void ExitConverter()
{
	ucnv_close(g_utf16be);
}
