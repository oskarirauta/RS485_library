#ifndef __DATATYPES_H__
#define __DATATYPES_H__

#ifndef byte
#define byte unsigned char
#endif

#ifndef boolean
typedef enum {false, true} boolean;
#endif

#ifndef errType
typedef enum {ok, timeout, checksum} errType;
#endif

#ifndef uint8
#define uint8 unsigned char
#endif

#ifndef int8
#define int8 char
#endif

#ifndef uint16
#define uint16 unsigned short
#endif

#ifndef int16
#define int16 short
#endif

#endif
