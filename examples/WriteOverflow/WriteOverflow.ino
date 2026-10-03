// BufferStream library example: track the space left and detect a buffer that is too small
//
// written() returns the bytes stored, availableForWrite() the space left. A write that does
// not fit stores what fits and sets the write error (getWriteError()).

#include <BufferStream.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600UL);

  char line[12];
  WriteBufferStream stream(line, sizeof(line) - 1);  // keep one byte for the '\0'

  stream.print("temp=");
  stream.print(21);
  Serial.print("written: ");
  Serial.println((unsigned long) stream.written());  // written: 7
  Serial.print("free: ");
  Serial.println(stream.availableForWrite());        // free: 4

  stream.print(" Celsius");  // does not fit: only " Cel" is stored
  line[stream.written()] = '\0';

  if (stream.getWriteError()) {
    Serial.print("truncated: ");
  }
  Serial.println(line);  // truncated: temp=21 Cel
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Nothing to do
}
