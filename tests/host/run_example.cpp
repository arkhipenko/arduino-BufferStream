// Runs one example's setup() on the host model. The Makefile passes the sketch path in
// EXAMPLE_INO and compares the output with expected/<Example>.txt.
#include <Arduino.h>

unsigned long g_millis = 0;
HostSerial Serial;

#include EXAMPLE_INO

int main() {
  setup();
  return 0;
}
