/* RS485 library ( Master version )
 
 written by Oskari Rauta
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include "RS485.h"

/* Helper functions */

void RS485::begin(char *deviceNode, uint32 baud) {  
  strcpy((char *)devFile, (char *)deviceNode);
  fd = serialOpen((char *)devFile, baud);

  _replyVal = _replyVal2 = 0;
  __replyVal = __replyVal2 = 0;

  if ( RS485_SETTLETIME > 0 )
    sleep(RS485_SETTLETIME);
}

void RS485::outputMSG(unsigned char *MSG, const boolean withoutChecksum) {
  unsigned char cmdData[strlen((char *)MSG)+20];
  if ( RS485_COMMAND_DELAY > 0 )
    usleep(RS485_COMMAND_DELAY);

  if ( serialDataAvail(fd) )
    while (( serialGetchar(fd) != -1 ) && ( serialDataAvail(fd)) );

  sprintf((char *)cmdData, "++++%s((%d))\r\n", (char *)MSG, rs485checksumByte(MSG));
  serialPrintf(fd, (char *)cmdData);
}

boolean RS485::getReply(void) {

  int ch, l = 0;
  boolean getChecksum = false;
  _replyChecksum = 0;
  _err = timeout;
  boolean firstByte = false;
  uint8 timeOuts = 0;


  while ( 1 ) {
    ch = serialGetchar(fd);

    if (( ch == -1 ) && ( !firstByte ) && ( timeOuts < RS485_READ_RETRYS )) {
      timeOuts++;
      continue;
    }

    if (( ch == -1 ) || ( l >= RS485_REPLYSIZE )) // Timeout - or too long reply
      return false;

    firstByte = true;

    if (( !getChecksum ) && ( ch == '(' ))
      getChecksum = true;

    if (( getChecksum) && ( ch == ')' )) {

      if ( serialDataAvail(fd) ) // Read rest of data to clear the line
        while (( serialGetchar(fd) != -1 ) && ( serialDataAvail(fd)) );

      _err = checksum;
      if ( rs485checksumByte(_reply) != _replyChecksum )
        return false;
      _err = ok;
      return true;
    }

    if ( !getChecksum ) {
      _reply[l++] = ch; 
      _reply[l] = '\0';
    } 
    else
      if (( ch > 47 ) && ( ch < 58 ))
        _replyChecksum = (( _replyChecksum * 10 ) + ( ch - 48 ));
  }

  return false; // We should never reach this..
}

boolean RS485::getAnswer(unsigned char *MSG) {

  int ch, l = 0;
  unsigned char errC = 0;
  boolean getChecksum = false;
  _replyChecksum = 0;
  _err = checksum;
  boolean firstByte = false;
  uint8 timeOuts = 0;

  while ( _err == checksum )

    _err = timeout;
  outputMSG(MSG);

  while ( 1 ) {
    ch = serialGetchar(fd);

    if (( ch == -1 ) && ( !firstByte ) && ( timeOuts < RS485_READ_RETRYS )) {
      timeOuts++;
      continue;
    }

    if (( ch == -1 ) || ( l >= RS485_REPLYSIZE )) // Timeout - or too long reply
      return false;

    firstByte = true;

    if (( !getChecksum ) && ( ch == '(' ))
      getChecksum = true;

    if (( getChecksum) && ( ch == ')' )) {

      if ( serialDataAvail(fd) ) // Read rest of data to clear the line
        while (( serialGetchar(fd) != -1 ) && ( serialDataAvail(fd)) );

      _err = ok;
      if ( rs485checksumByte(_reply) != _replyChecksum ) {
        _err = checksum;
        errC++;
        if ( errC > (RS485_CHECKSUM_RETRYS - 1) )
          return false;
        _replyChecksum = 0;
        getChecksum = false;
        l = 0;
      } 
      else {
        _err = ok;
        return true;
      }
    }

    if ( !getChecksum ) {
      _reply[l++] = ch; 
      _reply[l] = '\0';
    } 
    else
      if (( ch > 47 ) && ( ch < 58 ))
        _replyChecksum = (( _replyChecksum * 10 ) + ( ch - 48 ));
  }

  return false; // We should never reach this..
}

boolean RS485::getStatus(void) {
  if ( !getAnswer((unsigned char *)"ATZ")) {
    _err = refused;
    return false;
  }
  if ( strcmp((char *)_reply, "OK") == 0 )
    return true;
  return false;
}

