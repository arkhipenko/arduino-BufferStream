// BufferStream library example: read a String through the Stream interface
// by Industrial Shields

#include <BufferStream.h>

String data("This is the string!");

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  // Create stream from data (a const buffer is accepted as is)
  ReadBufferStream stream(data.c_str(), data.length());

  // Print data from the stream
  Serial.begin(9600UL);
  while (stream.available()) {
    Serial.print((char) stream.read());
  }
  Serial.println();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Nothing to do
}
