# BufferStream

Version: 1.1.0

BufferStream presents a memory buffer as an Arduino `Stream`. `ReadBufferStream` serves the bytes of a buffer through `read()`, `peek()` and `available()`, so any code that parses a `Stream` (`parseInt()`, `readBytes()`, a JSON reader) can parse memory. `WriteBufferStream` collects `print()` and `write()` output in a fixed buffer, reports the bytes written and flags an overflow. The library allocates nothing and copies nothing. The caller owns the buffer and keeps it alive while the stream object exists.

This repository is a maintained fork of the BufferStream library by Industrial Shields (Boot&Work Corp. S.L.). See "Version history" for the changes.

## Installation

- Arduino IDE: download the repository as a ZIP file and use Sketch, Include Library, Add .ZIP Library. The library is not in the Arduino Library Manager index.
- PlatformIO: add the tagged repository to `platformio.ini`:

```ini
lib_deps =
    https://github.com/arkhipenko/arduino-BufferStream.git#v1.1.0
```

The library has no build options. It builds for every Arduino architecture whose core provides `Stream`. CI compiles the examples for AVR (Uno) and ESP32 (arduino-esp32 2.0.17 and 3.x, ESP32 and ESP32-C3).

## Quick start

```cpp
#include <BufferStream.h>

// Parse a buffer
const char record[] = "id=17;temp=-4";
ReadBufferStream in(record, sizeof(record) - 1);
long id = in.parseInt();    // 17
long temp = in.parseInt();  // -4

// Format into a buffer
char line[32];
WriteBufferStream out(line, sizeof(line) - 1);  // one byte kept for the '\0'
out.print("temp=");
out.print(temp);
line[out.written()] = '\0';                     // "temp=-4"
if (out.getWriteError()) {
  // the text did not fit and was truncated
}
```

## API

`#include <BufferStream.h>` includes both classes and the version macros.

### ReadBufferStream

| Member | Description |
| --- | --- |
| `ReadBufferStream(const void *buff, size_t len)` | Reads `len` bytes from `buff`. Accepts `char`, `uint8_t`, `const` buffers and `String::c_str()` without a cast. The buffer is never written. |
| `int available()` | Bytes left. Capped at `INT_MAX`. |
| `int read()` | The next byte (0 to 255), then advances. -1 at the end. |
| `int peek()` | The next byte (0 to 255) without advancing. -1 at the end. |
| `size_t write(...)` | Returns 0. A read stream accepts no output. |

All `Stream` methods work on top of these (`readBytes()`, `readString()`, `readStringUntil()`, `find()`, `parseInt()`, `parseFloat()`).

### WriteBufferStream

| Member | Description |
| --- | --- |
| `WriteBufferStream(void *buff, size_t len)` | Writes at most `len` bytes into `buff`. Accepts `char` and `uint8_t` buffers. |
| `size_t write(uint8_t value)` | Stores one byte and returns 1. Returns 0 and sets the write error when the buffer is full. |
| `size_t write(const uint8_t *buffer, size_t size)` | Copies what fits and returns that count. Sets the write error when not all of it fits. |
| `int availableForWrite()` | Free space in bytes. Capped at `INT_MAX`. |
| `size_t written() const` | Bytes stored since construction. |
| `int getWriteError()`, `void clearWriteError()` | From `Print`. Non-zero after a write that did not fit. |
| `int available()`, `int read()`, `int peek()` | Return 0, -1, -1. A write stream has no input. |

All `Print` methods work on top of these (`print()`, `println()`, and `printf()` where the core has it).

### Version macros

| Macro | Value |
| --- | --- |
| `BUFFERSTREAM_VERSION_STRING` | `"1.1.0"` |
| `BUFFERSTREAM_VERSION` | `10100` (major * 10000 + minor * 100 + patch) |

## Semantics

