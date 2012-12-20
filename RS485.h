#ifndef RS485_H
#define RS485_H

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
  
#ifndef RS485_SETTLETIME
#define RS485_SETTLETIME 1200
#endif

#ifndef RS485_COMMANDSIZE
#define RS485_COMMANDSIZE 250
#endif
  
#ifndef RS485_DEFAULT_PHONENO
#define RS485_DEFAULT_PHONENO 99
#endif
  
#ifndef RS485_DEFAULT_BLOCKSIZE
#define RS485_DEFAULT_BLOCKSIZE 32
#endif
  
#ifndef RS485_FIRST_PHONENO
#define RS485_FIRST_PHONENO 1
#endif
  
#ifndef RS485_LAST_PHONENO
#define RS485_LAST_PHONENO 31
#endif
  
#ifndef RS485_MIN_BLOCKSIZE
#define RS485_MIN_BLOCKSIZE 8
#endif
  
#ifndef RS485_MAX_BLOCKSIZE
#define RS485_MAX_BLOCKSIZE 8192
#endif

// Delay is added to make sure master has enough time before data arrives
// set delay to zero if you want to disable it. 300-500 should be enough
// on modern computers.

#ifndef RS485_COMMAND_DELAY
#define RS485_COMMAND_DELAY 350
#endif
  
/* RS485 library 

written by Oskari Rauta
*/

byte rs485checksumByte(char *str);
uint16 rs485valFrom(char *str, byte offset);
  
class RS485 : public Print {
 private:
  uint8 _rts, serNo;
  HardwareSerial *hwSerial;
  uint8 chCue[4], conState, receivedChecksum;
  uint32 connected;
  char receivedCom[RS485_COMMANDSIZE];
  char *outgoingBuffer;
  uint32 outgoingTimer;
  
  boolean parseCommand(void);
  void sendBuf(void);

 public:
  uint8 phoneNO;
  uint16 blockSize;
  
  RS485(uint8 ser=1, uint8 rtsPin = 9);
  void begin(uint32 baud = 115200, uint8 phoneNo = RS485_DEFAULT_PHONENO);
  void end(void);

  uint32 available(void);
  uint8 read(void);
  void flush(void);
  virtual void write(unsigned char);
  using Print::write;
  void waitForSend(void);
  boolean received(void);
  void outputMSG(char *MSG, const boolean withoutChecksum = false);
  char *command();

};
#endif
  