// SafeString.h — defensive string handling for untrusted act (ППО) text.
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace bsc::text {

inline constexpr std::size_t kDefaultFieldLimit = 256;

// safeSet: assigns a sanitized copy of src to dst.
//  * nullptr or failure (e.g. std::bad_alloc) -> returns false, dst is left unchanged;
//  * reads at most maxBytes bytes, so a missing '\0' terminator cannot overrun;
//  * invalid UTF-8 bytes become '?', control characters become spaces,
//    the result is trimmed and never cuts a UTF-8 sequence in half.
bool safeSet(std::string& dst, const char* src, std::size_t maxBytes = kDefaultFieldLimit) noexcept;
bool safeSet(std::string& dst, std::string_view src, std::size_t maxBytes = kDefaultFieldLimit) noexcept;

// Same rules as safeSet, returns the sanitized string.
std::string sanitize(std::string_view src, std::size_t maxBytes);

// Length of the valid UTF-8 sequence starting at text[pos], 0 if invalid.
std::size_t utf8SequenceLength(std::string_view text, std::size_t pos) noexcept;

// Lower case for ASCII and Russian letters, 'ё' folded to 'е'. Byte length is
// preserved, so offsets in the folded string are valid in the original one.
std::string foldCase(std::string_view text);

std::string_view trim(std::string_view text) noexcept;

// True if no letter or digit directly precedes text[pos] (start of a word).
bool startsWord(std::string_view text, std::size_t pos) noexcept;

// Folded haystack contains folded needle as a whole word (not inside another word).
bool containsWord(std::string_view foldedHaystack, std::string_view foldedNeedle) noexcept;

// First number in the text: "+36,100 м" -> 36.1, "H=38м" -> 38.
std::optional<double> parseNumber(std::string_view text) noexcept;

// Locale independent decimal formatting without trailing zeros: 36.10 -> "36.1".
std::string formatNumber(double value, int precision = 2);

}  // namespace bsc::text
