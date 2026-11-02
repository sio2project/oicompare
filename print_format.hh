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
struct fmt::formatter<basic_print_format<CharT, Func>, CharT>
{
  template <typename ParseContext>
  constexpr auto
  parse (ParseContext &ctx) const -> ParseContext::iterator
  {
    auto it = ctx.begin (), end = ctx.end ();
    if (it != end && *it != CharT{'}'})
      throw format_error{"invalid format"};
    return it;
  }

  template <typename FormatContext>
  constexpr auto
  format (const basic_print_format<CharT, Func> &obj, FormatContext &ctx) const
      -> FormatContext::iterator
  {
    return obj.func (ctx);
  }
};

#endif /* __OICOMPARE_PRINT_FORMAT_HH__*/