#ifndef __SECTYPES_H__
#define __SECTYPES_H__

#include "dataTypes.h"

typedef struct {
	unsigned char relayNo, type, amountPIRs;
	boolean active, initialized;
	boolean hasLDR, hasSHT, hasGAS;
	boolean hasLED, hasBUT, hasSPKR;
	boolean hasSND;
	boolean PIR[4];
	boolean BUT;
	unsigned short LDR, GAS;
	unsigned short tempHi, tempLo;
	unsigned short humidHi, humidLo;
	unsigned char ledR, ledG, ledB;
	boolean sndPlaying;
	boolean silenced;
	boolean isAuthorized;
	boolean needsPic;
	unsigned char wantsPic;
	unsigned char missedRounds;

} devNode;

void initDev(devNode *dev, unsigned char relayNo);

#endif
