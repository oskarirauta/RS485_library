#include <stdio.h>
#include <stdlib.h>
  
#include "RS485.h"

RS485 rs485;

#define MAXROUNDS 1
  
int main() {
  int rounds = 0;
  rs485.begin((char *)"/dev/tty.usbserial-FTVTL64A");
  
  while ( rounds++ < MAXROUNDS ) {
    if ( rs485.call(2) ) {
      printf("Sensor #2 answered.\r\n");
      if ( rs485.getStatus() )
        printf("Status is OK.\r\n");
      else
        printf("Status query resulted: \"%s\"\r\n", rs485.reply());
      	if ( !rs485.receiveFile((unsigned char *)"ATLICENSE", (char *)"./recvFile") )
          printf("Was not able to receive license file.\r\n");
      	else
          printf("Received file: recvFile\r\n");
      
      printf("Now disconnecting..\r\n");
      rs485.disconnect();
    } else 
      printf("Sensor #2 did not answer when called..\r\n");
  }
  
  return 0;
}

/*
#define amountDevs 32

#include <stdio.h>
#include <stdlib.h>

#include "dataTypes.h"
#include "secTypes.h"
#include "util.h"
#include "wiringSerial.h"

devNode devList[amountDevs];

int fd;
byte devNo;
boolean quit = false;
unsigned char receivedData[128];
unsigned char receivedChecksum;
errType err;

unsigned int bufSize, fileSize, blockSize;

unsigned int16 blockChecksum;

// types: 0 = none, 1 = master, 2 = relay, 3 = keyboard, 4 = sensor

void initSelf() {
	devList[0].initialized = devList[0].active = true;
	devList[0].type = 1;
}

void initRelays() {
	devList[1].initialized = true;
	devList[1].active = true;
}

void initialize() {
	unsigned char b;
	for ( b = 0; b < amountDevs; b++ )
		initDev(&devList[b], b + 1);

	initSelf();

	initRelays();

	devNo = 1;
}

void outputMSGc(char *MSG) {
	if ( serialDataAvail(fd) ) {
		int ch = 0;
		while (( ch != -1 ) && ( serialDataAvail(fd)))
			ch = serialGetchar(fd);
	}

	usleep(500);

	serialPrintf(fd, "++++%s((%d))\r\n", MSG, checksumByte(MSG));
//	printf("%s(%d)\r\n", MSG, checksumByte(MSG));
//	if ( tcdrain(fd) == -1 )
//		printf("Error while draining buffer\r\n");
}

boolean getAnswer() {
	int ch;
	char l = 0;
	boolean done = false;
	boolean gettingChecksum = false;
	receivedChecksum = 0;

	err = timeout;

	while ( !done ) {
		ch = serialGetchar(fd);
		if (( ch == -1 ) || ( l > 94 )) // Timeout - or too long answer
			return false;

		if (( !gettingChecksum ) && ( ch == '(' ))
			gettingChecksum = true;

		if (( gettingChecksum ) && ( ch == ')' ))
			done = true;
		
		if ( !gettingChecksum ) {
			receivedData[l] = ch;
			l++;
			receivedData[l] = '\0';
		} else
			if (( ch > 47 ) && ( ch < 58 ))
				receivedChecksum = (( receivedChecksum * 10 ) + (ch - 48));

	}

	err = checksum;

	if ( checksumByte(receivedData) != receivedChecksum )
		return false;

	err = ok;
	
	return true;
}

boolean waitForSync() {
  boolean result = false;
  char syncCue[3];
  int ch;

  while ( 1 ) {
	ch = serialGetchar(fd);
	if ( ch == - 1 )
		return false;
	syncCue[0] = syncCue[1]; syncCue[1] = syncCue[2];
	syncCue[3] = ch;
	if ( syncCue[0] && syncCue[1] && syncCue[2] == '#' )
		return true;
  }
  return false;
}

boolean getFileInfo() {
  if ( !getAnswer() )
    return false;
  
  if ( strncmp(receivedData, "SENDMODE,", 9) != 0 )
    return false;
  
  bufSize = fileSize = 0;
  byte b = 9;
    
  while (( receivedData[b] > 47 ) && ( receivedData[b] < 58 )) {
    fileSize = ( fileSize * 10 ) + ( receivedData[b] - 48 );
    b++;
  }
  
  if ( receivedData[b] != ',' )
    return false;

  b++;

  while (( receivedData[b] > 47 ) && ( receivedData[b] < 58 )) {
  	bufSize = ( bufSize * 10 ) + ( receivedData[b] - 48 );
    b++;
  }
  
  printf("Received file size: %d and transfer buffer size: %d\r\n", fileSize, bufSize);
  
  return true;
}

boolean getBlockInfo() {
  if ( !getAnswer() )
    return false;
  
  if ( strncmp(receivedData, "ATBLOCK,", 8) != 0 )
    return false;
  
  blockSize = blockChecksum = 0;
  byte b = 8;
  
  while (( receivedData[b] > 47 ) && ( receivedData[b] < 58 )) {
    blockSize = ( blockSize * 10 ) + ( receivedData[b] - 48 );
    b++;
  }
  
  if ( receivedData[b] != ',' )
    return false;

  b++;

  while (( receivedData[b] > 47 ) && ( receivedData[b] < 58 )) {
  	blockChecksum = ( blockChecksum * 10 ) + ( receivedData[b] - 48 );
    b++;
  }
    
  return true;
}


void setupDev(devNode *dev) {
	unsigned char str[254];

	outputMSGc("ATD99");

	if ( !getAnswer() ) {
		printf("Device did not answer - ");
		if ( err == timeout )
			printf("Timeout\r\n");
		if ( err == checksum )
			printf("Checksum mismatch\r\n");

		dev->missedRounds++;

		return;
	}
	if ( strcmp(receivedData, "OK") != 0 ) {
		printf("Device did not answer the way it should.. Answer was not OK\r\n");
		dev->missedRounds++;
		return;
	}
	
	sprintf(str, "ATPHONENO=%d", dev->relayNo);
	outputMSGc(str);
	if ( !getAnswer() ) {
		printf("Error. Device did not answer while setting phone no.\r\n");
		dev->missedRounds++;
		return;
	}

	if ( strcmp(receivedData, "OK") == 0 ) {
		printf("Device accepted new phone number. Now disconnecting.\r\n");
		dev->missedRounds = 0;
		dev->initialized = true;
	}

	outputMSGc("ATH");

}

void callSensor(devNode *dev) {
	unsigned char str[254];

	sprintf(str, "ATD%d", dev->relayNo);
	outputMSGc(str);

	if ( !getAnswer() ) {
		printf("Sensor did not answer when called..\r\n");
		dev->missedRounds++; return;
	}

	outputMSGc("ATZ");
	if ( !getAnswer() ) {
		printf("Sensor did not answer to query\r\n");
		dev->missedRounds++; return;
	}

	dev->missedRounds = 0;

	if ( strcmp(receivedData, "OK") == 0 )
		printf("Sensor state OK\r\n");
	
	outputMSGc("ATTEST");
	if ( !getAnswer() ) {
		printf("Sensor did not answer to TEST query\r\n");
		dev->missedRounds++; return;
	}

	if ( strcmp(receivedData, "OK") == 0 )
		printf("Sensor TEST returns OK\r\n");
	else
		if ( strcmp(receivedData, "UNK") == 0 )
			printf("Sensor does not know command ATTEST\r\n");
      
    outputMSGc("ATLICENSE");
    if (!getFileInfo())
      printf("License file retrieval failed..\r\n");
    else {
//      char *buffer = (char *)malloc(bufSize);
      unsigned char buffer[bufSize];
      int ch, tmpi, tmp2, receivedBytes = 0;
      unsigned int16 rChecksum;
      boolean gotBlockInfo, failed, firstBlock = true;
      unsigned char chb;
      failed = true;
      while (receivedBytes < fileSize) {
        gotBlockInfo = false;
        while (!gotBlockInfo) {

	  if ( !firstBlock ) {
//		if (!waitForSync()) printf("Failed to sync..\r\n");
		outputMSGc("ATDONE");
		if (!getAnswer()) printf("No receipt for ATDONE\r\n");
		else if (strcmp(receivedData, "ATDONE") == 0) {
			printf("All data received\r\n");
		}

	  }
		firstBlock = false;


          outputMSGc("ATHEADER");
          gotBlockInfo = getBlockInfo();
          if (!gotBlockInfo) printf("Failed to retrieve block info.. Received: \"%s\"\r\n", receivedData);
          else printf("We got block info.. \"%s\"\r\n", receivedData);
        }
        failed = true;
        while ( failed ) {
        	outputMSGc("ATSEND");
        	rChecksum = tmpi = tmp2 = 0;
        	failed = false;
        	while ( tmpi < blockSize ) {
          		ch = serialGetchar(fd);
          		if ( ch != -1 ) {
            		chb = ch;
                  	rChecksum += chb;
                    buffer[tmp2] = chb;
            		tmp2++;
          		} else {
                  failed = true; printf("Timed out..\r\n"); }
          		tmpi++;
        	}
        	if ( failed ) printf("Failed to retrieve block\r\n");
          	if ( blockChecksum != rChecksum ) { 
              failed = true;
              printf("Checksum mismatch %d != %d\r\n", blockChecksum, rChecksum); 
            }
        }
          
        for ( tmpi = 0; tmpi < blockSize; tmpi++ )
          printf("%c", buffer[tmpi]);
        receivedBytes += blockSize;
      }
    }
        

}

void requestLicense() {
  
  outputMSGc("ATLICENSE");
  
}

void loop() {

	if ( devList[devNo].active ) {
		printf("Interacting with dev #%d\r\n", devNo);
		if ( devList[devNo].initialized ) {
			callSensor(&devList[devNo]);
		} else { // Device has not been initialized
			setupDev(&devList[devNo]);
		}

	}

	devNo++;
	if ( devNo >= amountDevs )
		devNo = 1;
}

int main() {
	printf("rs485 master started.\r\n");
	initialize();
	fd = serialOpen("/dev/tty.usbserial-FTVTL64A", 115200);
	serialFlush(fd);

	while ( !quit )
		loop();

	serialClose(fd);
	return 0;
}

        

        
        
*/
        