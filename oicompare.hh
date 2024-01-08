#ifndef __OICOMPARE_HH__
#define __OICOMPARE_HH__

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace oicompare
{
namespace detail
{
constexpr bool
is_whitespace (char ch) noexcept
{
  return ch == '\0' || ch == '\t' || ch == '\v' || ch == '\r' || ch == ' ';
}

template <typename It>
concept char_iterator
    = std::forward_iterator<It>
      && std::convertible_to<std::iter_reference_t<It>, char>;

template <typename R>
concept char_range
    = std::ranges::forward_range<R>
      && std::convertible_to<std::ranges::range_reference_t<R>, char>
      && char_iterator<std::ranges::iterator_t<R>>;
}

/**
 * Type of token.
 */
enum class token_type
{
  /**
   * Word: a sequence of non-whitespace characters.
   */
  word,

  /**
   * Newline: a LF character.
   *
   * We do not recognize CR in any way as a part of a newline sequence.
   */
  newline,

  /**
   * EOF: end of file.
   */
  eof,
};

/**
 * An input token.
 */
template <detail::char_iterator It> struct token
{
  /**
   * Type of token.
   */
  token_type type;

  /**
   * Beginning of the token.
   */
  It first;

  /**
   * End of the token.
   */
  It last;

  /**
   * Compares two tokens.
   *
   * Two tokens are equal if their types are the same. Furthermore, word tokens
   * need to be equal (other tokens need not).
   *
   * @param lhs first token
   * @param rhs second token
   */
  template <detail::char_iterator It2>
  constexpr std::optional<std::optional<std::pair<It, It2>>>
  compare (const token<It2> &other)
  {
    if (type != other.type)
      return std::optional<std::pair<It, It2>>{std::nullopt};
    else if (type == token_type::word)
      {
        auto [mismatch, mismatch_other]
            = std::ranges::mismatch (first, last, other.first, other.last);
        if (mismatch == last && mismatch_other == other.last)
          return std::nullopt;
        else
          return {{{std::move (mismatch), std::move (mismatch_other)}}};
      }
    else
      return std::nullopt;
  }
};

namespace detail
{
template <detail::char_iterator It, std::sentinel_for<It> Sent>
constexpr token<It>
scan (It &first, Sent last)
{
  using token = token<It>;

  while (first != last && is_whitespace (*first))
    ++first;

  if (first == last)
    return token{token_type::eof, first, first};
  else if (*first == '\n')
    {
      auto first_save = first;
      ++first;
      return token{token_type::newline, first_save, first};
    }
  else
    {
      auto first_save = first;
      while (first != last && !is_whitespace (*first) && *first != '\n')
        ++first;
      return token{token_type::word, first_save, first};
    }
}
}

/**
 * Represents a mismatch in the inputs.
 */
template <detail::char_iterator It1, detail::char_iterator It2> struct mismatch
{
  /**
   * The line number in which the mismatch occurred.
   */
  std::make_unsigned_t<std::iter_difference_t<It1>> line_number;

  /**
   * The mismatch.
   */
  std::optional<std::pair<It1, It2>> first_difference;

  /**
   * The token found in the first file.
   */
  token<It1> first;

  /**
   * The token found in the second file.
   */
  token<It2> second;
};

/**
 * Compare two input ranges, returning the mismatch or none if they are
 * equivalent.
 *
 * @param first1 first input begin
 * @param last1 first input end
 * @param first2 last input begin
 * @param last2 last input end
 * @return mismatch or none
 */
template <detail::char_iterator It1, std::sentinel_for<It1> Sent1,
          detail::char_iterator It2, std::sentinel_for<It2> Sent2>
constexpr std::optional<mismatch<It1, It2>>
compare (It1 first1, Sent1 last1, It2 first2, Sent2 last2)
{
  std::make_unsigned_t<std::iter_difference_t<It1>> line_number = 1;

  while (true)
    {
      auto tok1 = detail::scan (first1, last1);
      auto tok2 = detail::scan (first2, last2);

      if (tok1.type == token_type::eof)
        while (tok2.type == token_type::newline)
          tok2 = detail::scan (first2, last2);
      else if (tok2.type == token_type::eof)
        while (tok1.type == token_type::newline)
          tok1 = detail::scan (first1, last1);

      if (auto mismatch = tok1.compare (tok2))
        return {{line_number, std::move (*mismatch), tok1, tok2}};
      else if (tok1.type == token_type::newline)
        ++line_number;
      else if (tok1.type == token_type::eof)
        break;
    }

  return {};
}

/**
 * Compare two input ranges, returning the mismatch or none if they are
 * equivalent.
 *
 * @param range1 first range
 * @param range2 last range
 * @return mismatch or none
 */
template <detail::char_range R1, detail::char_range R2>
constexpr std::optional<
    mismatch<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>>>
compare (R1 &&range1, R2 &&range2)
{
  return compare (std::ranges::begin (range1), std::ranges::end (range1),
                  std::ranges::begin (range2), std::ranges::end (range2));
}
}

#endif /* __OICOMPARE_HH__ */