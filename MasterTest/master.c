#define amountDevs 32

#include <stdio.h>

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

// types: 0 = none, 1 = master, 2 = relay, 3 = keyboard, 4 = sensor

void initSelf() {
	devList[0].initialized = devList[0].active = true;
	devList[0].type = 1;
}

void initRelays() {
	devList[1].initialized = false;
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
