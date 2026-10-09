#include <windows.h>
#include "TextSearch.h"
#include <algorithm>
namespace TextSearch
{
namespace
{
bool WordCharacter(wchar_t value)
{
    WORD category = 0;
    return value == L'_' ||
           (GetStringTypeW(CT_CTYPE1, &value, 1, &category) && (category & (C1_ALPHA | C1_DIGIT)) != 0);
}
} // namespace
bool MatchesAt(std::wstring_view text, std::wstring_view needle, size_t at, const Options &options)
{
    if (needle.empty() || at > text.size() || needle.size() > text.size() - at)
        return false;
    if (CompareStringOrdinal(text.data() + at, int(needle.size()), needle.data(), int(needle.size()),
                             !options.matchCase) != CSTR_EQUAL)
        return false;
    return !options.wholeWord ||
           ((at == 0 || !WordCharacter(text[at - 1])) &&
            (at + needle.size() == text.size() || !WordCharacter(text[at + needle.size()])));
}
std::optional<Match> Find(std::wstring_view text, std::wstring_view needle, size_t selectionStart,
                          size_t selectionEnd, const Options &options)
{
    if (needle.empty() || needle.size() > text.size())
        return std::nullopt;
    selectionStart = std::min(selectionStart, text.size());
    selectionEnd = std::clamp(selectionEnd, selectionStart, text.size());
    const auto last = text.size() - needle.size();
    if (options.down)
    {
        for (size_t at = selectionEnd; at <= last; ++at)
            if (MatchesAt(text, needle, at, options))
                return Match{at, needle.size(), false};
        if (options.wrap)
            for (size_t at = 0; at < selectionEnd && at <= last; ++at)
                if (MatchesAt(text, needle, at, options))
                    return Match{at, needle.size(), true};
    }
    else
    {
        if (selectionStart >= needle.size())
            for (size_t at = std::min(selectionStart - needle.size(), last) + 1; at > 0;)
            {
                --at;
                if (MatchesAt(text, needle, at, options))
                    return Match{at, needle.size(), false};
            }
        if (options.wrap)
            for (size_t at = last + 1; at > 0;)
            {
                --at;
                if (at + needle.size() > selectionStart && MatchesAt(text, needle, at, options))
                    return Match{at, needle.size(), true};
            }
    }
    return std::nullopt;
}
size_t ReplaceAll(std::wstring &text, std::wstring_view needle, std::wstring_view replacement,
                  const Options &options)
{
    if (needle.empty())
        return 0;
    std::wstring result;
    size_t count = 0;
    for (size_t at = 0; at < text.size();)
    {
        if (MatchesAt(text, needle, at, options))
        {
            result.append(replacement);
            at += needle.size();
            ++count;
        }
        else
            result.push_back(text[at++]);
    }
    if (count)
        text = std::move(result);
    return count;
}
} // namespace TextSearch
