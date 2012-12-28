#ifndef __RS485_DATATYPES_H__
#define __RS485_DATATYPES_H__

#ifndef byte
#define byte unsigned char
#endif

#ifndef boolean
typedef enum {
  bFalse = 0 , bTrue = 1} 
tBool;
#define boolean tBool
#define false bFalse
#define true bTrue
#endif


#ifndef errType
typedef enum {
  ok, timeout, checksum, format, refused} 
errType;
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

#ifndef uint32
#define uint32 unsigned int
#endif

#ifndef int32
#define int32 int
#endif

#endif