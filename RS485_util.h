/* RS485_util (part of RS485 library )

written by Oskari Rauta
*/

#ifndef __RS485_util_slave_H__
#define __RS485_util_slave_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
  
#if ARDUINO >= 100
 #include "Arduino.h"
#elif defined(__ARM3X8E__)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

boolean rs485fileInfo(uint8 &fileType, uint16 &fileID, uint32 &fileSize, char *str, byte offset);  

#endif

        