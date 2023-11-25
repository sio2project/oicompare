#ifndef __OICOMPARE_TESTS_HH__
#define __OICOMPARE_TESTS_HH__

#include <variant>

#include "oicompare.hh"

namespace oicompare::tests
{
using namespace std::string_view_literals;

struct token
{
  token_type type;
  std::size_t first;
  std::size_t last;
};

struct success
{
  constexpr void
  swap () noexcept
  {
  }
};

struct failure
{
  std::size_t line_number;
  token first;
  token second;

  constexpr void
  swap () noexcept
  {
    std::swap (first, second);
  }
};

struct result : std::variant<success, failure>
{
  constexpr void
  swap () noexcept
  {
    std::visit ([] (auto &x) { x.swap (); }, *this);
  }
};

struct test_case
{
  result expected_result;
  std::string_view first;
  std::string_view second;
};

#define REP10(X) X X X X X X X X X X
#define REP100(X) REP10 (REP10 (X))

constexpr auto test_cases = std::array{
    // Identical
    test_case{success{}, ""sv, ""sv},
    test_case{success{}, "ABC"sv, "ABC"sv},
    test_case{success{}, "ABC\nDEF\n"sv, "ABC\nDEF\n"sv},

    // Equivalent whitespace
    test_case{success{}, "A B C D E"sv, "A B\tC\0D\rE"sv},

    // Excessive whitespace
    test_case{success{}, "A B"sv, "A B \t\0\r"sv},
    test_case{success{}, "A\0B"sv, "\0\t A   \r \0B\t\t\t"sv},

    // Trailing newlines
    test_case{success{}, "A\nB\nC"sv, "A\nB\nC\n\n\n\n\n\n\n\n\n\n"sv},
    test_case{success{}, "A\n\n\n"sv, "A\n\n\n\n\n"sv},

    // Simply wrong
    test_case{failure{1, {token_type::eof, 0, 0}, {token_type::word, 0, 8}},
              ""sv, "NONEMPTY"sv},
    test_case{failure{1, {token_type::word, 0, 3}, {token_type::word, 0, 2}},
              "YES"sv, "NO"sv},
    test_case{failure{1, {token_type::word, 0, 3}, {token_type::word, 0, 3}},
              "TAK"sv, "NIE"sv},
    test_case{failure{1, {token_type::word, 0, 1}, {token_type::word, 0, 1}},
              "0"sv, "1"sv},
    test_case{failure{1, {token_type::word, 0, 4}, {token_type::word, 0, 2}},
              "NONE"sv, "-1"sv},
    test_case{failure{1, {token_type::word, 0, 3}, {token_type::word, 0, 3}},
              "123"sv, "456"sv},
    test_case{failure{1, {token_type::word, 0, 1}, {token_type::word, 0, 32}},
              "1"sv, "1.000000000000000000000000000001"sv},
    test_case{failure{1, {token_type::word, 4, 5}, {token_type::word, 4, 5}},
              "1 2 3 4"sv, "1 2 4 3"sv},

    // Different arrangement of lines
    test_case{
        failure{1, {token_type::word, 2, 3}, {token_type::newline, 1, 2}},
        "A B"sv, "A\nB"sv},

    // Bug of original compare which would consider these equal
    test_case{
        failure{1, {token_type::word, 0, 101}, {token_type::word, 0, 100}},
        REP100 ("A"sv) "B"sv, REP100 ("A"sv) " B"sv},
};

#undef REP100
#undef REP10

template <typename It>
constexpr bool
compare_token (It first, const token &expected,
               const oicompare::token<It> &got)
{
  return expected.type == got.type
         && expected.first == static_cast<std::size_t> (got.first - first)
         && expected.last == static_cast<std::size_t> (got.last - first);
}

template <typename It1, typename It2>
constexpr bool
compare_result (It1 first1, It2 first2, const result &expected,
                const std::optional<oicompare::mismatch<It1, It2>> &got)
{
  if (std::holds_alternative<failure> (expected))
    {
      if (!got)
        return false;

      const auto &fail = std::get<failure> (expected);

      return fail.line_number == got->line_number
             && compare_token (first1, fail.first, got->first)
             && compare_token (first2, fail.second, got->second);
    }
  else
    return !got;
}
}

#endif /* __OICOMPARE_TESTS_HH__ */