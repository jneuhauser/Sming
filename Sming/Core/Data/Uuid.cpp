/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * Uuid.cpp - Universal Unique Identifier
 *
 * See https://pubs.opengroup.org/onlinepubs/9629399/apdxa.htm.
 *
 * @author mikee47 <mike@sillyhouse.net>
 *
 ****/

#include "Uuid.h"
#include <SystemClock.h>
#include <stringconversion.h>

extern "C" {
uint32_t os_random();
void os_get_random(void* buf, size_t n);
}

bool Uuid::operator==(const Uuid& other) const
{
	// Ensure these are stricly compared as a set of words to avoid PROGMEM issues
	struct S {
		uint32_t a, b, c, d;
	};
	auto& s1 = reinterpret_cast<const S&>(*this);
	auto& s2 = reinterpret_cast<const S&>(other);
	return s1.a == s2.a && s1.b == s2.b && s1.c == s2.c && s1.d == s2.d;
}

bool Uuid::generate(MacAddress mac)
{
	uint8_t version = 1; // DCE version
	uint8_t variant = 2; // DCE variant
	uint16_t clock_seq = os_random();
	uint32_t time;
	if(SystemClock.isSet()) {
		time = SystemClock.now(eTZ_UTC);
	} else {
		time = os_random();
	}
	// Time only provides 32 bits, we need 60
	time_low = (os_random() & 0xFFFFFFFC) | (time & 0x00000003);
	time_mid = (time >> 2) & 0xFFFF;
	time_hi_and_version = (version << 12) | ((time >> 18) << 2);
	clock_seq_hi_and_reserved = (variant << 6) | ((clock_seq >> 8) & 0x3F);
	clock_seq_low = clock_seq & 0xFF;
	mac.getOctets(node);

	return SystemClock.isSet();
}

bool Uuid::generate()
{
	MacAddress::Octets mac;
	os_get_random(mac, sizeof(mac));
	// RFC4122 requires LSB of first octet to be 1
	mac[0] |= 0x01;
	return generate(mac);
}

bool Uuid::decompose(const char* s, size_t len)
{
	if(len != stringSize) {
		return false;
	}

	char* p;
	time_low = strtoul(s, &p, 16);
	if(*p != '-' || p - s != 8) {
		return false;
	}
	s = ++p;

	time_mid = strtoul(s, &p, 16);
	if(*p != '-' || p - s != 4) {
		return false;
	}
	s = ++p;

	time_hi_and_version = strtoul(s, &p, 16);
	if(*p != '-' || p - s != 4) {
		return false;
	}
	s = ++p;

	uint16_t x = strtoul(s, &p, 16);
	if(*p != '-' || p - s != 4) {
		return false;
	}
	clock_seq_hi_and_reserved = x >> 8;
	clock_seq_low = x & 0xff;
	s = ++p;

	for(unsigned i = 0; i < sizeof(node); ++i) {
		uint8_t c = unhex(*s++) << 4;
		c |= unhex(*s++);
		node[i] = c;
	}

	return true;
}

size_t Uuid::toString(char* buffer, size_t bufSize) const
{
	if(isFlashPtr(this)) {
		return Uuid(*this).toString(buffer, bufSize);
	}

	if(buffer == nullptr || bufSize < stringSize) {
		return 0;
	}

	auto set = [&](unsigned offset, uint32_t value, unsigned digits) {
		ultoa_wp(value, &buffer[offset], 16, digits, '0');
	};

	// 2fac1234-31f8-11b4-a222-08002b34c003
	// 0        9    14   19   24          36

	set(0, time_low, 8);
	buffer[8] = '-';
	set(9, time_mid, 4);
	buffer[13] = '-';
	set(14, time_hi_and_version, 4);
	buffer[18] = '-';
	set(19, clock_seq_hi_and_reserved, 2);
	set(21, clock_seq_low, 2);
	buffer[23] = '-';

	unsigned pos = 24;
	for(unsigned i = 0; i < 6; ++i) {
		buffer[pos++] = hexchar(node[i] >> 4);
		buffer[pos++] = hexchar(node[i] & 0x0f);
	}

	return stringSize;
}

String Uuid::toString() const
{
	String s;
	if(s.setLength(stringSize)) {
		toString(s.begin(), stringSize);
	}
	return s;
}
