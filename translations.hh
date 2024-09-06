#ifndef __OICOMPARE_TRANSLATIONS_HH__
#define __OICOMPARE_TRANSLATIONS_HH__

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include "oicompare.hh"
#include "print_format.hh"

namespace oicompare::translations
{
enum class kind
{
  terse,
  abbreviated,
  full,
};

namespace detail
{
using namespace std::string_view_literals;

constexpr bool
is_ascii_printable (char ch)
{
  return ch >= 32 && ch <= 126 && ch != '"' && ch != '<' && ch != '>';
}

constexpr std::size_t
char_length (char ch)
{
  if (detail::is_ascii_printable (ch)) [[likely]]
    return 1;
  else
    return 6;
}

constexpr bool
string_length_fits (std::string_view str, std::size_t limit)
{
  std::size_t result = 0;
  for (char ch : str)
    {
      std::size_t new_result = result + char_length (ch);
      if (new_result > limit)
        return false;
      result = new_result;
    }
  return true;
}

template <std::output_iterator<char> OutputIt>
OutputIt
append_char (OutputIt out, char ch)
{
  if (detail::is_ascii_printable (ch)) [[likely]]
    *out++ = ch;
  else
    {
      constexpr std::array hex_table{'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

      *out++ = '<';
      *out++ = '0';
      *out++ = 'x';
      *out++ = hex_table[ch >> 4];
      *out++ = hex_table[ch & 0x0F];
      *out++ = '>';
    }

  return out;
}

constexpr std::size_t abbreviated_max = 100;
constexpr std::string_view ellipsis = "…"sv;
}

template <bool Abbreviated> struct represent_word;

template <> struct represent_word<false>
{
  template <std::output_iterator<char> OutputIt>
  static OutputIt
  represent (OutputIt out, std::string_view word, std::size_t)
  {
    *out++ = '"';
    for (char ch : word)
      out = detail::append_char (std::move (out), ch);
    *out++ = '"';

    return out;
  }
};

template <> struct represent_word<true>
{
  template <std::output_iterator<char> OutputIt>
  static OutputIt
  represent (OutputIt out, std::string_view word, std::size_t first_difference)
  {
    using namespace detail;
    using namespace std::string_view_literals;

    assert (!word.empty ());
    assert (first_difference <= word.size ());

    std::size_t used_chars
        = char_length ('"') + char_length (word[first_difference]);
    if (first_difference > 0)
      {
        used_chars += char_length (word[first_difference - 1]);
        if (first_difference > 1)
          {
            used_chars += char_length (word[0]);
            if (first_difference > 2)
              used_chars += ellipsis.size ();
          }
      }

    if (first_difference < word.size () - 1)
      {
        used_chars += char_length (word[first_difference + 1]);
        if (first_difference < word.size () - 2)
          used_chars += ellipsis.size ();
      }

    used_chars += char_length ('"');

    assert (used_chars <= abbreviated_max);

    auto append_char_helper
        = [&] (char ch) constexpr { out = append_char (std::move (out), ch); };

    *out++ = '"';

    if (first_difference > 0)
      {
        if (first_difference > 1)
          {
            append_char_helper (word[0]);

            /*
             * A simple greedy algorithm will not work, because:
             *
             *  XY (2 bytes)
             *
             * is shorter than
             *
             *  … (3 bytes)
             *
             * so instead of lookahead we will use a limited length check (so
             * the running time is limited).
             */
            auto ellipsized = word.substr (1, first_difference - 1);
            if (string_length_fits (ellipsized,
                                    abbreviated_max
                                        - (used_chars - ellipsis.size ())))
              {
                for (std::size_t i = 1; i < first_difference - 1; ++i)
                  {
                    used_chars += char_length (word[i]);
                    append_char_helper (word[i]);
                  }
                used_chars -= ellipsis.size ();
              }
            else
              {
                for (std::size_t i = 1; i < first_difference - 1; ++i)
                  {
                    std::size_t new_used_chars
                        = used_chars + char_length (word[i]);
                    if (new_used_chars > abbreviated_max)
                      break;
                    used_chars = new_used_chars;
                    append_char_helper (word[i]);
                  }
                out = std::move (
                    std::ranges::copy (ellipsis, std::move (out)).out);
              }
          }

        append_char_helper (word[first_difference - 1]);
      }

    if (first_difference < word.size ())
      {
        append_char_helper (word[first_difference]);
      }

    if (first_difference < word.size () - 1)
      {
        append_char_helper (word[first_difference + 1]);

        if (first_difference < word.size () - 2)
          {
            auto ellipsized = word.substr (first_difference + 2);
            if (string_length_fits (ellipsized,
                                    abbreviated_max
                                        - (used_chars - ellipsis.size ())))
              {
                for (std::size_t i = first_difference + 2; i < word.size ();
                     ++i)
                  {
                    used_chars += char_length (word[i]);
                    append_char_helper (word[i]);
                  }
                used_chars -= ellipsis.size ();
              }
            else
              {
                for (std::size_t i = first_difference + 2; i < word.size ();
                     ++i)
                  {
                    std::size_t new_used_chars
                        = used_chars + char_length (word[i]);
                    if (new_used_chars > abbreviated_max)
                      break;
                    used_chars = new_used_chars;
                    append_char_helper (word[i]);
                  }
                out = std::move (
                    std::ranges::copy (ellipsis, std::move (out)).out);
              }
          }
      }

    assert (used_chars <= abbreviated_max);

    *out++ = '"';

    return out;
  }
};

template <kind Kind> struct english_translation
{
  static auto
  represent (const oicompare::token<const char *> &token,
             const char *mismatch) noexcept
  {
    using namespace std::string_view_literals;

    return print_format ([&token, mismatch] (auto &ctx) {
      auto out = ctx.out ();
      switch (token.type)
        {
        case oicompare::token_type::eof:
          out = std::move (
              std::ranges::copy ("end of file"sv, std::move (out)).out);
          break;
        case oicompare::token_type::newline:
          out = std::move (
              std::ranges::copy ("end of line"sv, std::move (out)).out);
          break;
        case oicompare::token_type::word:
          assert (mismatch);
          represent_word<Kind == kind::abbreviated>::represent (
              out, {token.first, token.last},
              mismatch ? mismatch - token.first : 0);
          break;
        default:
#ifdef __GNUC__
          __builtin_unreachable ();
#endif
          out = std::move (
              std::ranges::copy ("unknown token"sv, std::move (out)).out);
          break;
        }
      return out;
    });
  }

  static void
  print (const std::optional<oicompare::mismatch<const char *, const char *>>
             &mismatch)
  {
    if (mismatch)
      switch (Kind)
        {
        case kind::terse:
          fmt::println ("WRONG");
          break;
        case kind::abbreviated:
        case kind::full:
          fmt::println ("WRONG: line {}: expected {}, got {}",
                        mismatch->line_number,
                        represent (mismatch->first,
                                   mismatch->first_difference.has_value ()
                                       ? mismatch->first_difference->first
                                       : nullptr),
                        represent (mismatch->second,
                                   mismatch->first_difference.has_value ()
                                       ? mismatch->first_difference->second
                                       : nullptr));
          break;
        }
    else
      fmt::println ("OK");
  }
};

template <kind Kind> struct polish_translation
{
  static auto
  represent (const oicompare::token<const char *> &token,
             const char *mismatch) noexcept
  {
    using namespace std::string_view_literals;

    return print_format ([&token, mismatch] (auto &ctx) {
      auto out = ctx.out ();
      switch (token.type)
        {
        case oicompare::token_type::eof:
          out = std::move (
              std::ranges::copy ("koniec pliku"sv, std::move (out)).out);
          break;
        case oicompare::token_type::newline:
          out = std::move (
              std::ranges::copy ("koniec wiersza"sv, std::move (out)).out);
          break;
        case oicompare::token_type::word:
          represent_word<Kind == kind::abbreviated>::represent (
              out, {token.first, token.last},
              mismatch ? mismatch - token.first : 0);
          break;
        default:
#ifdef __GNUC__
          __builtin_unreachable ();
#endif
          out = std::move (
              std::ranges::copy ("nieznany token"sv, std::move (out)).out);
          break;
        }
      return out;
    });
  }

  static void
  print (const std::optional<oicompare::mismatch<const char *, const char *>>
             &mismatch)
  {
    if (mismatch)
      switch (Kind)
        {
        case kind::terse:
          fmt::println ("ŹLE");
          break;
        case kind::abbreviated:
        case kind::full:
          fmt::println ("ŹLE: wiersz {}: oczekiwano {}, otrzymano {}",
                        mismatch->line_number,
                        represent (mismatch->first,
                                   mismatch->first_difference.has_value ()
                                       ? mismatch->first_difference->first
                                       : nullptr),
                        represent (mismatch->second,
                                   mismatch->first_difference.has_value ()
                                       ? mismatch->first_difference->second
                                       : nullptr));
          break;
        }
    else
      fmt::println ("OK");
  }
};

using translation = void (*) (
    const std::optional<oicompare::mismatch<const char *, const char *>> &);
}

#endif /* __OICOMPARE_TRANSLATIONS_HH__ */