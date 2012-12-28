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

boolean rs485valFrom(uint16 &dest, char *str, byte offset) {
  dest = 0;
  for ( byte b = 0; b < strlen(str) - offset; b++ )
    if (( str[b+offset] < 48 ) || ( str[b+offset] > 57 ))
      return false;
    else 
      dest = ( b > 0 ? dest * 10 : 0 ) + str[b+offset] - 48;
  return true;
}

boolean rs485vals(uint16 &dest1, uint16 &dest2, char *str, byte offset) {
  dest1 = 0; 
  dest2 = 0;

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    dest1 = ( dest1 * 10 ) + ( str[offset++] - 48 );

  if ( str[offset++] != ',' )
    return false; // Wrong format?

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    dest2 = ( dest2 * 10 ) + ( str[offset++] - 48 );

  return true;  
}

boolean rs485longs(uint32 &dest1, uint32 &dest2, char *str, byte offset) {
  dest1 = 0; 
  dest2 = 0;

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    dest1 = ( dest1 * 10 ) + ( str[offset++] - 48 );

  if ( str[offset++] != ',' )
    return false; // Wrong format?

  if (( str[offset] < 48 ) || ( str[offset] > 57 ))
    return false; // Non-numeric

  while (( str[offset] > 47 ) && ( str[offset] < 58 ))
    dest2 = ( dest2 * 10 ) + ( str[offset++] - 48 );

  return true;  
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
  pinMode(_rts, PWM);
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

void RS485::begin(uint32 baud, uint8 phoneNo) {
  pinMode(_rts, OUTPUT);
  digitalWrite(_rts, LOW);

  hwSerial -> begin(baud);
  waitForSend();

  delay(RS485_SETTLETIME);
  hwSerial -> flush();
  conState = FTPstate = 0;
  receivedChecksum = 0;
  connected = false;
  phoneNO = phoneNo;
  blockSize = RS485_DEFAULT_BLOCKSIZE;
  outgoingTimer = 0;
  FTPoutgoingTimer = 0;
  FTPsentBytes = 0;
  FTPfileSize = 0;
  ignoreBytes = 0;
  outgoingFTP = FTPbufFull = FTPrecvDone = FTPrecvAborted = readyToRecv = false;
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

  if ( FTPstate == 3 ) {

    uint8 receivedCh;

    if ( !hwSerial -> available() ) {
      if (( gotFirstByte ) && ( millis() > receiveTimer + RS485_RECEIVE_TIMEOUT )) {
        FTPtimedOut = true; // Timed-out while waiting for data..
        receiveTimer = millis();
        receivedCh = 0;
      } 
      else
        return false;
    } 
    else
      receivedCh = hwSerial -> read();

    gotFirstByte = true;
    receiveTimer = millis() + 1;

    FTPrChecksum = FTPrChecksum + receivedCh;
    FTPrecvBuffer[FTPcounter++] = receivedCh;

    if ( FTPcounter < FTPbufferSize )
      return false;

    FTPstate = 2;

    if ( FTPtimedOut ) {
      amountTimeOuts++;
      if ( amountTimeOuts >= RS485_MAX_TIMEOUTS ) {
        while ( hwSerial -> available() )
          hwSerial -> read();
        FTPstate = 0;
        FTPrecvAborted = true;
        FTPrecvDone = false;
        outputMSG((char *)"ABORT");
        return false;
      }
    }

    if ( FTPrChecksum != FTPchecksum ) {
      outputMSG((char *)"FAIR");
      FTPstate = 2;
      return false;
    }

    FTPsentBytes += FTPbufferSize;
    FTPbufFull = true;
    FTPstate = 2;

    if ( FTPsentBytes >= FTPfileSize ) 
      FTPrecvDone = true;

    outputMSG((char *)"DONE");

    return false;
  }

  if (( FTPstate < 2 ) && ( FTPrecvBuffer != NULL )) {
    free(FTPrecvBuffer);
    FTPrecvBuffer = NULL;
  }

  if (( outgoingTimer > 0 ) || ( FTPoutgoingTimer > 0 )) {
    sendBuf();
    return false;
  }

  if ( ! hwSerial -> available() )
    return false;

  if ( ignoreBytes > 0 ) {
    while (( ignoreBytes > 0 ) && ( hwSerial -> available()))
      hwSerial -> read();
    return false;
  }        

  byte l;

  chCue[0] = chCue[1]; 
  chCue[1] = chCue[2]; 
  chCue[2] = chCue[3];
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
    } 
    else // Command was too long - ignore
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
      if ( connected )
      outputMSG((char *)"ERR");
  }

  return false;
}

