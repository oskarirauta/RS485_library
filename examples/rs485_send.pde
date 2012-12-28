// Example routine for rs485 library
// Written by Oskari Rauta

#define TESTSIZE 12000

#include <RS485.h>

RS485 rs485(1, 9); // Serial1 and pin9 for RTS

//char licenseFile[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas egestas mattis accumsan. Phasellus purus neque, porttitor ac bibendum a, condimentum ut leo..";
char licenseFile[TESTSIZE+1];

uint32 writtenBytes;

void setup() {
  
  byte b = 0;
  for ( uint16 b2 = 0; b2 < TESTSIZE; b2++ ) {
    licenseFile[b2] = 48 + b;
    b++;
    if ( b > 9 ) b = 0;
  }
  
  licenseFile[TESTSIZE] = '\0';
  
  rs485.begin(115200, 2); // begin with baudrate 115200  
  rs485.outputMSG((char *)"Ready");
  SerialUSB.println("Ready");
  rs485.blockSize = 2048;
}

void loop() {

  if ( rs485.state() == RS485_NEEDSBYTES ) {
    writtenBytes += min(rs485.blockSize, strlen(licenseFile) - writtenBytes);
    rs485.FTPfillBytes(licenseFile + writtenBytes, min(rs485.blockSize, strlen(licenseFile) - writtenBytes));
  }

  if ((rs485.received()) && (rs485.state() == RS485_IDLE))
    parseCommand();
}

void parseCommand() {

  if ( strcmp(rs485.command(), "ATTEST") == 0 ) { // Add attest to command set
    rs485.outputMSG((char *)"OK"); // Answer OK to ATTEST query
    return; // Yes, it's same as ATZ, but this is a demonstration
  }

  if ( strcmp(rs485.command(), "ATLICENSE") == 0 ) {
    writtenBytes = 0;
    rs485.FTPsendMode(strlen(licenseFile));
    rs485.FTPfillBytes(licenseFile, min(rs485.blockSize, strlen(licenseFile)));
  }

  rs485.outputMSG((char *)"UNK"); // Answer UNK on unknown commands

}

