#ifndef __WriteBufferStream_H__
#define __WriteBufferStream_H__

#include "BaseBufferStream.h"

class WriteBufferStream : public BaseBufferStream {
	public:
		explicit WriteBufferStream(void *buff, size_t len);

	public:
		// A write that does not fit stores what fits, returns that count and sets the
		// write error (getWriteError() != 0). No '\0' terminator is added.
		virtual size_t write(uint8_t value);
		virtual size_t write(const uint8_t *buffer, size_t size);
		virtual int availableForWrite();	// free space, capped at INT_MAX

		size_t written() const;	// bytes stored since construction

		using Print::write;

	protected:
		size_t used;
};

#endif
