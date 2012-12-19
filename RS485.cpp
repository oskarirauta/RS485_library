/* RS485 library

written by Oskari Rauta
*/

#include "RS485.h"

byte rs485checksumByte(char *str) {
  byte result = 0;
  for ( byte b = 0; b < strlen(str); b++ )
    result += str[b];
  return result;
}

uint16 rs485valFrom(char *str, byte offset) {
  uint16 result = 0;
  for ( byte b = 0; b < strlen(str) - offset; b++ )
  {
    if (( str[b+offset] < 48 ) || ( str[b+offset] > 57 ))
      return -1;
    result = (( result * 10 ) + (str[b+offset] - 48 ));
  }
  return result;
}
  
RS485::RS485(uint8 ser, uint8 rtsPin) {
  serNo = ser;
  switch (serNo) {
    case 2:
    	hwSerial = &Serial2;
    	break;
    case 3:
    	hwSerial = &Serial3;
    	break;
    #if defined(STM32_HIGH_DENSITY) && !defined(BOARD_maple_RET6)
    case 4:
    	hwSerial = &Serial4;
    	break;
    case 5:
    	hwSerial = &Serial5;
    	break;
    #endif
    default: // 1
    	hwSerial = &Serial1;
    	break;
  }
  _rts = rtsPin;
  pinMode(_rts, OUTPUT);
  digitalWrite(_rts, LOW);
}
  
void RS485::waitForSend(void) {
  switch (serNo) {
    case 2:
  		while ( (USART2_BASE -> SR & USART_SR_TC) == 0);
    	break;
    case 3:
  		while ( (USART3_BASE -> SR & USART_SR_TC) == 0);
    	break;
    #if defined(STM32_HIGH_DENSITY) && !defined(BOARD_maple_RET6)
    case 4:
  		while ( (USART4_BASE -> SR & USART_SR_TC) == 0);
    	break;
    case 5:
  		while ( (USART5_BASE -> SR & USART_SR_TC) == 0);
    	break;
    #endif
    default:
  		while ( (USART1_BASE -> SR & USART_SR_TC) == 0);
    	break;
  }
}

void RS485::begin(uint32 baud) {
  pinMode(_rts, OUTPUT);
  digitalWrite(_rts, LOW);
  
  hwSerial -> begin(baud);
  waitForSend();

  delay(RS485_SETTLETIME);
  hwSerial -> flush();
  conState = 0;
  receivedChecksum = 0;
  connected = false;
  phoneNO = RS485_DEFAULT_PHONENO;
  blockSize = RS485_DEFAULT_BLOCKSIZE;
}

void RS485::end(void) {
  hwSerial -> end();
}

uint32 RS485::available(void) {
  return hwSerial -> available();
}

uint8 RS485::read(void) {
  return hwSerial -> read();
}

void RS485::flush(void) {
  hwSerial -> flush();
}

void RS485::write(unsigned char ch) {
  hwSerial -> write(ch);
}

boolean RS485::received(void) {
  if ( ! hwSerial -> available() )
    return false;
  
  byte l;
  
  chCue[0] = chCue[1]; chCue[1] = chCue[2]; chCue[2] = chCue[3];
  chCue[3] = hwSerial -> read();
  
  if (( conState == 1 ) && ( chCue[3] == '(' )) // Command ends, checksum begins
    conState = 2;
  
  if (( conState == 2 ) && ( chCue[3] == ')' )) // Checksum received
    conState = 3;
  
  if (( conState == 2 ) && ( chCue[3] > 47 ) && ( chCue[3] < 58 ))
    receivedChecksum = ( receivedChecksum * 10 ) + ( chCue[3] - 48 );
  
  if ( conState == 1 ) { // Receiving command
    if ( strlen(receivedCom) < RS485_COMMANDSIZE-3 ) {
      l = strlen(receivedCom);
      if (( chCue[3] > 96 ) && ( chCue[3] < 123 ))
        receivedCom[l] = chCue[3] - 32;
      else
        receivedCom[l] = chCue[3];
      receivedCom[l+1] = '\0';
    } else // Command was too long - ignore
      conState = 0;
  }
  
  if (( chCue[0] && chCue[1] && chCue[2] == '+' ) && 
      (( chCue[3] == 'A' ) || ( chCue[3] == 'a' ))) {
    conState = 1;
    receivedCom[0] = 'A';
    receivedCom[1] = '\0';
    receivedChecksum = 0;
  }
  
  if ( conState == 3 ) {
    conState = 0;
    if ( rs485checksumByte(receivedCom) == receivedChecksum )
      return parseCommand();
    else 
      outputMSG((char *)"ERR");
  }
      
  return false;
}

