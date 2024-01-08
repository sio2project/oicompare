# oicompare

**oicompare** is a very simple program for comparing files. It works both as a
command line utility and as a library, for example for embedding in custom
checker programs.

## Building

**oicompare** has very minimal dependencies:

  * A C++20 compatible compiler (tested on recent GCC and Clang)
  * The Meson build system (so also the Ninja build tool)

**oicompare** also uses two libraries internally, but these are both embedded
and there is no extra effort in getting them:

  * [fmt](https://fmt.dev/latest/index.html) for message formatting
  * [mio](https://github.com/vimpunk/mio) for memory mapped files

## Command line usage

To use the `oicompare` program, simply pass it two files:

```sh
oicompare expected.txt received.txt
```

You may also pass the third argument to select a different translation:

```sh
oicompare expected.txt received.txt english_terse
```

Please note that most translations will assume that the first file is the
*expected program output* and the second file is the *got program output*.
Specify the translation in the form `language_kind`.

The following languages are available:

  * `english` (the default) – messages in English
  * `polish` – messages in Polish

The following translation kinds are available:

  * `terse` – only print whether the inputs match, this yields no more
    information than the exit code
  * `abbreviated` – show the mismatched tokens, using no more than 100
    bytes for each token's representation, and thus no more than 255 bytes for
    the entire report
  * `full` – show the mismatched tokens

## API usage

The header-only API in `oicompare.hh` provides the `oicompare::compare`
function (in two variants), along with supporting types
`oicompare::token_type`, `oicompare::token<It>` and `oicompare::mismatch<It1,
  It2>`.

The `oicompare::compare` function accepts either two
[forward ranges](https://en.cppreference.com/w/cpp/ranges/forward_range) of
`char`, or two iterator-sentinel pairs. This means that memory-mapped files and
strings are easily comparable. However, to use this on sequential file streams
(for example, for pipes), you would need to implement a buffer with unlimited
look-back.

The result is `optional<mismatch<It1, It2>>`, with `mismatch` specialized for
iterators of the two ranges passed (they need not be of the same type). An
empty value means that no mismatch was found (so the inputs are equivalent).
The `mismatch` structure has the following fields:

```cpp
enum class token_type
{
  word,
  newline,
  eof
};

template <typename It>
struct token
{
  token_type type;
  It first;
  It last;
};

template <typename It1, typename It2>
struct mismatch
{
  make_unsigned_t<iter_difference_t<It1>> line_number;
  optional<pair<It1, It2>> first_difference;
  token<It1> first;
  token<It2> second;
};
```

Where `first` and `second` are the tokens from the first and second file
respectively that differ.

## Tests

Test data is encoded in `tests.hh`. The file `tester.cc` runs these tests both
during compile time (as all of **oicompare** logic is `constexpr`) and during
run time. The tester is primitive and may be expanded in the future. The API of
the test data is currently **not stable**.