void RS485::sendBuf(void) { 

  if (( outgoingTimer == 0 ) || ( outgoingTimer + RS485_COMMAND_DELAY > micros() ))
    return;

  while ( hwSerial -> available() > 0 )
    hwSerial -> read();

  waitForSend();
  digitalWrite(_rts, HIGH);
  if ( outgoingFTP )
    hwSerial -> write(FTPbuffer, FTPbufferSize);
  else {
    hwSerial -> print(outgoingBuffer);
    free(outgoingBuffer);
    outgoingBuffer = NULL;
  }
  waitForSend();
  digitalWrite(_rts, LOW);

  outgoingTimer = outgoingFTP = 0;
  if ( readyToRecv ) {
    FTPtimedOut = readyToRecv = false;
    FTPstate = 3;
  }
}

void RS485::outputMSG(char *MSG, const boolean withoutChecksum) {

  while ( outgoingTimer > 0 ) // Clear buffer out of the way..
    sendBuf();

  outgoingBuffer = (char *)malloc(strlen(MSG) + 12);

  if ( withoutChecksum)
    strcpy(outgoingBuffer, MSG);
  else
    sprintf(outgoingBuffer, "%s((%d))\r\n", MSG, rs485checksumByte(MSG));

  outgoingTimer = micros() + 1;
} 

void RS485::outputBUF(void) {
  while ( outgoingTimer > 0 ) // Clear buffer out of the way..
    sendBuf();

  outgoingFTP = true;
  outgoingTimer = micros() + 1;
}

