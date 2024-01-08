#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <mio/mmap.hpp>

#include "oicompare.hh"
#include "print_format.hh"
#include "translations.hh"

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

oicompare::translations::translation
parse_translation (std::string_view name)
{
  using namespace oicompare::translations;

  if (name == "english_abbreviated"sv)
    return english_translation<kind::abbreviated>::print;
  else if (name == "english_full"sv)
    return english_translation<kind::full>::print;
  else if (name == "english_terse"sv)
    return english_translation<kind::terse>::print;
  else if (name == "polish_abbreviated"sv)
    return polish_translation<kind::abbreviated>::print;
  else if (name == "polish_full"sv)
    return polish_translation<kind::full>::print;
  else if (name == "polish_terse"sv)
    return polish_translation<kind::terse>::print;
  else
    return nullptr;
}
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

  std::string_view translation_name = argc < 4 ? "english_terse"sv : argv[3];
  oicompare::translations::translation translation;

  if (auto parsed_translation = parse_translation (translation_name))
    translation = parsed_translation;
  else
    {
      fmt::println (stderr, "Unknown translation: {}", translation_name);
      return 2;
    }

  auto result = oicompare::compare (file1.mmap (), file2.mmap ());
  translation (result);

  return result ? EXIT_FAILURE : EXIT_SUCCESS;
}