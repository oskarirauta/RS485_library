#ifndef __RS485_H__
#define __RS485_H__

#include <stdio.h>
#include <stdlib.h>
  
#include "dataTypes.h"
#include "util.h"
#include "wiringSerial.h"

// Settle time in seconds - 0 to disable
#ifndef RS485_SETTLETIME
#define RS485_SETTLETIME 1
#endif

#ifndef RS485_REPLYSIZE
#define RS485_REPLYSIZE 250
#endif
    
// Delay is added to make sure master has enough time before data arrives
// set delay to zero if you want to disable it. 300-500 should be enough
// on modern computers.

#ifndef RS485_COMMAND_DELAY
#define RS485_COMMAND_DELAY 200
#endif

#ifndef RS485_CHECKSUM_RETRYS
#define RS485_CHECKSUM_RETRYS 3
#endif
  
#ifndef RS485_READ_RETRYS
#define RS485_READ_RETRYS 3
#endif  
  
#define RS485_IDLE 1 // terminal mode
#define RS485_SENDING 2 // File send mode
#define RS485_RECEIVING 3 // File receive mode
#define RS485_NEEDSBYTES 4 // Sending, need more bytes
#define RS485_HASBYTES 5 // Receiving, buffer full

/* RS485 library ( Master version )

written by Oskari Rauta
*/
  
class RS485 {
 private:
  char devFile[250];
  int fd;
  unsigned char _replyChecksum;
  unsigned char _reply[RS485_REPLYSIZE];
  errType _err;
  uint16 _replyVal, _replyVal2;
  uint32 __replyVal, __replyVal2;
    
  boolean parseCommand(void);
  void sendBuf(void);
  void outputBUF(void);

 public:
  uint16 FTPchecksumErrors, FTPtimeoutErrors; // amount of errorenous blocks in last transmit
  uint32 FTPreceivedBytes, FTPsentBytes; // Amounts of bytes received/sent in last succesfull transmission
  
  void begin(char *deviceNode, uint32 baud = 115200);
  void outputMSG(unsigned char *MSG, const boolean withoutChecksum = false);
  boolean getReply(void);
  boolean getAnswer(unsigned char *MSG); // Same as getreply, but sends the command and handles checksum mismatches
  boolean getStatus(void);
  boolean call(uint16 number);
  boolean setPhoneNo(uint16 number);
  boolean getPhoneNo(void);
  boolean getPhonePool(void);
  boolean setBlockSize(uint16 blockSize);
  boolean getBlockSize(void);
  boolean getBlockSizePool(void);
  boolean getValue(unsigned char *CMD, char *replyHeader);
  boolean getLong(unsigned char *CMD, char *replyHeader);
  boolean getValues(unsigned char *CMD, char *replyHeader);
  boolean getLongs(unsigned char *CMD, char *replyHeader);
  void disconnect(void);
  char *reply(void);
  uint16 replyValue(char num);
  uint16 replyValue32(char num); // Values got with getValues (int32)
  
  /* - - - File transfer commands - - - */
  boolean receiveFile(unsigned char *CMD, char *filename, const boolean append = false);
  boolean sendFile(unsigned char *CMD, char *filename);
  
};
  
#endif
  
        

        
        
        
        

        
        
        
        
        
        