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

Please note that most translations (including the default) will assume that
the first file is the *expected output* and the second file is the *received
output*.

The following translations are available:

  * `english` (the default), `english_terse` – messages in English
  * `polish`, `polish_terse` – messages in Polish, in UTF-8
  * `polish_ascii`, `polish_ascii_terse` – messages in Polish, replacing
    diacritic characters with closest ASCII equivalents

The `terse` variants only print the code (like `OK` or `WRONG`) without
explaining the error. In particular, these do not leak any information about
the input files (other than them not being equivalent).

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
empty value means that no mismatch was found (so the outputs are equivalent).
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
  size_t line_number;
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