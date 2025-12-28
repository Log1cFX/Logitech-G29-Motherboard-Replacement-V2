/*
 * ffb_library.cpp
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#include <FfbEngine.h>
#include "ffb_library.h"
#include "PIDReportHandler.h"

static PIDReportHandler pidHandler;
static FfbEngine ffbEngine(&pidHandler);
static Gains gains[2];
static EffectParams effectparams[2];

Wheel_Status ffb_init() {
	int8_t ret = 0;
	effectparams[0].springMaxPosition = 0x7FFF;
	effectparams[1].springMaxPosition = 0x7FFF;
	ret = ffbEngine.setEffectParams(effectparams);
	ret = ffbEngine.setGains(gains);
	return (ret==0)? WHEEL_OK : WHEEL_ERROR;
}

void ffb_updateAxis(int32_t updated_value){
	effectparams[0].springPosition = updated_value;
	effectparams[1].springPosition = updated_value;
}

uint8_t* FfbOnPIDBlockLoad() {
	return pidHandler.getPIDBlockLoad();
}
uint8_t* FfbOnPIDPool() {
	return pidHandler.getPIDPool();
}
uint8_t* FfbOnPIDStatus() {
	return pidHandler.getPIDStatus();
}
void FfbOnUsbData(uint8_t *data, uint8_t size) {
	pidHandler.UppackUsbData(data, size);
}
void FfbOnCreateNewEffect(USB_FFBReport_CreateNewEffect_Feature_Data_t *data) {
	pidHandler.CreateNewEffect(data);
}

void ffb_getForces(int32_t *forces) {
	ffbEngine.getForce(forces);
}
