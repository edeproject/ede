#include "Utf8.h"

int utf16_to_utf8(unsigned int from, char* to) {
	if(from < 0x000080) {
		to[0] = from;
		return 1;
	} else if(from < 0x000800) {
		to[0] = 0xC0 | (from >> 6);
		to[1] = 0x80 | (from & 0x3F);
		return 2;
	} else if(from < 0x010000) {
		to[0] = 0xE0 | (from >> 12);
		to[1] = 0x80 | ((from >> 6) & 0x3F);
		to[2] = 0x80 | (from & 0x3F);
		return 3;
	} else if(from < 0x00200000) {
		to[0] = 0xF0 | (from >> 18);
		to[1] = 0x80 | ((from >> 12) & 0x3F);
		to[2] = 0x80 | ((from >> 6) & 0x3F);
		to[3] = 0x80 | (from & 0x3F);
		return 4;
	} else if(from < 0x01000000) {
		to[0] = 0xF8 | (from >> 24);
		to[1] = 0x80 | ((from >> 18) & 0x3F);
		to[2] = 0x80 | ((from >> 12) & 0x3F);
		to[3] = 0x80 | ((from >> 6) & 0x3F);
		to[4] = 0x80 | (from & 0x3F);
		return 5;
	}
	to[0] = '?';
	return -1;
}

int utf16_to_utf8_str(unsigned short* from, int len, char* to) {
	int j = 0;
	for(int i = 0; i < len; i++) {
		int s = utf16_to_utf8((unsigned int)from[i], to + j);
		if(s < 1) 
			j += 1;
		else
			j += s;
	}
	return j;
}

bool is_utf16(unsigned short* from, int len) {
	return !(from[0] < 0x000080);
}
