// RS485 benchmark
// Written by Oskari Rauta

// Use with master example: benchmark

#include <RS485.h>
#include <RS485_util.h>

RS485 rs485(1 ,9); // RS485 in Serial1 and PIN9 is used for RTS.

uint8 fileType;
uint16 fileID;
uint32 incomingSize, receivedBytes, writtenBytes, vFileSize, timer;

char outputContent[] = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Donec odio. Quisque volutpat mattis eros. Nullam malesuada erat ut turpis. Suspendisse urna nibh, viverra non, semper suscipit, posuere a, pede. ";
char str[200];
int16 outputC = 0;
char *buffer;
boolean transferring;

void buildBuffer(const boolean initBuf = false);


void setup() {
  rs485.begin(115200, 2); // begin with baudrate 115200 and phone number 2
  rs485.outputMSG((char *)"Ready");
  SerialUSB.println("Ready to start benchmark\r\n\n");
  transferring = false;
  buffer = NULL;
}

void loop() {
    
  switch ( rs485.state() ) {
    case RS485_NEEDSBYTES:
      buildBuffer();
      writtenBytes += min(rs485.blockSize, vFileSize - writtenBytes);
      rs485.FTPfillBytes(buffer, min(rs485.blockSize, vFileSize - writtenBytes));
      break;
    case RS485_HASBYTES:
      // SerialUSB.write(rs485.FTPrecvBuffer, rs485.FTPhasBytes());
      receivedBytes += rs485.FTPhasBytes();
      rs485.FTPfreeBytes(); // It actually does not free - freeing happens elsewhere, it resets the buffer.
      break;
    case RS485_IDLE:
      if ( transferring ) {
        sprintf(str, "Transfer ended. It took %d milliseconds.", millis() - timer);
        SerialUSB.println(str);
        transferring = false;
        if ( buffer != NULL ) {
          free(buffer);
          buffer = NULL;
        }
      }
/*      if ( rs485.received())
        parseCommand();*/
      break;
    default:
      break;
  }
  
    if ((rs485.received()) && (rs485.state() == RS485_IDLE))
    parseCommand();

}

void buildBuffer(const boolean initBuf) {
  if ( initBuf ) {
    outputC = 0;
    buffer = (char *)malloc((rs485.blockSize * sizeof(char)) + ( sizeof(char) * 2));
  }
  
  for ( int16 c = 0; c < rs485.blockSize; c++ ) {
    buffer[c] = outputContent[outputC];
    
    if ( outputC++ >= strlen(outputContent) )
      outputC = 0;
  }
}

void parseCommand() {
  
  if ( strcmp(rs485.command(), "ATTEST") == 0 ) { // Add attest to command set
    rs485.outputMSG((char *)"OK"); // Answer OK to ATTEST query
    return; // Yes, it's same as ATZ, but this is a demonstration
  }
  
  if (( strncmp(rs485.command(), "ATDOWNLOAD,", 11) == 0 ) && 
    ( rs485fileInfo(fileType, fileID, vFileSize, rs485.command(), 11) )) {
    setBlockSize(fileID);
    writtenBytes = 0;
    buildBuffer(true);
    sprintf(str, "\nSending file. Size: %d bytes, block size: %d bytes", vFileSize, rs485.blockSize);
    SerialUSB.println(str);
    transferring = true;
    timer = millis();
    rs485.FTPsendMode(vFileSize);
    rs485.FTPfillBytes(buffer, min(rs485.blockSize, vFileSize));
  }

  if (( strncmp(rs485.command(), "ATUPLOAD,", 9) == 0 ) && 
    ( rs485fileInfo(fileType, fileID, incomingSize, rs485.command(), 9) )) {
    setBlockSize(fileID);
    sprintf(str, "\nReceiving file. Size: %d bytes, block size: %d bytes", incomingSize, rs485.blockSize);
    SerialUSB.println(str);
    receivedBytes = 0;
    transferring = true;
    timer = millis();
    rs485.FTPreceiveMode(incomingSize);
  }

  rs485.outputMSG((char *)"UNK"); // Answer UNK on unknown commands
 
}

void setBlockSize(uint16 _id) {
  rs485.blockSize = (_id == 0 ? 64 : (132 * _id));
}