boolean RS485::parseCommand(void) {

  if ( strncmp("ATD", receivedCom, 3 ) == 0 ) {
    if ( rs485valFrom(newValue, receivedCom, 3) ) {
      if ( newValue == phoneNO ) {
        connected = true;
        FTPstate = 0;
        FTPbuffer = NULL;
        FTPfileSize = 0;
        FTPsentBytes = 0;
        outputMSG((char *)"OK");
      } 
      else { // Master called someone else but hangup wasn't done properly - be smart..
        connected = false;
        FTPstate = 0;
        FTPbuffer = NULL;
        FTPfileSize = 0;
        FTPsentBytes = 0;
      }
      return false;
    }
  }

  if ( !connected ) { // Commands that are parsed only when disconnected..

    if (strncmp("ATBLOCK,", receivedCom, 8) == 0) {
      uint16 _bs, _chk;
      if ( rs485vals(_bs, _chk, receivedCom, 8) ) {
        if ( _bs > 8000 ) ignoreBytes = _bs - 160;
        else if ( _bs > 4000 ) ignoreBytes = _bs - 148;
        else if ( _bs > 1000 ) ignoreBytes = _bs - 128;
        else if ( _bs > 500 ) ignoreBytes = _bs - 96;
        else if ( _bs > 200 ) ignoreBytes = _bs - 64;
        else if ( _bs > 100 ) ignoreBytes = _bs - 32;
        else if ( _bs > 48 ) ignoreBytes = _bs - 16;
        else if ( _bs > 28 ) ignoreBytes = _bs - 12;
      }
    }

    return false;
  }

  if ((strcmp("ATH", receivedCom) == 0) || (strcmp("ATH0", receivedCom) == 0)) {
    connected = false; // Does not give a receipt - test with ATZ to be sure..
    FTPstate = 0;
    FTPbuffer = NULL;
    FTPfileSize = 0;
    FTPsentBytes = 0;
    return false;
  }

  if ( FTPstate == 1 ) { // Next commands apply only for sending file

    if (strcmp("ATHEADER", receivedCom) == 0) { // Must send with master header so other slaves would read it too..
      sprintf(str, "++++ATBLOCK,%d,%d", FTPbufferSize, FTPchecksum);
      outputMSG(str);
      return false;
    }

    if (strcmp("ATSEND", receivedCom) == 0) {
      outputBUF();
      return false;
    }

    if (strcmp("ATDONE", receivedCom) == 0) {
      FTPsentBytes += FTPbufferSize;
      FTPbufferSize = 0;
      FTPbuffer = NULL;

      if ( FTPsentBytes < FTPfileSize )
        outputMSG((char *)"OK");
      else {
        outputMSG((char *)"DONE");
        FTPfileSize = 0;
        FTPsentBytes = 0;
        FTPstate = 0;
        FTPrecvDone = true;
        FTPrecvAborted = false;
      }

      return false;
    }

    if (strcmp("ATABORT", receivedCom) == 0) {
      FTPbuffer = NULL;
      outputMSG((char *)"OK");
      FTPfileSize = 0;
      FTPsentBytes = 0;
      FTPstate = 0;
      FTPrecvDone = false;
      FTPrecvAborted = true;
    }

    outputMSG((char *)"UNK"); // Received unknown command for transfer mode
    return false;

  }

  if (strcmp("ATDATAOK", receivedCom) == 0) {
    if ( FTPrChecksum != FTPchecksum )
      outputMSG((char *)"FAIL");
    else if ( amountTimeOuts > RS485_MAX_TIMEOUTS )
      outputMSG((char *)"ABORT");
    else
      outputMSG((char *)"DONE");
    return false;
  }

  if ( FTPstate == 2 ) { // Next commands apply only for receiving file

    if (strncmp("ATBLOCK,", receivedCom, 8) == 0) {
      if ( rs485vals(FTPbufferSize, FTPchecksum, receivedCom, 8) ) {
        if ( FTPrecvBuffer == NULL )
          FTPrecvBuffer = (char *)malloc(FTPbufferSize + 12);
        FTPcounter = FTPrChecksum = 0; 
        FTPbufFull = false;
        readyToRecv = true; 
        gotFirstByte = FTPtimedOut = false;
        outputMSG((char *)"SEND");
      } 
      else
        outputMSG((char *)"ERR"); // Mismatch error
      return false;
    }

    if (strcmp("ATRESEND", receivedCom) == 0) {
      readyToRecv = true;
      outputMSG((char *)"SEND");
      FTPrChecksum = 0; 
      FTPcounter = 0;
      return false;
    }

    if (strcmp("ATDONE", receivedCom) == 0) {
      FTPstate = 0;
      FTPrecvAborted = false;
      FTPrecvDone = true;
      outputMSG((char *)"OK");
      return false;
    }

    if (strcmp("ATABORT", receivedCom) == 0) {
      outputMSG((char *)"OK");
      FTPrecvDone = false;
      FTPrecvAborted = true;
      FTPfileSize = 0;
      FTPsentBytes = 0;
      FTPstate = 0;
    }

    outputMSG((char *)"UNK"); // Received unknown command for transfer mode
    return false;

  }

  if (strcmp("ATZ", receivedCom) == 0) {
    outputMSG((char *)"OK");
    return false;
  }

  if ( FTPstate != 0 ) // Next commands do not apply in filetransfer mode
    return false; // And in filetransfer mode, we do not support custom commands.

  if (strcmp("ATDONE", receivedCom) == 0) {
    outputMSG((char *)"OK");
    return false;
  }

  if (strcmp("ATPHONEPOOL", receivedCom) == 0 ) {
    sprintf(str, "PHONEPOOL,%d,%d", RS485_FIRST_PHONENO, RS485_LAST_PHONENO);
    outputMSG(str);
    return false;
  }

  if (strcmp("ATBLOCKSIZEPOOL", receivedCom) == 0 ) {
    sprintf(str, "BLOCKSIZEPOOL,%d,%d", RS485_MIN_BLOCKSIZE, RS485_MAX_BLOCKSIZE);
    outputMSG(str);
    return false;
  }

  if (strcmp("ATPHONENO", receivedCom) == 0 ) {
    sprintf(str, "%d", phoneNO);
    outputMSG(str);
    return false;
  }

  if (strcmp("ATBLOCKSIZE", receivedCom) == 0 ) {
    sprintf(str, "%d", blockSize);
    outputMSG(str);
    return false;
  }

  if (strncmp("ATPHONENO=", receivedCom, 10) == 0) {

    if ( !rs485valFrom(newValue, receivedCom, 10) ) { 
      outputMSG((char *)"ERR"); // Number was not readable - error as checksum error
      return false;
    }

    if (( newValue < RS485_FIRST_PHONENO ) || ( newValue > RS485_LAST_PHONENO )) { // Outside pool
      outputMSG((char *)"NO");
      return false;
    }

    phoneNO = newValue;
    outputMSG((char *)"OK");
    return false;
  }

  if (strncmp("ATBLOCKSIZE=", receivedCom, 12) == 0) {    
    if ( !rs485valFrom(newValue, receivedCom, 12) ) {
      outputMSG((char *)"ERR");
      return false;
    }

    if (( newValue < RS485_MIN_BLOCKSIZE ) || ( newValue > RS485_MAX_BLOCKSIZE )) { // Outside pool
      outputMSG((char *)"NO");
      return false;
    }

    blockSize = newValue;
    outputMSG((char *)"OK");
    return false;      
  }

  return true; // Not a base command, application handles command.
}

