#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <mio/mmap.hpp>

#include "oicompare.hh"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace
{
class mapped_file
{
public:
  mapped_file (const std::filesystem::path &path) : mmap_{create_mmap (path)}
  {
  }

  constexpr const mio::mmap_source &
  mmap () const noexcept
  {
    return mmap_;
  }

private:
  static mio::mmap_source
  create_mmap (const std::filesystem::path &path)
  {
    auto size = std::filesystem::file_size (path);

    if (size == 0)
      // mmap() will not let us map something of size 0
      return {};

    return {path.native (), 0, size};
  }

  mio::mmap_source mmap_;
};

constexpr bool
is_ascii_printable (char ch)
{
  return ch >= 32 && ch <= 126;
}

std::string
represent (const oicompare::token<const char *> &token) noexcept
{
  switch (token.type)
    {
    case oicompare::token_type::eof:
      return "EOF"s;
    case oicompare::token_type::newline:
      return "EOL"s;
    case oicompare::token_type::word:
      return fmt::format (
          R"("{}")",
          fmt::join (
              std::string_view{token.first, token.last}
                  | std::views::transform ([&] (char ch) -> std::string {
                      if (is_ascii_printable (ch))
                        return {ch};
                      else
                        {
                          constexpr std::array hex_table{
                              '0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

                          return {'<',
                                  '0',
                                  'x',
                                  hex_table[ch >> 4],
                                  hex_table[ch & 0x0F],
                                  '>'};
                        }
                    }),
              ""sv));
    default:
      return "<unknown>"s;
    }
}

template <bool Terse> struct english_translation
{
  static void
  print (const std::optional<oicompare::mismatch<const char *, const char *>>
             &mismatch)
  {
    if (mismatch)
      fmt::println (Terse ? "WRONG" : "WRONG: line {}: expected {}, got {}",
                    mismatch->line_number, represent (mismatch->first),
                    represent (mismatch->second));
    else
      fmt::println ("OK");
  }
};

template <bool Ascii, bool Terse> struct polish_translation
{
  static void
  print (const std::optional<oicompare::mismatch<const char *, const char *>>
             &mismatch)
  {
    if (mismatch)
      fmt::println (Terse ? "{}"
                          : "{}: wiersz {}: oczekiwano {}, otrzymano {}",
                    Ascii ? "ZLE" : "ŹLE", mismatch->line_number,
                    represent (mismatch->first), represent (mismatch->second));
    else
      fmt::println ("OK");
  }
};

using translation = void (*) (
    const std::optional<oicompare::mismatch<const char *, const char *>> &);
}

int
main (int argc, char **argv)
{
  if (argc < 3 || argc > 4) [[unlikely]]
    {
      fmt::println (stderr, "Usage: {} FILE1 FILE2 [TRANSLATION]", argv[0]);
      return 2;
    }

  mapped_file file1{argv[1]};
  mapped_file file2{argv[2]};

  std::string_view translation_name = argc < 4 ? "english"sv : argv[3];
  translation trans;

  if (translation_name == "english"sv)
    trans = english_translation<false>::print;
  else if (translation_name == "english_terse"sv)
    trans = english_translation<true>::print;
  else if (translation_name == "polish"sv)
    trans = polish_translation<false, false>::print;
  else if (translation_name == "polish_terse"sv)
    trans = polish_translation<false, true>::print;
  else if (translation_name == "polish_ascii"sv)
    trans = polish_translation<true, false>::print;
  else if (translation_name == "polish_ascii_terse"sv)
    trans = polish_translation<true, true>::print;
  else
    {
      fmt::println ("Unknown translation: {}", translation_name);
      return 2;
    }

  auto result = oicompare::compare (
      file1.mmap ().data (), file1.mmap ().data () + file1.mmap ().size (),
      file2.mmap ().data (), file2.mmap ().data () + file2.mmap ().size ());
  trans (result);

  return result ? EXIT_FAILURE : EXIT_SUCCESS;
}