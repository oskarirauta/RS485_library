/* RS485 helper functions
 
 written by Oskari Rauta */

#include <stdio.h>
#include "util.h"

byte rs485checksumByte(unsigned char *str) {
  unsigned char result = 0;
  unsigned char b;
  for ( b = 0; b < strlen((char *)str); b++ )
    result += str[b];
  return result;
}

boolean rs485valFrom(uint16 *dest, unsigned char *str, unsigned char offset) {
  unsigned char b;
  uint16 _dest = 0;

  for ( b = 0; b < strlen((char *)str) - offset; b++ )
    if (( str[b+offset] < 48 ) || ( str[b+offset] > 57 ))
      return false;
    else
      _dest = ( b > 0 ? _dest * 10 : 0 ) + str[b+offset] - 48;
  *dest = _dest;
  return true;
}

boolean rs485longFrom(uint32 *dest, unsigned char *str, unsigned char offset) {
  unsigned char b;
  uint32 _dest = 0;

  for ( b = 0; b < strlen((char *)str) - offset; b++ )
    if (( str[b+offset] < 48 ) || ( str[b+offset] > 57 ))
      return false;
    else
      _dest = ( b > 0 ? _dest * 10 : 0 ) + str[b+offset] - 48;
  *dest = _dest;
  return true;
}

boolean rs485vals(uint16 *dest1, uint16 *dest2, unsigned char *str, unsigned char offset) {
  uint16 _dest1, _dest2;
  _dest1 = _dest2 = 0;

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (1)
    if (( str[offset] < 48 ) || ( str[offset] > 57 ))
      break;
    else
      _dest1 = ( _dest1 * 10 ) + ( str[offset++] - 48 );

  if ( str[offset++] != ',' )
    return false; // Wrong format?

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (1)
    if (( str[offset] < 48 ) || ( str[offset] > 57 ))
      break;
    else
      _dest2 = ( _dest2 * 10 ) + ( str[offset++] - 48 );

  *dest1 = _dest1;
  *dest2 = _dest2;

  return true;
}

boolean rs485longs(uint32 *dest1, uint32 *dest2, unsigned char *str, unsigned char offset) {
  uint32 _dest1, _dest2;
  _dest1 = _dest2 = 0;

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (1)
    if (( str[offset] < 48 ) || ( str[offset] > 57 ))
      break;
    else
      _dest1 = ( _dest1 * 10 ) + ( str[offset++] - 48 );

  if ( str[offset++] != ',' )
    return false; // Wrong format?

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (1)
    if (( str[offset] < 48 ) || ( str[offset] > 57 ))
      break;
    else
      _dest2 = ( _dest2 * 10 ) + ( str[offset++] - 48 );

  *dest1 = _dest1;
  *dest2 = _dest2;

  return true;
}