- Both streams move forward only. There is no rewind. Create a new stream object to start again.
- `ReadBufferStream` sets the `Stream` timeout to 0 in its constructor. A buffer cannot receive more data, so `readBytes()`, `parseInt()` and the other timed methods return at the end of the buffer at once. A caller can still call `setTimeout()` afterwards.
- `WriteBufferStream` adds no `'\0'` terminator. Write it yourself, or reserve one byte and use `written()` as the index (see the quick start).
- A write that does not fit stores the bytes that fit. `print()` returns the count stored, and `getWriteError()` becomes non-zero. `clearWriteError()` resets the flag only. It does not free space.
- The objects hold a pointer to the caller's buffer. They are not thread safe.

## Limits and costs

- Memory: one pointer and one `size_t` per stream, plus one `size_t` for `WriteBufferStream`.
- `available()` and `availableForWrite()` return `int`. For a buffer above `INT_MAX` bytes (32767 on AVR) they report `INT_MAX`.
- A bulk `write(buffer, size)` is one `memcpy`. `print()` of a string uses it.

## Examples

| Example | Shows |
| --- | --- |
| `ReadString` | Reading a `String` byte by byte. |
| `WriteString` | Building a C string with `print()` and checking the terminator fits. |
| `ParseBuffer` | `parseInt()`, `readStringUntil()` and `readBytes()` over a buffer, with no timeout wait at the end. |
| `WriteOverflow` | `written()`, `availableForWrite()` and `getWriteError()` when the text does not fit. |

## Tests

`make -C tests/host` builds the library against a host model of the Arduino `Print` and `Stream` classes (`tests/host/stubs/Arduino.h`, written from the AVR core source). It runs the unit tests and the `setup()` of every example, and it compares each example's output with `tests/host/expected/`. GitHub Actions runs the host tests with g++ and clang++ in C++11, C++17 and C++20, compiles the examples for AVR and ESP32, and runs Arduino Lint.

## Upgrading to 1.1.0

- The constructors take `const void *` (read) and `void *` (write). Code that cast a buffer to compile against 1.0.0 still compiles. The casts can be removed.
- `ReadBufferStream` no longer waits 1000 ms at the end of the buffer in `readBytes()`, `readString()`, `readStringUntil()`, `find()`, `parseInt()` and `parseFloat()`. Call `setTimeout()` to restore a wait.
- `WriteBufferStream` sets the write error on an overflow. Code that checks `getWriteError()` now sees truncation that 1.0.0 did not report.
- `WriteBufferStream::availableForWrite()` returns the free space instead of 0.
- `ReadBufferStream::available()` is capped at `INT_MAX` instead of going negative.

## Version history

### 1.1.0 - 2026-10-03

Fixes:

- D1: the library did not compile without `-fpermissive`. `BaseBufferStream` assigned a `void *` to its `uint8_t *` member. It failed on ESP32 and built on AVR only because the AVR core passes `-fpermissive`.
- D2: `ReadBufferStream` rejected `const` buffers such as `String::c_str()`, so the `ReadString` example did not compile on ESP32. The read constructor now takes `const void *`.
- D3: reading past the end waited the full `Stream` timeout (1000 ms) in every timed `Stream` method. The read stream now sets the timeout to 0.
- D4: `available()` turned negative for a buffer above `INT_MAX` bytes. It is now capped.

Additions:

- I1: `WriteBufferStream::written()` and `availableForWrite()`.
- I2: `WriteBufferStream` sets the write error on an overflow.
- I3: `WriteBufferStream::write(buffer, size)` copies with one `memcpy`.
- I4: `BUFFERSTREAM_VERSION_STRING` and `BUFFERSTREAM_VERSION`.
- New examples `ParseBuffer` and `WriteOverflow`, host tests, `library.json`, GitHub Actions CI.

### 1.0.0 - 2019-08-30

First release by Industrial Shields (github.com/IndustrialShields/arduino-BufferStream, since moved to github.com/Industrial-Shields/arduino-BufferStream).

## License

GNU Lesser General Public License, version 3 or later. See `COPYING`. Copyright (c) 2019 Boot&Work Corp. S.L. Modified 2026 by Anatoli Arkhipenko.
