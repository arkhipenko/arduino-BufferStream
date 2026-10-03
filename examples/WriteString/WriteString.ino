// BufferStream library example: build a C string with print()
// by Industrial Shields

#include <BufferStream.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600UL);

  // Create stream from data
  char data[100];
  WriteBufferStream stream(data, sizeof(data));

  // Add data to stream
  stream.print("Hello");
  stream.print(' ');
  stream.print("WriteStream!");

  // Add the '\0' character at the end of the stream. write() returns 0 when the buffer is
  // full, and then data holds no complete C string.
  if (stream.write(uint8_t('\0')) != 1) {
    Serial.println("Buffer too small for the text and its terminator");
    return;
  }

  // Print data: Hello WriteStream!
  Serial.println(data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Nothing to do
}
