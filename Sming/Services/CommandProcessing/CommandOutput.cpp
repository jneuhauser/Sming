/*
 * CommandOutput.cpp
 *
 *  Created on: 5 jul. 2015
 *      Author: Herman
 */

#include "CommandOutput.h"
#include <debug_progmem.h>

CommandOutput::CommandOutput(Stream* reqStream) : outputStream(reqStream)
{
}

CommandOutput::~CommandOutput()
{
	debugf("destruct");
}

size_t CommandOutput::write(uint8_t outChar)
{
	uint8_t outBuf[1] = {uint8_t(outChar)};
	return write(outBuf, 1);
}

size_t CommandOutput::write(const uint8_t* buffer, size_t size)
{
	if (buffer == nullptr) {
		return 0;
	}
	
	if(outputStream) {
		return outputStream->write(buffer, size);
	}

#ifndef DISABLE_NETWORK
	if(outputTcpClient) {
		return outputTcpClient->write(reinterpret_cast<const char*>(buffer), size);
	}
	if(outputSocket && outputSocket->send(reinterpret_cast<const char*>(buffer), size, WS_FRAME_TEXT)) {
		return size;
	}
#endif

	return 0;
}
/*
size_t CommandOutput::write(const uint8_t* buffer, size_t size)
{
	size_t n = 0;
	while(size--) {
		n += write(*buffer++);
	}
	return n;
}
*/