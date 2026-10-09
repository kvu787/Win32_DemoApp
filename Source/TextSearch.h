#pragma once
#include <optional>
#include <string>
#include <string_view>
namespace TextSearch
{
struct Options
{
    bool down = true, matchCase = false, wholeWord = false, wrap = true;
};
struct Match
{
    size_t start = 0, length = 0;
    bool wrapped = false;
};
bool MatchesAt(std::wstring_view text, std::wstring_view needle, size_t at, const Options &options);
std::optional<Match> Find(std::wstring_view text, std::wstring_view needle, size_t selectionStart,
                          size_t selectionEnd, const Options &options);
size_t ReplaceAll(std::wstring &text, std::wstring_view needle, std::wstring_view replacement,
                  const Options &options);
} // namespace TextSearch