boolean RS485::call(uint16 number) {
  unsigned char callCmd[64];

  sprintf((char *)callCmd, "ATD%d", number);
  outputMSG(callCmd);

  if (!getReply()) // No checksum handling during calling
    return false; // when not connected, mismatch report is not sent by slave(s)

  if (strcmp((char *)_reply, "OK") == 0 )
    return true;

  return false; // We got something else - might be a data transmission error for OK, or something else..  
}

boolean RS485::setPhoneNo(uint16 number) {
  unsigned char setCmd[64];

  sprintf((char *)setCmd, "ATPHONENO=%d", number);

  if ( !getAnswer(setCmd) ) {
    _err = refused;
    return false;
  }
  if ( strcmp((char *)_reply, "NO") == 0 ) {
    _err = refused;
    return false;
  }
  if ( strcmp((char *)_reply, "OK") == 0 )
    return true;
  return false;
}

boolean RS485::getPhoneNo(void) {
  if ( !getAnswer((unsigned char *)"ATPHONENO")) {
    _err = refused;
    return false;
  }

  if ( !rs485valFrom(&_replyVal, _reply, 0) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::getPhonePool(void) {
  if ( !getAnswer((unsigned char *)"ATPHONEPOOL")) {
    _err = refused;
    return false;
  }

  if ( strncmp((char *)_reply, "PHONEPOOL,", 10) != 0 ) {    
    _err = refused;
    return false;
  } // Device does not support command - refused


  if ( !rs485vals(&_replyVal, &_replyVal2, _reply, 10) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::setBlockSize(uint16 number) {
  unsigned char setCmd[64];

  sprintf((char *)setCmd, "ATBLOCKSIZE=%d", number);

  if ( !getAnswer(setCmd) )
    return false;
  if ( strcmp((char *)_reply, "NO") == 0 ) {
    _err = refused;
    return false;
  }
  if ( strcmp((char *)_reply, "OK") == 0 )
    return true;
  return false;
}

boolean RS485::getBlockSize(void) {
  if ( !getAnswer((unsigned char *)"ATBLOCKSIZE")) {
    _err = refused;
    return false;
  }

  if ( !rs485valFrom(&_replyVal, _reply, 0) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::getBlockSizePool(void) {
  if ( !getAnswer((unsigned char *)"ATBLOCKSIZEPOOL")) {
    _err = refused;
    return false;
  }

  if ( strncmp((char *)_reply, "BLOCKSIZEPOOL,", 14) != 0 ) {    
    _err = refused;
    return false;
  } // Device does not support command - refused


  if ( !rs485vals(&_replyVal, &_replyVal2, _reply, 10) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::getValue(unsigned char *CMD, char *replyHeader) {
  if ( !getAnswer(CMD)) {
    _err = refused;
    return false;
  }

  if ( strlen(replyHeader) > 0 )
    if ( strncmp((char *)_reply, replyHeader, strlen(replyHeader)) != 0 ) {
      _err = refused;
      return false;
    } // Device did not support command or return the way it should had.. - refused?


  if ( !rs485valFrom(&_replyVal, _reply, strlen(replyHeader)) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::getLong(unsigned char *CMD, char *replyHeader) {
  if ( !getAnswer(CMD)) {
    _err = refused;
    return false;
  }

  if ( strlen(replyHeader) > 0 )
    if ( strncmp((char *)_reply, replyHeader, strlen(replyHeader)) != 0 ) {
      _err = refused;
      return false;
    } // Device did not support command or return the way it should had.. - refused?


  if ( !rs485longFrom(&__replyVal, _reply, strlen(replyHeader)) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::getValues(unsigned char *CMD, char *replyHeader) {
  if ( !getAnswer(CMD)) {
    _err = refused;
    return false;
  }

  if ( strlen(replyHeader) > 0 )
    if ( strncmp((char *)_reply, replyHeader, strlen(replyHeader)) != 0 ) {
      _err = refused;
      return false;
    } // Device did not support command or return the way it should had.. - refused?


  if ( !rs485vals(&_replyVal, &_replyVal2, _reply, strlen(replyHeader)) ) {
    _err = format;
    return false;
  }
  return true;
}

boolean RS485::getLongs(unsigned char *CMD, char *replyHeader) {
  if ( !getAnswer(CMD)) {
    _err = refused;
    return false;
  }

  if ( strlen(replyHeader) > 0 )
    if ( strncmp((char *)_reply, replyHeader, strlen(replyHeader)) != 0 ) {
      _err = refused;
      return false;
    } // Device did not support command or return the way it should had.. - refused?

  if ( !rs485longs(&__replyVal, &__replyVal2, _reply, strlen(replyHeader)) ) {
    _err = format;
    return false;
  }
  return true;
}

void RS485::disconnect(void) {
  outputMSG((unsigned char *)"ATH0");
}

char *RS485::reply(void) {
  return (char *)_reply;
}

uint16 RS485::replyValue(char num) {
  if ( num == 2 )
    return _replyVal2;
  return _replyVal;
}

uint16 RS485::replyValue32(char num) {
  if ( num == 2 )
    return __replyVal2;
  return __replyVal;
}

/* - - - File transfer commands - - - */

boolean RS485::receiveFile(unsigned char *CMD, char *filename, const boolean append) {
  uint32 fileSize, receivedBytes = 0;
  uint16 blockSize, buffSize, blockChecksum, recvChecksum, recvBytes, actualBytes;
  boolean failed, blockStatus, firstBlock = true;
  int ch;
  unsigned char chb;
  FILE *outputFile;

  FTPreceivedBytes = 0;

  if ( !getLongs(CMD, (char *)"SENDMODE,") )
    return false;

  fileSize = __replyVal;
  blockSize = __replyVal2;

  if ( fileSize == 0 ) { // We are not going to download a 0 sized file..
    _err = refused;
    getAnswer((unsigned char *)"ATABORT");
    return false;
  }

  FTPchecksumErrors = FTPtimeoutErrors = 0;

  while ( receivedBytes < fileSize ) {

    if ( !getValues((unsigned char *)"ATHEADER", (char *)"++++ATBLOCK,") ) {
      _err = refused; // Something went wrong..
      getAnswer((unsigned char *)"ATABORT");
      return false;
    }

    buffSize = _replyVal;
    blockChecksum = _replyVal2;
    failed = true;
    unsigned char buffer[buffSize];

    while ( failed ) {
      failed = false;
      recvChecksum = recvBytes = actualBytes = 0;
      outputMSG((unsigned char *)"ATSEND");

      while ( recvBytes < buffSize ) {
        ch = serialGetchar(fd);
        if ( ch != -1 ) {
          chb = ch;
          recvChecksum += chb;
          buffer[actualBytes++] = chb;
        } 
        else
          failed = true; // Timed-out
        recvBytes++;
      }

      if ( recvBytes != actualBytes ) { // timeout error
        failed = true;
        FTPtimeoutErrors++;
      }

      if ( recvChecksum != blockChecksum ) { // checksum error
        failed = true;
        FTPchecksumErrors++;
      }

    }

    if (!failed) { // Received buffer is OK, write it to file..

      if ( firstBlock ) { // We need to open our file
        outputFile = fopen(filename, append ? "ab" : "wb");
        firstBlock = false;
      }

      fwrite(buffer, 1, actualBytes, outputFile);
      receivedBytes += actualBytes;

    }

    blockStatus = false;

    while (!blockStatus) {
      blockStatus = getAnswer((unsigned char *)"ATDONE");

      if ( strcmp((char *)_reply, "DONE") == 0 ) { // Close file, we are finished..
        fclose(outputFile);
        FTPreceivedBytes = receivedBytes;
        return true;
      } 
      else if ( strcmp((char *)_reply, "OK") != 0 ) { // Abort; something went wrong..
        _err = refused; // Something went wrong..
        getAnswer((unsigned char *)"ATABORT");
        return false;
      }
    }

  }

  // We shouldn't end up here.. But if we did.. Close the file..
  fclose(outputFile);

  return true;
}

boolean RS485::sendFile(unsigned char *CMD, char *filename) {
  uint16 blockSize, bufSize, blockChecksum, i;
  uint32 fileSize, bytesLeft;
  FILE *inputFile;
  unsigned char *buffer;
  unsigned char str[64];
  size_t readedBytes;
  boolean wantsData, wantsResend;

  FTPsentBytes = 0;

  if ( !getLongs(CMD, (char *)"RECVMODE,") ) {
    _err = refused;
    return false;
  }

  bytesLeft = fileSize = __replyVal;
  blockSize = __replyVal2;

  if ( fileSize == 0 ) { // We are not going to upload a 0 sized file..
    _err = refused;
    getAnswer((unsigned char *)"ATABORT");
    return false;
  }

  FTPchecksumErrors = FTPtimeoutErrors = 0;

  inputFile = fopen(filename, "rb");

  while ( bytesLeft > 0 ) {
    bufSize = blockSize < bytesLeft ? blockSize : bytesLeft; /* my compiler lacked support for min */
    buffer = (unsigned char *)malloc(sizeof(unsigned char) * bufSize);

    readedBytes = fread(buffer, 1, bufSize, inputFile);
    if (( readedBytes != bufSize ) || ( buffer == NULL )) { // memory or file-reading error encountered, output error as format
      _err = format;
      fclose(inputFile);
      return(false);
    }

    blockChecksum = 0;
    for ( i = 0; i < readedBytes; i++ )
      blockChecksum += buffer[i];

    sprintf((char *)str, "ATBLOCK,%ld,%d", readedBytes, blockChecksum);

    if ( !getAnswer(str)) {
      _err = refused;
      getAnswer((unsigned char *)"ATABORT");
      fclose(inputFile);
      return false;
    }

    if ( strcmp((char *)_reply, "SEND") != 0 ) { // Something has went wrong - slave should reply with send
      _err = refused;
      getAnswer((unsigned char *)"ATABORT");
      fclose(inputFile);
      return false;
    }

    wantsData = true; 
    wantsResend = false;

    while ( wantsData ) {
      wantsData = false;

      if ( wantsResend ) { // Checksum/timeout error, request data resending..

        if ( !getAnswer((unsigned char *)"ATRESEND")) {
          _err = refused;
          getAnswer((unsigned char *)"ATABORT");
          fclose(inputFile);
          return false;
        } 
        else if ( strcmp((char *)_reply, "SEND") != 0 ) { // Something has went wrong - slave should reply with send
          _err = refused;
          getAnswer((unsigned char *)"ATABORT");
          fclose(inputFile);
          return false;
        }

        wantsResend = false;
      }

      if ( RS485_COMMAND_DELAY > 0 )
        usleep(RS485_COMMAND_DELAY);

      if ( serialDataAvail(fd) )
        while (( serialGetchar(fd) != -1 ) && ( serialDataAvail(fd)) );

      for ( i = 0; i < readedBytes; i++ )
        serialPutchar(fd, buffer[i]);

      if ( RS485_COMMAND_DELAY > 1 ) // Give some time for slave
        usleep(ceil(RS485_COMMAND_DELAY / 2));

      if ( !getReply() ) {
        _err = refused;
        getAnswer((unsigned char *)"ATABORT");
        fclose(inputFile);
        return false;
      }

      if ( strcmp((char *)_reply, "DONE") == 0 )
        wantsData = false; // Block sent properly..
      else if ( strcmp((char *)_reply, "FAIL") == 0 ) {
        wantsData = true; // Block checksum error
        wantsResend = true;
        FTPchecksumErrors++;
      } 
      else if ( strcmp((char *)_reply, "ABORT") == 0 ) { // Aborted by receiver (slave): too many timeouts?
        _err = refused;
        fclose(inputFile);
        return false;
      }
      else { // Wrong answer - Query again before failing..
        if ( !getAnswer((unsigned char *)"ATDATAOK")) {
          _err = refused;
          getAnswer((unsigned char *)"ATABORT");
          fclose(inputFile);
          return false;
        } 
        else if ( strcmp((char *)_reply, "DONE") == 0 )
          wantsData = false; // Block sent properly..
        else if ( strcmp((char *)_reply, "FAIL") == 0 ) { // Block checksum error
          wantsData = true; // Block checksum error
          wantsResend = true;
          FTPchecksumErrors++;
        } 
        else { // Aborted by receiver (slave): too many timeouts? - or reply still unreadable.
          _err = refused;
          fclose(inputFile);
          return false;
        }
      }
    }

    bytesLeft -= readedBytes;
  }

  // File has been sent
  fclose(inputFile);

  getAnswer((unsigned char *)"ATDONE");
  return true;

}

        