#ifndef __UTF8_H__
#define __UTF8_H__

// 'to' must be at least 5 bytes long
int utf16_to_utf8(unsigned int from, char* to);

// 'to' must be at least len * 5 long
int utf16_to_utf8_str(unsigned short* from, int len, char* to);

// check if characters are in UTF-16 encoding
bool is_utf16(unsigned short* from, int len);

#endif