char *RS485::command() {
  return receivedCom;
}


uint8 RS485::state(void) {

  switch ( FTPstate ) {
  case 0:
    return RS485_IDLE;
    break;
  case 1:
    return FTPbufferSize == 0 ? RS485_NEEDSBYTES : RS485_SENDING;
    break;
  default:
    return FTPbufFull ? RS485_HASBYTES : RS485_RECEIVING;
    break;
  }

  return RS485_IDLE;
}

void RS485::FTPsendMode(uint32 _fileSize) {
  sprintf(str, "SENDMODE,%d,%d", _fileSize, blockSize);
  FTPbuffer = NULL;
  FTPstate = 1;
  FTPfileSize = _fileSize;
  FTPbufferSize = 0;
  FTPsentBytes = 0;
  FTPrecvDone = FTPrecvAborted = false;
  outputMSG(str);
}

void RS485::FTPreceiveMode(uint32 _fileSize) {
  sprintf(str, "RECVMODE,%d,%d", _fileSize, blockSize); // Report accepted filesize back
  FTPbuffer = NULL;
  FTPstate = 2;
  FTPfileSize = _fileSize;
  amountTimeOuts = 0;
  FTPbufferSize = 0;
  FTPsentBytes = 0;
  FTPbufFull = FTPrecvDone = FTPrecvAborted = false;
  outputMSG(str);  
}

uint RS485::fileSize(void) {
  return FTPfileSize;
}

uint RS485::FTPhasBytes(void) {
  return FTPstate == 2 ? FTPbufferSize : 0;
}

void RS485::FTPfillBytes(char *data, uint16 size) {
  FTPbuffer = NULL;
  FTPbuffer = data;
  FTPbufferSize = size;

  FTPchecksum = 0;
  for (uint16 b = 0; b < size; b++ ) {
    FTPchecksum += FTPbuffer[b];
  }
}

void RS485::FTPfreeBytes(void) {
  FTPbufFull = false;
  FTPbufferSize = 0;
  FTPstate = 2;
}

uint16 RS485::FTPreceivedBytes(void) {
  return FTPsentBytes;
}

boolean RS485::FTPdone(void) {
  return FTPrecvDone;
}

boolean RS485::FTPaborted(void) {
  return FTPrecvAborted;
}