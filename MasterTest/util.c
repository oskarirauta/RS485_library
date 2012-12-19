#include <string.h>

unsigned char checksumByte(unsigned char *s) {
	unsigned char b;
	unsigned char result = 0;

	for ( b = 0; b < strlen(s); b++ )
			result += s[b];

	return result;
}

