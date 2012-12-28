#ifndef __RS485_UTIL_H__
#define __RS485_UTIL_H__

#include <string.h>
#include "dataTypes.h"

/* RS485 library ( Master version )
 
 written by Oskari Rauta
 */

/* Helper functions */

#ifdef __cplusplus
extern "C" {
#endif

  extern byte rs485checksumByte(unsigned char *str);
  extern boolean rs485valFrom(uint16 *dest, unsigned char *str, unsigned char offset);
  extern boolean rs485longFrom(uint32 *dest, unsigned char *str, unsigned char offset);
  extern boolean rs485vals(uint16 *dest1, uint16 *dest2, unsigned char *str, unsigned char offset);
  extern boolean rs485longs(uint32 *dest1, uint32 *dest2, unsigned char *str, unsigned char offset);

#ifdef __cplusplus
}
#endif

#endif