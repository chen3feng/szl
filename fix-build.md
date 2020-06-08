# Fix Build For Modern C++ Environment

## My Envirionment

- Ubuntu 20.04 LTS (Run in Docker under macOS)
- GCC 9.3.0

## Install Prerequisites

For Ubuntu, apt install:
- openssl
- libssl-dev
- libpcre++-dev
- libicu-dev

## Regenerate pb.h/cc

The orginal source package contains pre-built pb.h and pb.cc, which are from pb 2.3.0.
They are too old.

Install protobuf from souce:

- Download protobuf-3.4.1 source package
- tar xf, ./configure, make, make install
- Run `ldconfig` to let protoc can find it's shared libraries
  (Or disable shared library during configure stage)

```bash
cd src
protoc --cpp_out=. emitvalues/sawzall.proto
```

## Remove Build Errors

Open `configure.ac`, find the line which contains

```make
WARN_FLAGS="-Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare"
```

Add `-Wno-unused-local-typedefs` after the existed flags.

## Fix Build Warnings

- In `src/engine/assembler.cc`, change the type of `int8` arrays to `uint8`
- In `src/utilities/gzipwrapper.cc`, add `unsigned` to the type of `const char magic[]`
- in `src/engine/code.cc` and `src/utilities/random_base.cc`, add `#include <unistd.h>`
- in `src/engine/form.cc`, Change some `return false;`s to `return NULL;`
- In `src/protoc_plugin/szl_generator.h`, add missing `std::` prefix to some places before `set`, `map`, and `string`.

## Fix Warnings

DOING.

Most of warnings looks not important, but are easy to be fixed. for example,

```
engine/linecount.cc:124:14: warning: invalid suffix on literal; C++11 requires a space between literal and string macro [-Wliteral-suffix]
  124 |       printf("%d,%d\t%d\t%"PRId64"\n", i, cnt_pairs[i].node->line_counter(),
```

can be fixed by adding 2 spaces around `PRId64`.

Some other kinds of errors include `-Wnarraw`, `-Wunused-result`, `-Woverflow`, etc.

## Fix Test Errors

TODO

There are many test error to be fixed.

You can find them just by running the `make check` command.
