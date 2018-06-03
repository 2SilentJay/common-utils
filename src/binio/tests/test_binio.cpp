#include <iostream>
#include <cstdio>

#include "TestMemArea.h"
#include "TestPacket.h"
#include "TestPacketSafe.h"
#include "TestStaticArray.h"

using namespace binio;

int main_binio(int, char**) {

	TestByteBuffer byte_buffer;
	byte_buffer.test();

	TestPacket packet;
	packet.test();

	TestPacketSafe packet_safe;
	packet_safe.test();
	
	TestStaticArray static_array;
	static_array.test();

	std::cout << "<---- the end of main_binio() ---->\n";
	return 0;
}


