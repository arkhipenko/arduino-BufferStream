// Host model of the Arduino API used by BufferStream, its tests and its examples.
//
// Print and Stream follow framework-arduino-avr 1.8.x cores/arduino/Print.{h,cpp} and
// Stream.{h,cpp}. The parts that decide BufferStream behavior are copied in logic:
// - Print::write(buf, n) calls write(uint8_t) per byte and stops at the first 0.
// - Print::setWriteError() is protected, the error flag is private.
// - Stream::timedRead()/timedPeek() poll read()/peek() until millis() - start >= _timeout
//   (default 1000 ms). arduino-esp32 3.3.9 cores/esp32/Stream.cpp has the same logic.
//
// millis() is a fake clock: each call advances it by 1 ms, so a timeout loop ends and the
// time it spent can be measured (g_millis). delay() advances it by the given amount.
#ifndef BUFFERSTREAM_HOST_ARDUINO_H
#define BUFFERSTREAM_HOST_ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>

extern unsigned long g_millis;
inline unsigned long millis() { return g_millis++; }
inline void delay(unsigned long ms) { g_millis += ms; }

#define DEC 10
#define HEX 16

class String {
  public:
    String() {}
    String(const char *s) : s_(s ? s : "") {}
    String(const std::string &s) : s_(s) {}
    const char *c_str() const { return s_.c_str(); }
    unsigned int length() const { return (unsigned int) s_.size(); }
    String &operator+=(char c) { s_ += c; return *this; }
    String &operator+=(const String &o) { s_ += o.s_; return *this; }
    bool operator==(const char *o) const { return s_ == o; }
    bool operator==(const String &o) const { return s_ == o.s_; }
  private:
    std::string s_;
};

class Print {
  private:
    int write_error;
    size_t printNumber(unsigned long n, uint8_t base) {
      char buf[8 * sizeof(long) + 1];
      char *str = &buf[sizeof(buf) - 1];
      *str = '\0';
      if (base < 2) base = 10;
      do {
        char c = (char) (n % base);
        n /= base;
        *--str = c < 10 ? (char) (c + '0') : (char) (c + 'A' - 10);
      } while (n);
      return write(str);
    }
  protected:
    void setWriteError(int err = 1) { write_error = err; }
  public:
    Print() : write_error(0) {}
    virtual ~Print() {}
    int getWriteError() { return write_error; }
    void clearWriteError() { setWriteError(0); }

    virtual size_t write(uint8_t) = 0;
    size_t write(const char *str) {
      if (str == NULL) return 0;
      return write((const uint8_t *) str, strlen(str));
    }
    virtual size_t write(const uint8_t *buffer, size_t size) {
      size_t n = 0;
      while (size--) {
        if (write(*buffer++)) n++;
        else break;
      }
      return n;
    }
    size_t write(const char *buffer, size_t size) { return write((const uint8_t *) buffer, size); }
    virtual int availableForWrite() { return 0; }

    size_t print(const String &s) { return write(s.c_str(), s.length()); }
    size_t print(const char str[]) { return write(str); }
    size_t print(char c) { return write((uint8_t) c); }
    size_t print(unsigned char b, int base = DEC) { return print((unsigned long) b, base); }
    size_t print(int n, int base = DEC) { return print((long) n, base); }
    size_t print(unsigned int n, int base = DEC) { return print((unsigned long) n, base); }
    size_t print(long n, int base = DEC) {
      if (base == 10 && n < 0) {
        size_t t = print('-');
        return printNumber(0UL - (unsigned long) n, 10) + t;
      }
      return printNumber((unsigned long) n, (uint8_t) base);
    }
    size_t print(unsigned long n, int base = DEC) { return printNumber(n, (uint8_t) base); }

    size_t println() { return write("\r\n"); }
    template <typename T> size_t println(T v) { size_t n = print(v); return n + println(); }
    template <typename T> size_t println(T v, int base) { size_t n = print(v, base); return n + println(); }

    virtual void flush() {}
};

enum LookaheadMode { SKIP_ALL, SKIP_NONE, SKIP_WHITESPACE };
#define NO_IGNORE_CHAR '\x01'

class Stream : public Print {
  protected:
    unsigned long _timeout;
    unsigned long _startMillis;
    int timedRead() {
      int c;
      _startMillis = millis();
      do {
        c = read();
        if (c >= 0) return c;
      } while (millis() - _startMillis < _timeout);
      return -1;
    }
    int timedPeek() {
      int c;
      _startMillis = millis();
      do {
        c = peek();
        if (c >= 0) return c;
      } while (millis() - _startMillis < _timeout);
      return -1;
    }
    int peekNextDigit(LookaheadMode lookahead, bool detectDecimal) {
      int c;
      while (1) {
        c = timedPeek();
        if (c < 0 || c == '-' || (c >= '0' && c <= '9') || (detectDecimal && c == '.')) return c;
        switch (lookahead) {
          case SKIP_NONE: return -1;
          case SKIP_WHITESPACE:
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return -1;
            break;
          case SKIP_ALL: break;
        }
        read();
      }
    }
  public:
    Stream() : _timeout(1000), _startMillis(0) {}
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;

    void setTimeout(unsigned long timeout) { _timeout = timeout; }
    unsigned long getTimeout() { return _timeout; }

    // Stream::find() through findMulti() with one target (same result for a plain target)
    bool find(const char *target) {
      size_t len = strlen(target), idx = 0;
      if (len == 0) return true;
      int c;
      while ((c = timedRead()) >= 0) {
        if (c == target[idx]) {
          if (++idx == len) return true;
        } else {
          idx = (c == target[0]) ? 1 : 0;
        }
      }
      return false;
    }

    long parseInt(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR) {
      bool isNegative = false;
      long value = 0;
      int c = peekNextDigit(lookahead, false);
      if (c < 0) return 0;
      do {
        if (c == ignore) {
        } else if (c == '-') {
          isNegative = true;
        } else if (c >= '0' && c <= '9') {
          value = value * 10 + c - '0';
        }
        read();
        c = timedPeek();
      } while ((c >= '0' && c <= '9') || c == ignore);
      return isNegative ? -value : value;
    }

    size_t readBytes(char *buffer, size_t length) {
      size_t count = 0;
      while (count < length) {
        int c = timedRead();
        if (c < 0) break;
        *buffer++ = (char) c;
        count++;
      }
      return count;
    }
    size_t readBytes(uint8_t *buffer, size_t length) { return readBytes((char *) buffer, length); }

    String readString() {
      String ret;
      int c = timedRead();
      while (c >= 0) {
        ret += (char) c;
        c = timedRead();
      }
      return ret;
    }
    String readStringUntil(char terminator) {
      String ret;
      int c = timedRead();
      while (c >= 0 && c != terminator) {
        ret += (char) c;
        c = timedRead();
      }
      return ret;
    }
};

// Serial writes to stdout and never has input. '\r' is dropped, so the output of an example
// compares with a plain text file on any host.
class HostSerial : public Stream {
  public:
    void begin(unsigned long) {}
    operator bool() const { return true; }
    virtual size_t write(uint8_t c) {
      if (c == '\r') return 1;
      return fputc(c, stdout) == EOF ? 0 : 1;
    }
    using Print::write;
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual int peek() { return -1; }
};
extern HostSerial Serial;

#endif // BUFFERSTREAM_HOST_ARDUINO_H
