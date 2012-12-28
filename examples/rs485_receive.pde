// Example routine for rs485 library
// Written by Oskari Rauta

#include <RS485.h>
#include <RS485_util.h>

RS485 rs485(1, 9); // Serial1 and pin9 for RTS
uint32 incomingSize, receivedBytes;
uint16 fileID;
byte fileType;

void setup() {

  rs485.begin(115200, 2); // begin with baudrate 115200  
  rs485.outputMSG((char *)"Ready");
  SerialUSB.println("Ready\r\n\n");
  rs485.blockSize = 4096;
}

void loop() {

  if ( rs485.state() == RS485_HASBYTES ) {
    //    SerialUSB.write(rs485.FTPrecvBuffer, rs485.FTPhasBytes());
    receivedBytes += rs485.FTPhasBytes();
    rs485.FTPfreeBytes();
  }

  if ((rs485.received()) && (rs485.state() == RS485_IDLE))
    parseCommand();
}

void parseCommand() {

  if ( strcmp(rs485.command(), "ATTEST") == 0 ) { // Add attest to command set
    rs485.outputMSG((char *)"OK"); // Answer OK to ATTEST query
    return; // Yes, it's same as ATZ, but this is a demonstration
  }

  if (( strncmp(rs485.command(), "ATUPLOAD,", 9) == 0 ) && 
    ( rs485fileInfo(fileType, fileID, incomingSize, rs485.command(), 9) )) {
    char str[200];
    sprintf(str, "Incoming file. Type %d with ID# %d (map to filename). Size of file is %d bytes", 
        fileType, fileID, incomingSize);
    SerialUSB.println(str);
    receivedBytes = 0;
    rs485.FTPreceiveMode(incomingSize);
  }

  rs485.outputMSG((char *)"UNK"); // Answer UNK on unknown commands

}


