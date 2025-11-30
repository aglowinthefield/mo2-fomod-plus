#pragma once

#include <format>
#include <type_traits>

// random access container formatter (vector, QStringList, ...)

namespace MOBase::details
{
template <typename Container>
concept random_access_container = requires(Container const& c) {
  { c.begin() } -> std::random_access_iterator;
  { c.end() } -> std::random_access_iterator;
};
}  // namespace MOBase::details

// formatter specialization for random access container, the default format is
// [ a, b, c ] up to 3 elements, then [ a, b, ..., c ]
//
// it is possible to modify the output by changing the format string, check the test
// file for examples
//
template <typename Container, class CharT>
  requires MOBase::details::random_access_container<Container>
struct std::formatter<Container, CharT>
{
  static constexpr CharT field_delimiter = '$';

  // brackets to use - space_bracket indicates if there should be a space between
  // bracket and content (default true)
  CharT open_bracket  = static_cast<CharT>('[');
  bool space_bracket  = true;
  CharT close_bracket = static_cast<CharT>(']');

  // delimiter to use
  basic_string_view<CharT> delimiter = ", ";

  // maximum number of element to show
  std::size_t max_show = 3;

  // retrieve the next character from the given iterator or throw format_error if the
  // iterator is at the end of the context
  //
  template <class ParseContext>
  constexpr CharT next_or_throw(ParseContext const& ctx,
                                typename ParseContext::iterator it)
  {
    if (it == ctx.end()) {
      throw std::format_error("invalid format args for random access container.");
    }

    return *it;
  }

  //
  template <class ParseContext>
  constexpr auto end_or_next(ParseContext const& ctx,
                             typename ParseContext::iterator& it)
  {
    if (it == ctx.end() || *it == '}') {
      return it;
    }

    if (*it == field_delimiter) {
      return ++it;
    }

    return it;
  }

  // parse a size from the context starting at the given iterator value
  //
  template <class ParseContext>
  constexpr auto parse_size(ParseContext& ctx, typename ParseContext::iterator it,
                            std::size_t& value)
  {
    if (it == ctx.end() || *it == '}' || *it == field_delimiter) {
      return it;
    }
    value = value * 10 + (*it++ - static_cast<CharT>('0'));
    return parse_size(ctx, it, value);
  }

  // increment the given iterator until the end of the context or the next field
  // delimiter is found
  //
  template <class ParseContext>
  constexpr auto find_end(ParseContext& ctx, typename ParseContext::iterator it)
  {
    if (it == ctx.end() || *it == '}' || *it == field_delimiter) {
      return it;
    }
    return find_end(ctx, ++it);
  }

  template <class ParseContext>
  constexpr ParseContext::iterator parseImpl(ParseContext& ctx,
                                             typename ParseContext::iterator it)
  {
    if (it == ctx.end() || *it == '}') {
      return it;
    }

    // the format string is delimited by field delimiter ($) - here we check the first
    // character and then parse up to the end of format string or the next delimiter,
    // and we repeat (recursively, due to constexpr)
    //
    const auto c = *it++;
    switch (c) {

    // b specifies the opening and closing brackets, if there is a space between the two
    // brackets, spaces will be included in the output string after the opening bracket
    // and before the closing one
    //
    case 'b':
      open_bracket  = next_or_throw(ctx, it++);
      close_bracket = next_or_throw(ctx, it++);

      if (close_bracket == ' ') {
        space_bracket = true;
        close_bracket = next_or_throw(ctx, it++);
      } else {
        space_bracket = false;
      }

      it = end_or_next(ctx, it);
      break;

    // n specifies the maximum number of values to include in the format string before
    // collapsing to '...' (default is 3)
    //
    case 'n': {
      max_show = 0;
      it       = parse_size(ctx, it, max_show);

      if (*it == field_delimiter) {
        it++;
      }

    } break;

    // d specifies the delimiter to use between value, this is a multi-characters string
    case 'd': {
      const auto start = it;
      it               = find_end(ctx, it);
      delimiter        = std::basic_string_view<CharT>(start, it);

      if (*it == field_delimiter) {
        it++;
      }

    } break;

    default:
      throw std::format_error("invalid format args for random access container.");
    }

    return parseImpl(ctx, it);
  }

  template <class ParseContext>
  constexpr ParseContext::iterator parse(ParseContext& ctx)
  {
    return parseImpl(ctx, ctx.begin());
  }

  template <typename FmtIt, typename It>
  FmtIt formatRange(FmtIt& out, It cur, It end) const
  {
    while (cur != end) {
      out = std::format_to(out, "{}", *cur);
      ++cur;

      if (cur != end) {
        out = std::format_to(out, "{}", delimiter);
      }
    }

    return out;
  }

  template <typename FmtContext>
  FmtContext::iterator format(Container v, FmtContext& ctx) const
  {
    const auto n = v.end() - v.begin();

    // no item, we just output brackets, e.g. '[]' (no space)
    if (n == 0) {
      return std::format_to(ctx.out(), "{}{}", open_bracket, close_bracket);
    }

    // add opening bracket + possibly a space, e.g. '[' or '[ '
    auto out =
        std::format_to(ctx.out(), "{}{}", open_bracket, space_bracket ? " " : "");

    if (n <= max_show) {
      // less than max_show elements, format the whole range
      out = formatRange(out, v.begin(), v.end());
    } else {
      // otherwise, add first max_show/2 elements, then ellipsis (...), then last
      // max_show/2 elements - if max_show is odd, one more element is added at the
      // beginning
      const auto n_first = (max_show % 2 == 1) ? max_show / 2 + 1 : max_show / 2;
      out                = formatRange(out, v.begin(), v.begin() + n_first);
      out                = std::format_to(out, "{}...{}", delimiter, delimiter);
      out                = formatRange(out, v.end() - (max_show - n_first), v.end());
    }

    // format possibly a space + closing bracket, e.g. ']' or ' ]'
    return std::format_to(ctx.out(), "{}{}", space_bracket ? " " : "", close_bracket);
  }
};
