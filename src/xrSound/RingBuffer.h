#pragma once

// Original code:
// https://github.com/altmp/alt-voice/blob/master/src/CRingBuffer.h

template<typename T, size_t BufferSize>
class CRingBuffer
{
	T _buffer[BufferSize] = { 0 };
	size_t _writeCursor = 0;
	size_t _readCursor = 0;
public:
	CRingBuffer()
	{
	}

	~CRingBuffer()
	{
	}

	void Write(const T* buffer, size_t count)
	{

		if ((_writeCursor + count) > (_readCursor + BufferSize))
		{
			//Buffer autoclear on overflow
			_writeCursor = 0;
			_readCursor = 0;

			if (count > BufferSize)
				count = BufferSize;
		}

		size_t _writeCursorDivided = _writeCursor % BufferSize;
		if ((_writeCursorDivided + count) > BufferSize)
		{
			size_t writeTail = BufferSize - _writeCursorDivided;
			if (writeTail != 0)
				memcpy(&_buffer[_writeCursorDivided], &buffer[0], writeTail * sizeof(T));

			size_t writeHead = count - writeTail;
			if (writeHead != 0)
				memcpy(&_buffer[0], &buffer[writeTail], writeHead * sizeof(T));
		}
		else
			memcpy(&_buffer[_writeCursorDivided], buffer, count * sizeof(T));
		_writeCursor += count;
	}

	size_t Read(T* buffer, size_t count)
	{
		size_t _readCursorDivided = _readCursor % BufferSize;
		if ((_readCursor + count) > _writeCursor)
			count = _writeCursor - _readCursor;

		if ((_readCursorDivided + count) > BufferSize)
		{
			size_t readTail = BufferSize - _readCursorDivided;
			if (readTail != 0)
				memcpy(&buffer[0], &_buffer[_readCursorDivided], readTail * sizeof(T));

			size_t readHead = count - readTail;
			if (readHead != 0)
				memcpy(&buffer[readTail], &_buffer[0], readHead * sizeof(T));
		}
		else
			memcpy(buffer, &_buffer[_readCursorDivided], count * sizeof(T));

		_readCursor += count;
		return count;
	}

	size_t BytesToRead()
	{
		size_t toRead = _writeCursor - _readCursor;
		return toRead;
	}

	bool IsHalfFull()
	{
		bool halfFull = (_writeCursor - _readCursor) > (BufferSize / 2);
		return halfFull;
	}

	float GetFillLevel()
	{
		float fillLevel = (_writeCursor - _readCursor) / (float)BufferSize;
		return fillLevel;
	}
};