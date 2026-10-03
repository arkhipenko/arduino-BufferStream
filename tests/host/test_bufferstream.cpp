// Host tests for BufferStream. Build and run: make -C tests/host
// Run one test: ./test_bufferstream <test name>
//
// Defect ids (D1-D4) and improvement ids (I1-I4) refer to README.md, version history 1.1.0.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <string>

#include <BufferStream.h>

unsigned long g_millis = 0;
HostSerial Serial;

static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { failures++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
  } while (0)

#define CHECK_EQ(a, b) do { \
    long long va_ = (long long) (a), vb_ = (long long) (b); \
    if (va_ != vb_) { failures++; printf("  FAIL %s:%d: %s == %s (%lld != %lld)\n", __FILE__, __LINE__, #a, #b, va_, vb_); } \
  } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
// Read side

static void test_read_basic() {
  const uint8_t src[3] = {'a', 'b', 0xFF};
  ReadBufferStream s(src, sizeof src);
  CHECK_EQ(s.available(), 3);
  CHECK_EQ(s.peek(), 'a');
  CHECK_EQ(s.read(), 'a');
  CHECK_EQ(s.available(), 2);
  CHECK_EQ(s.read(), 'b');
  CHECK_EQ(s.peek(), 0xFF);   // a 0xFF byte is 255, not -1
  CHECK_EQ(s.read(), 0xFF);
  CHECK_EQ(s.available(), 0);
  CHECK_EQ(s.read(), -1);
  CHECK_EQ(s.peek(), -1);
  CHECK_EQ(s.write((uint8_t) 'x'), 0);   // a read stream accepts no output
  CHECK_EQ(s.print("xyz"), 0);
}

static void test_read_empty() {
  ReadBufferStream s(NULL, 0);
  CHECK_EQ(s.available(), 0);
  CHECK_EQ(s.read(), -1);
  CHECK_EQ(s.peek(), -1);
}

// D1, D2: every common buffer type is accepted without a cast. With the 1.0.0 signatures this
// file does not compile (void * rejects const buffers, uint8_t * rejects char buffers).
static void test_ctor_buffer_types() {
  const char *cstr = "cs";
  char carr[] = "ca";
  uint8_t uarr[] = {'u', 'a'};
  const uint8_t curr[] = {'c', 'u'};
  String str("st");

  ReadBufferStream r1(cstr, 2);
  ReadBufferStream r2(carr, 2);
  ReadBufferStream r3(uarr, 2);
  ReadBufferStream r4(curr, 2);
  ReadBufferStream r5(str.c_str(), str.length());
  ReadBufferStream r6((uint8_t *) str.c_str(), str.length());   // Dictionary 3.x call form
  CHECK_EQ(r1.read(), 'c');
  CHECK_EQ(r2.read(), 'c');
  CHECK_EQ(r3.read(), 'u');
  CHECK_EQ(r4.read(), 'c');
  CHECK_EQ(r5.read(), 's');
  CHECK_EQ(r6.read(), 's');

  char wc[4];
  uint8_t wu[4];
  WriteBufferStream w1(wc, sizeof wc);
  WriteBufferStream w2(wu, sizeof wu);
  CHECK_EQ(w1.print("ab"), 2);
  CHECK_EQ(w2.print("cd"), 2);
  CHECK(memcmp(wc, "ab", 2) == 0);
  CHECK(memcmp(wu, "cd", 2) == 0);
}

// D3: reading past the end returns at once. In 1.0.0 each call below waited the full Stream
// timeout (1000 ms).
static void test_read_past_end_no_wait() {
  const char src[] = "abcd";
  {
    ReadBufferStream s(src, 4);
    char out[10];
    unsigned long t0 = g_millis;
    CHECK_EQ(s.readBytes(out, sizeof out), 4);
    CHECK(memcmp(out, "abcd", 4) == 0);
    CHECK(g_millis - t0 < 20);
  }
  {
    ReadBufferStream s(src, 4);
    unsigned long t0 = g_millis;
    CHECK(s.readString() == "abcd");
    CHECK(g_millis - t0 < 20);
  }
  {
    ReadBufferStream s(src, 4);
    unsigned long t0 = g_millis;
    CHECK(s.readStringUntil(';') == "abcd");
    CHECK(g_millis - t0 < 20);
  }
  {
    ReadBufferStream s(src, 4);
    unsigned long t0 = g_millis;
    CHECK(!s.find("zz"));
    CHECK(g_millis - t0 < 20);
  }
  {
    const char num[] = "x=-42";
    ReadBufferStream s(num, 5);
    unsigned long t0 = g_millis;
    CHECK_EQ(s.parseInt(), -42);
    CHECK_EQ(s.parseInt(), 0);   // nothing left
    CHECK(g_millis - t0 < 20);
  }
}

// D3: a caller can still choose a timeout.
static void test_read_timeout_settable() {
  ReadBufferStream s("a", 1);
  s.setTimeout(50);
  char out[4];
  unsigned long t0 = g_millis;
  CHECK_EQ(s.readBytes(out, sizeof out), 1);
  CHECK(g_millis - t0 >= 50);
}

// D4: available() never goes negative. The buffer is not read, so a dummy pointer is enough.
static void test_available_clamped() {
  static const uint8_t dummy = 0;
  size_t big = (size_t) INT_MAX + 10;
  ReadBufferStream r(&dummy, big);
  CHECK_EQ(r.available(), INT_MAX);
  ReadBufferStream exact(&dummy, (size_t) INT_MAX);
  CHECK_EQ(exact.available(), INT_MAX);

  static uint8_t wdummy = 0;
  WriteBufferStream w(&wdummy, big);
  CHECK_EQ(w.availableForWrite(), INT_MAX);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Write side

static void test_write_basic() {
  char buf[16];
  memset(buf, '#', sizeof buf);
  WriteBufferStream w(buf, sizeof buf);
  CHECK_EQ(w.print("Hello"), 5);
  CHECK_EQ(w.print(' '), 1);
  CHECK_EQ(w.print(-12), 3);
  CHECK_EQ(w.write((uint8_t) '\0'), 1);
  CHECK(strcmp(buf, "Hello -12") == 0);
  CHECK_EQ(buf[10], '#');   // nothing written past the data
  CHECK_EQ(w.getWriteError(), 0);
  CHECK_EQ(w.available(), 0);
  CHECK_EQ(w.read(), -1);
  CHECK_EQ(w.peek(), -1);
}

// I1: written() and availableForWrite() track the buffer.
static void test_write_counters() {
  uint8_t buf[8];
  WriteBufferStream w(buf, sizeof buf);
  CHECK_EQ(w.written(), 0);
  CHECK_EQ(w.availableForWrite(), 8);
  w.print("abc");
  CHECK_EQ(w.written(), 3);
  CHECK_EQ(w.availableForWrite(), 5);
  w.print("defgh");
  CHECK_EQ(w.written(), 8);
  CHECK_EQ(w.availableForWrite(), 0);
}

// I2: an overflow sets the write error and stores what fits.
static void test_write_overflow_sets_error() {
  uint8_t *buf = new uint8_t[5];   // heap block of exactly 5 bytes: ASan catches an overrun
  WriteBufferStream w(buf, 5);
  CHECK_EQ(w.print("Hello world"), 5);
  CHECK(memcmp(buf, "Hello", 5) == 0);
  CHECK(w.getWriteError() != 0);
  CHECK_EQ(w.written(), 5);
  w.clearWriteError();
  CHECK_EQ(w.getWriteError(), 0);
  CHECK_EQ(w.write((uint8_t) '!'), 0);   // single byte into a full buffer
  CHECK(w.getWriteError() != 0);
  CHECK_EQ(w.written(), 5);
  delete[] buf;
}

static void test_write_exact_fit_no_error() {
  uint8_t *buf = new uint8_t[5];
  WriteBufferStream w(buf, 5);
  CHECK_EQ(w.print("12345"), 5);
  CHECK_EQ(w.getWriteError(), 0);
  CHECK_EQ(w.write((const uint8_t *) "", 0), 0);   // empty write into a full buffer is fine
  CHECK_EQ(w.getWriteError(), 0);
  delete[] buf;
}

static void test_write_empty_buffer() {
  WriteBufferStream w(NULL, 0);
  CHECK_EQ(w.availableForWrite(), 0);
  CHECK_EQ(w.write((uint8_t) 'a'), 0);
  CHECK(w.getWriteError() != 0);
  CHECK_EQ(w.written(), 0);
}

// I3: bulk write copies what fits in one call and keeps the position for the next one.
static void test_write_bulk() {
  uint8_t *buf = new uint8_t[10];
  WriteBufferStream w(buf, 10);
  const uint8_t a[4] = {1, 2, 3, 4};
  const uint8_t b[8] = {5, 6, 7, 8, 9, 10, 11, 12};
  CHECK_EQ(w.write(a, sizeof a), 4);
  CHECK_EQ(w.getWriteError(), 0);
  CHECK_EQ(w.write(b, sizeof b), 6);
  CHECK(w.getWriteError() != 0);
  const uint8_t expect[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  CHECK(memcmp(buf, expect, 10) == 0);
  CHECK_EQ(w.written(), 10);
  CHECK_EQ(w.write(b, sizeof b), 0);
  delete[] buf;
}

// The output of a write stream parses back through a read stream.
static void test_round_trip() {
  char buf[32];
  WriteBufferStream w(buf, sizeof buf);
  w.print("t=");
  w.print(1234);
  w.print(";h=");
  w.print(56);
  ReadBufferStream r(buf, w.written());
  CHECK_EQ(r.parseInt(), 1234);
  CHECK_EQ(r.parseInt(), 56);
  CHECK_EQ(r.available(), 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// I4: the header version matches the manifests and the README.

static std::string slurp(const char *rel) {
  std::string path = std::string(REPO_ROOT) + "/" + rel;
  std::ifstream f(path.c_str(), std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  std::string s = ss.str();
  std::string out;   // normalize CRLF
  for (size_t i = 0; i < s.size(); i++) if (s[i] != '\r') out += s[i];
  if (out.empty()) printf("  cannot read %s\n", path.c_str());
  return out;
}

static bool contains(const std::string &hay, const std::string &needle) {
  bool ok = hay.find(needle) != std::string::npos;
  if (!ok) printf("  missing: \"%s\"\n", needle.c_str());
  return ok;
}

static void test_version_consistent() {
  const std::string v = BUFFERSTREAM_VERSION_STRING;
  int major = 0, minor = 0, patch = 0;
  CHECK_EQ(sscanf(v.c_str(), "%d.%d.%d", &major, &minor, &patch), 3);
  CHECK_EQ(BUFFERSTREAM_VERSION, major * 10000 + minor * 100 + patch);
  CHECK(contains(slurp("library.properties"), "\nversion=" + v + "\n"));
  CHECK(contains(slurp("library.json"), "\"version\": \"" + v + "\""));
  const std::string readme = slurp("README.md");
  CHECK(contains(readme, "\nVersion: " + v + "\n"));
  CHECK(contains(readme, "\n### " + v + " "));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

struct TestCase {
  const char *name;
  void (*fn)();
};

#define TEST(f) { #f, f }
static const TestCase tests[] = {
  TEST(test_read_basic),
  TEST(test_read_empty),
  TEST(test_ctor_buffer_types),
  TEST(test_read_past_end_no_wait),
  TEST(test_read_timeout_settable),
  TEST(test_available_clamped),
  TEST(test_write_basic),
  TEST(test_write_counters),
  TEST(test_write_overflow_sets_error),
  TEST(test_write_exact_fit_no_error),
  TEST(test_write_empty_buffer),
  TEST(test_write_bulk),
  TEST(test_round_trip),
  TEST(test_version_consistent),
};

int main(int argc, char **argv) {
  const char *only = argc > 1 ? argv[1] : NULL;
  int run = 0;
  for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
    if (only && strcmp(only, tests[i].name) != 0) continue;
    int before = failures;
    tests[i].fn();
    printf("%s %s\n", failures == before ? "pass" : "FAIL", tests[i].name);
    run++;
  }
  if (run == 0) {
    printf("no test named %s\n", only ? only : "");
    return 2;
  }
  printf("%d test(s), %d failed check(s)\n", run, failures);
  return failures ? 1 : 0;
}
