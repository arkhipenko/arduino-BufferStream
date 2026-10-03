// BufferStream library example: parse numbers and text out of a memory buffer
//
// A ReadBufferStream has its timeout set to 0. parseInt(), readBytes() and readStringUntil()
// return at once at the end of the buffer instead of waiting for data that cannot arrive.

#include <BufferStream.h>

const char record[] = "id=17;temp=-4;name=probe";

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600UL);

  ReadBufferStream stream(record, sizeof(record) - 1);  // without the '\0'

  long id = stream.parseInt();    // skips "id=" and reads 17
  long temp = stream.parseInt();  // skips ";temp=" and reads -4
  stream.readStringUntil('=');    // skips ";name="

  char name[16];
  size_t n = stream.readBytes(name, sizeof(name) - 1);  // the rest: "probe"
  name[n] = '\0';

  Serial.print("id: ");
  Serial.println(id);           // id: 17
  Serial.print("temp: ");
  Serial.println(temp);         // temp: -4
  Serial.print("name: ");
  Serial.println(name);         // name: probe
  Serial.print("left: ");
  Serial.println(stream.available());  // left: 0

  // Nothing is left. parseInt() returns 0 without waiting.
  long none = stream.parseInt();
  Serial.print("parseInt at end: ");
  Serial.println(none);         // parseInt at end: 0
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Nothing to do
}
