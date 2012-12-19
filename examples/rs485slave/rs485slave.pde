// Example routine for rs485 library
// Written by Oskari Rauta

#include <RS485.h>

RS485 rs485(1, 9); // Serial1 and pin9 for RTS

void setup() {
  rs485.begin(); // begin with baudrate 115200
  rs485.outputMSG((char *)"Ready");
  SerialUSB.println("Ready");
}

void loop() {
  if (rs485.received()) {
    if ( strcmp(rs485.command(), "ATTEST") == 0 ) // Add attest to command set
      rs485.outputMSG((char *)"OK"); // Answer OK to ATTEST query
    else
      rs485.outputMSG((char *)"UNK"); // Answer UNK on unknown commands
  }
}

