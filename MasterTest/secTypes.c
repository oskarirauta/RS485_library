#include "secTypes.h"

void initDev(devNode *dev, unsigned char relayNo) {
	dev->initialized = dev->active = false;
	dev->relayNo = relayNo;
	dev->type = 0;
	dev->amountPIRs = 0;
	dev->hasLDR = dev->hasSHT = dev->hasGAS = dev->hasLED = dev->hasBUT = dev->hasSPKR = dev->hasSND = false;
	dev->PIR[0] = dev->PIR[1] = dev->PIR[2] = dev->PIR[3] = false;
	dev->BUT = false;
	dev->LDR = dev->GAS = 0;
	dev->tempHi = dev->tempLo = dev->humidHi = dev->humidLo = 0;
	dev->ledR = dev->ledG = dev->ledB = 0;
	dev->sndPlaying = dev->silenced = false;
	dev->needsPic = false;
	dev->wantsPic = dev->missedRounds = 0;	
}

