#ifndef __ReadBufferStream_H__
#define __ReadBufferStream_H__

#include "BaseBufferStream.h"

class ReadBufferStream : public BaseBufferStream {
	public:
		// The buffer is never written. The stream timeout is set to 0, because a buffer
		// cannot receive more data (readBytes() and parseInt() return at the end at once).
		explicit ReadBufferStream(const void *buff, size_t len);

	public:
		virtual int available();
		virtual int read();
		virtual int peek();
};

#endif
