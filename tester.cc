#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <fmt/format.h>

#include "oicompare.hh"
#include "tests.hh"

using namespace oicompare::tests;

namespace
{
constexpr bool
test_constexpr ()
{
  for (const auto &test_case : test_cases)
    {
      {
        auto result = oicompare::compare (test_case.first, test_case.second);

        if (!compare_result (test_case.first.begin (),
                             test_case.second.begin (),
                             test_case.expected_result, result))
          return false;
      }

      // Test symmetry
      {
        auto result = oicompare::compare (test_case.second, test_case.first);

        auto expected = test_case.expected_result;
        expected.swap ();

        if (!compare_result (test_case.second.begin (),
                             test_case.first.begin (), expected, result))
          return false;
      }
    }

  return true;
}

// Run tests in compile time.
static_assert (test_constexpr ());
}

int
main ()
{
  std::size_t index = 0;
  for (const auto &test_case : test_cases)
    {
      // Prevent the compiler from optimizing the test
      std::string first_copy (test_case.first.size (), '\0');
      std::memcpy (first_copy.data (), test_case.first.data (),
                   test_case.first.size ());
      std::string second_copy (test_case.second.size (), '\0');
      std::memcpy (second_copy.data (), test_case.second.data (),
                   test_case.second.size ());

      {
        auto result = oicompare::compare (first_copy, second_copy);

        if (!compare_result (first_copy.begin (), second_copy.begin (),
                             test_case.expected_result, result))
          {
            fmt::println ("Test {} failed\n", index);
            return 1;
          }
      }

      // Test symmetry
      {
        auto result = oicompare::compare (second_copy, first_copy);

        auto expected = test_case.expected_result;
        expected.swap ();

        if (!compare_result (second_copy.begin (), first_copy.begin (),
                             expected, result))
          {
            fmt::println ("Test {} failed symmetry check!\n", index);
            return 99;
          }
      }

      ++index;
    }

  return 0;
}