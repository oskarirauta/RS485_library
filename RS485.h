/* RS485 library 

written by Oskari Rauta
*/

#ifndef __RS485_slave_H__
#define __RS485_slave_H__

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
#define RS485_COMMAND_DELAY 390
#endif

// For FTP receive timeout to occue, values above 100 are safe.
// Test it. If it fails, increase timeout value.

#ifndef RS485_RECEIVE_TIMEOUT
#define RS485_RECEIVE_TIMEOUT 120
#endif

// Amount of maximum allowed timeout counter when receiving
// a file, until transmission is ended as failured.
// If this happens a lot; you can try to increase timeout value
// below. And/or increase value above.
  
#ifndef RS485_MAX_TIMEOUTS
#define RS485_MAX_TIMEOUTS 5
#endif
  
#define RS485_IDLE 1 // terminal mode
#define RS485_SENDING 2 // File send mode
#define RS485_RECEIVING 3 // File receive mode
#define RS485_NEEDSBYTES 4 // Sending, need more bytes
#define RS485_HASBYTES 5 // Receiving, buffer full

/* Helper functions */  
  
byte rs485checksumByte(char *str);
boolean rs485valFrom(uint16 &dest, char *str, byte offset);
boolean rs485vals(uint16 &dest1, uint16 &dest2, char *str, byte offset);
boolean rs485longs(uint32 &dest1, uint32 &dest2, char *str, byte offset);

class RS485 : public Print {
 private:
  uint8 _rts, serNo;
  HardwareSerial *hwSerial;
  uint8 chCue[4], conState, FTPstate, receivedChecksum, amountTimeOuts;
  uint32 connected;
  char receivedCom[RS485_COMMANDSIZE];
  char *outgoingBuffer;
  uint32 outgoingTimer, receiveTimer;
  uint32 FTPfileSize, FTPoutgoingTimer, FTPsentBytes;
  char *FTPbuffer;
  uint16 FTPbufferSize, FTPchecksum, FTPcounter, FTPrChecksum;
  char str[48];
  uint16 ignoreBytes;
  uint16 newValue;
  boolean outgoingFTP, FTPbufFull, FTPtimedOut;
  boolean FTPrecvDone, FTPrecvAborted;
  boolean readyToRecv, gotFirstByte;
  
  boolean parseCommand(void);
  void sendBuf(void);
  void outputBUF(void);

 public:
  uint8 phoneNO;
  uint16 blockSize;
  char *FTPrecvBuffer;
  
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
  uint8 state(void); // Returns RS485_IDLE, RS485_RECEIVING or RS485_SENDING
  
  void FTPsendMode(uint32 _fileSize);
  void FTPreceiveMode(uint32 _fileSize);

  uint32 fileSize(void); // Returns size of file being in transfer
  uint32 FTPhasBytes(void); // Returns amount of bytes in receive buffer (0 = no bytes)
  
  void FTPfillBytes(char *data, uint16 size);
  void FTPfreeBytes(void); // Call after received data has been processed
  uint16 FTPreceivedBytes(void); // Amount of bytes transferred with last file transfer
  
  boolean FTPdone(void);
  boolean FTPaborted(void);

};

#endif
        