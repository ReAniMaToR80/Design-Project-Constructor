#include "SafeString.h"

#include <algorithm>
#include <charconv>
#include <exception>
#include <iomanip>
#include <locale>
#include <sstream>

namespace bsc::text {
namespace {

bool isContinuation(unsigned char c) noexcept {
    return (c & 0xC0U) == 0x80U;
}

// Expected sequence length by the lead byte, 0 for bytes that never start a sequence.
std::size_t expectedLength(unsigned char lead) noexcept {
    if (lead < 0x80U) return 1;
    if (lead >= 0xC2U && lead <= 0xDFU) return 2;
    if (lead >= 0xE0U && lead <= 0xEFU) return 3;
    if (lead >= 0xF0U && lead <= 0xF4U) return 4;
    return 0;
}

bool isAsciiAlnum(unsigned char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Lead bytes of the Cyrillic block U+0400..U+04FF.
bool isCyrillicLead(unsigned char c) noexcept {
    return c == 0xD0U || c == 0xD1U || c == 0xD2U || c == 0xD3U;
}

bool isWordLead(unsigned char c) noexcept {
    return c < 0x80U ? isAsciiAlnum(c) : isCyrillicLead(c);
}

bool wordCharBefore(std::string_view text, std::size_t pos) noexcept {
    if (pos == 0) {
        return false;
    }
    std::size_t j = pos - 1;
    while (j > 0 && isContinuation(static_cast<unsigned char>(text[j]))) {
        --j;
    }
    return isWordLead(static_cast<unsigned char>(text[j]));
}

bool wordCharAt(std::string_view text, std::size_t pos) noexcept {
    return pos < text.size() && isWordLead(static_cast<unsigned char>(text[pos]));
}

}  // namespace

std::size_t utf8SequenceLength(std::string_view text, std::size_t pos) noexcept {
    if (pos >= text.size()) {
        return 0;
    }
    const auto lead = static_cast<unsigned char>(text[pos]);
    const std::size_t length = expectedLength(lead);
    if (length == 0 || pos + length > text.size()) {
        return 0;
    }
    for (std::size_t k = 1; k < length; ++k) {
        if (!isContinuation(static_cast<unsigned char>(text[pos + k]))) {
            return 0;
        }
    }
    if (length >= 3) {
        const auto second = static_cast<unsigned char>(text[pos + 1]);
        const bool overlong = (lead == 0xE0U && second < 0xA0U) || (lead == 0xF0U && second < 0x90U);
        const bool surrogate = lead == 0xEDU && second >= 0xA0U;
        const bool tooLarge = lead == 0xF4U && second >= 0x90U;
        if (overlong || surrogate || tooLarge) {
            return 0;
        }
    }
    return length;
}

std::string sanitize(std::string_view src, std::size_t maxBytes) {
    std::string out;
    out.reserve(std::min(src.size(), maxBytes));
    std::size_t i = 0;
    while (i < src.size()) {
        const auto c = static_cast<unsigned char>(src[i]);
        if (c < 0x80U) {
            if (out.size() + 1 > maxBytes) break;
            out.push_back((c < 0x20U || c == 0x7FU) ? ' ' : static_cast<char>(c));
            ++i;
            continue;
        }
        const std::size_t length = utf8SequenceLength(src, i);
        if (length == 0) {
            const std::size_t expected = expectedLength(c);
            if (expected > 1 && i + expected > src.size()) {
                break;  // sequence truncated by the end of input: drop it
            }
            if (out.size() + 1 > maxBytes) break;
            out.push_back('?');
            ++i;
            continue;
        }
        if (out.size() + length > maxBytes) break;
        out.append(src.substr(i, length));
        i += length;
    }
    const std::string_view trimmed = trim(out);
    return std::string(trimmed);
}

bool safeSet(std::string& dst, std::string_view src, std::size_t maxBytes) noexcept {
    try {
        std::string value = sanitize(src, maxBytes);
        dst.swap(value);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool safeSet(std::string& dst, const char* src, std::size_t maxBytes) noexcept {
    if (src == nullptr) {
        return false;
    }
    std::size_t length = 0;
    while (length < maxBytes && src[length] != '\0') {
        ++length;
    }
    return safeSet(dst, std::string_view(src, length), maxBytes);
}

std::string foldCase(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        const bool hasNext = i + 1 < text.size();
        const auto next = hasNext ? static_cast<unsigned char>(text[i + 1]) : 0U;
        if (c >= 'A' && c <= 'Z') {
            out.push_back(static_cast<char>(c + ('a' - 'A')));
        } else if (c == 0xD0U && hasNext) {
            if (next >= 0x90U && next <= 0x9FU) {  // А..П -> а..п
                out.push_back(static_cast<char>(0xD0U));
                out.push_back(static_cast<char>(next + 0x20U));
            } else if (next >= 0xA0U && next <= 0xAFU) {  // Р..Я -> р..я
                out.push_back(static_cast<char>(0xD1U));
                out.push_back(static_cast<char>(next - 0x20U));
            } else if (next == 0x81U) {  // Ё -> е
                out.push_back(static_cast<char>(0xD0U));
                out.push_back(static_cast<char>(0xB5U));
            } else {
                out.push_back(static_cast<char>(c));
                out.push_back(static_cast<char>(next));
            }
            ++i;
        } else if (c == 0xD1U && next == 0x91U) {  // ё -> е
            out.push_back(static_cast<char>(0xD0U));
            out.push_back(static_cast<char>(0xB5U));
            ++i;
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

std::string_view trim(std::string_view text) noexcept {
    const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!text.empty() && isSpace(text.front())) text.remove_prefix(1);
    while (!text.empty() && isSpace(text.back())) text.remove_suffix(1);
    return text;
}

bool startsWord(std::string_view text, std::size_t pos) noexcept {
    return !wordCharBefore(text, pos);
}

bool containsWord(std::string_view foldedHaystack, std::string_view foldedNeedle) noexcept {
    if (foldedNeedle.empty()) {
        return false;
    }
    std::size_t pos = foldedHaystack.find(foldedNeedle);
    while (pos != std::string_view::npos) {
        if (!wordCharBefore(foldedHaystack, pos) && !wordCharAt(foldedHaystack, pos + foldedNeedle.size())) {
            return true;
        }
        pos = foldedHaystack.find(foldedNeedle, pos + 1);
    }
    return false;
}

std::optional<double> parseNumber(std::string_view text) noexcept {
    std::size_t start = 0;
    while (start < text.size() && !(text[start] >= '0' && text[start] <= '9')) {
        ++start;
    }
    if (start == text.size()) {
        return std::nullopt;
    }
    const bool negative = start > 0 && text[start - 1] == '-';
    char buffer[64] = {};
    std::size_t length = 0;
    bool seenSeparator = false;
    for (std::size_t i = start; i < text.size() && length + 1 < sizeof(buffer); ++i) {
        const char c = text[i];
        if (c >= '0' && c <= '9') {
            buffer[length++] = c;
        } else if ((c == '.' || c == ',') && !seenSeparator && i + 1 < text.size() && text[i + 1] >= '0' &&
                   text[i + 1] <= '9') {
            buffer[length++] = '.';
            seenSeparator = true;
        } else {
            break;
        }
    }
    double value = 0.0;
    const auto result = std::from_chars(buffer, buffer + length, value);
    if (result.ec != std::errc()) {
        return std::nullopt;
    }
    return negative ? -value : value;
}

std::string formatNumber(double value, int precision) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(precision) << value;
    std::string result = stream.str();
    if (result.find('.') != std::string::npos) {
        while (!result.empty() && result.back() == '0') result.pop_back();
        if (!result.empty() && result.back() == '.') result.pop_back();
    }
    return result == "-0" ? "0" : result;
}

}  // namespace bsc::text
