/*
 * ffb_library.cpp
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#include "ffb_library.h"
#include "FfbEngine.h"

FfbReportHandler ffh(HAL_GetTick);
UserInput ui;
FfbEngine ffe(ffh, ui, HAL_GetTick);
int32_t axis_value[NUM_AXES];

Wheel_Status ffb_init() {
	if (ui.GetButtons() != 0) {
		return WHEEL_ERROR;
	}
	return WHEEL_OK;
}

void ffb_updateAxis(int32_t updated_value) {
	axis_value[0] = updated_value;
	axis_value[1] = updated_value;
	ui.UpdatePosition(axis_value);
}

uint8_t* FfbOnPIDBlockLoad() {
	return ffh.FfbOnPIDBlockLoad();
}

uint8_t* FfbOnPIDPool() {
	return ffh.FfbOnPIDPool();
}

uint8_t* FfbOnPIDStatus() {
	return ffh.FfbOnPIDStatus();
}

void FfbOnUsbData(uint8_t *data, uint8_t size) {
	ffh.FfbOnUsbData(data, size);
}
void FfbOnCreateNewEffect(uint8_t *data) {
	ffh.FfbOnCreateNewEffect((USB_FFBReport_CreateNewEffect_Feature_Data_t *)data);
}

void ffb_getForces(int32_t *forces) {
	ffe.ForceCalculator(forces);
}
