/* RS485_util (part of RS485 library )

written by Oskari Rauta
*/

#include "RS485_util.h"

boolean rs485fileInfo(uint8 &fileType, uint16 &fileID, uint32 &fileSize, char *str, byte offset) {
  fileType = fileID = fileSize = 0;
  
  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric
  
  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    fileType = ( fileType * 10 ) + ( str[offset++] - 48 );
         
  if ( str[offset++] != ',' )
    return false; // Wrong format?

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    fileID = ( fileID * 10 ) + ( str[offset++] - 48 );

  if ( str[offset++] != ',' )
    return false; // Wrong format?

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric
  
  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    fileSize = ( fileSize * 10 ) + ( str[offset++] - 48 );
         
  return true;
}

        