#ifndef __BaseBufferStream_H__
#define __BaseBufferStream_H__

#include <Arduino.h>

class BaseBufferStream : public Stream {
	public:
		explicit BaseBufferStream(void *buff, size_t len);

	public:
		virtual size_t write(uint8_t) { return 0; }
		virtual int available() { return 0; }
		virtual int read() { return -1; }
		virtual int peek() { return -1; }
		virtual void flush() {}

		using Print::write;

	protected:
		uint8_t *buff;	// current position
		size_t len;		// bytes left from the current position
};

#endif // __BaseBufferStream_H__
