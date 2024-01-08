#ifndef __OICOMPARE_PRINT_FORMAT_HH__
#define __OICOMPARE_PRINT_FORMAT_HH__

#include <utility>

#include <fmt/format.h>

namespace
{
template <typename CharT, typename Func> struct basic_print_format
{
  constexpr explicit basic_print_format (Func &&func)
      : func{std::forward<Func> (func)}
  {
  }

  Func func;
};

template <typename Func>
constexpr auto
print_format (Func &&func)
{
  return basic_print_format<char, Func> (std::forward<Func> (func));
}
}

template <typename CharT, typename Func>
struct fmt::formatter<basic_print_format<CharT, Func>>
{
  template <typename OutputIt>
  constexpr fmt::basic_format_context<OutputIt, CharT>::iterator
  format (basic_print_format<CharT, Func> &&obj,
          fmt::basic_format_context<OutputIt, CharT> &ctx) const
  {
    return std::move (obj).func (ctx);
  }

  template <typename OutputIt>
  constexpr fmt::basic_format_context<OutputIt, CharT>::iterator
  format (basic_print_format<CharT, Func> &obj,
          fmt::basic_format_context<OutputIt, CharT> &ctx) const
  {
    return format (std::move (obj), ctx);
  }

  constexpr auto
  parse (basic_format_parse_context<CharT> &ctx) const
      -> format_parse_context::iterator
  {
    auto it = ctx.begin (), end = ctx.end ();
    if (it != end && *it != CharT{'}'})
      throw format_error{"invalid format"};
    return it;
  }
};

#endif /* __OICOMPARE_PRINT_FORMAT_HH__*/