void RS485::outputMSG(char *MSG, const boolean withoutChecksum) {

  if ( RS485_COMMAND_DELAY > 0 )
    delayMicroseconds(RS485_COMMAND_DELAY);
  
  while ( hwSerial -> available() > 0 )
    hwSerial -> read();
  
  if ( withoutChecksum ) {
    digitalWrite(_rts, HIGH);
    hwSerial -> print(MSG);
    waitForSend();
    digitalWrite(_rts, LOW);
    return;
  }
  
  char outputSTR[strlen(MSG)+15];
  sprintf(outputSTR,"%s((%d))\r\n", MSG, rs485checksumByte(MSG));
  digitalWrite(_rts, HIGH);
  hwSerial -> print(outputSTR);
  waitForSend();
  digitalWrite(_rts, LOW);
} 

boolean RS485::parseCommand(void) {
  
  if ( strncmp("ATD", receivedCom, 3 ) == 0 ) {
    uint16 callNo = rs485valFrom(receivedCom, 3);

    if ( callNo != -1 ) {
      if ( callNo == phoneNO ) {
        connected = true;
        outputMSG((char *)"OK");
      } else connected = false; // Master called someone else but hangup wasn't done properly - be smart..
      return false;
    }
  }
  
  if (!connected)
    return false;
  
  if (strcmp("ATZ", receivedCom) == 0) {
    outputMSG((char *)"OK");
    return false;
  }
  
  if ((strcmp("ATH", receivedCom) == 0) || (strcmp("ATH0", receivedCom) == 0)) {
    connected = false; // Does not give a receipt - test with ATZ to be sure..
    return false;
  }
  
  if (strcmp("ATPHONENO", receivedCom) == 0 ) {
    char str[12];
    sprintf(str, "%d", phoneNO);
    outputMSG(str);
    return false;
  }
  
  if (strcmp("ATBLOCKSIZE", receivedCom) == 0 ) {
    char str[15];
    sprintf(str, "%d", blockSize);
    outputMSG(str);
    return false;
  }
  
  if (strncmp("ATPHONENO=", receivedCom, 10) == 0) {
    uint16 newNo = rs485valFrom(receivedCom, 10);
    
    if ( newNo < 0 ) { // Number was not readable - error as checksum error
      outputMSG((char *)"ERR");
      return false;
    }
    
    if (( newNo < RS485_FIRST_PHONENO ) || ( newNo > RS485_LAST_PHONENO )) { // Outside pool
      outputMSG((char *)"NO");
      return false;
    }
    
    phoneNO = newNo;
    outputMSG((char *)"OK");
    return false;
  }
    
  if (strncmp("ATBLOCKSIZE=", receivedCom, 12) == 0) {
    uint16 newBlockSize = rs485valFrom(receivedCom, 12);
    
    if ( newBlockSize < 0 ) { // Number was not readable - error as checksum error
      outputMSG((char *)"ERR");
      return false;
    }
    
    if (( newBlockSize < RS485_MIN_BLOCKSIZE ) || ( newBlockSize > RS485_MAX_BLOCKSIZE )) { // Outside pool
      outputMSG((char *)"NO");
      return false;
    }
    
    blockSize = newBlockSize;
    outputMSG((char *)"OK");
    return false;      
  }
  
  return true; // Not a base command, application handles command.
}

char *RS485::command() {
  return receivedCom;
}