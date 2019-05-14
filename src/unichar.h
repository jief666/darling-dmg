#ifndef UNICHAR_H
#define UNICHAR_H
#include <string>
#include "hfsplus.h"
#include "be.h"
#include <unicode/unistr.h>

icu_64::UnicodeString Utf8ToUnicodeString(const std::string& str2);

std::string UnicharToString(uint16_t length, const unichar* string);
bool EqualNoCase(const HFSString& str1, const std::string& str2);
bool EqualCase(const HFSString& str1, const std::string& str2);
uint16_t StringToUnichar(const std::string& in, unichar* out, size_t maxLength /* in unichars */);

inline std::string UnicharToString(const HFSString& str) { return UnicharToString(be(str.length), str.string); }

#endif
