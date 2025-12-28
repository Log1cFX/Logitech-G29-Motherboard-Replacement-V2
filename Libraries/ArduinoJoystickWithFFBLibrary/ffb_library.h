 /*
 * ffb_library.h
 *
 *  Created on: Dec 28, 2025
 *      Author: raffi
 */

#ifndef ARDUINOJOYSTICKWITHFFBLIBRARY_FFB_LIBRARY_H_
#define ARDUINOJOYSTICKWITHFFBLIBRARY_FFB_LIBRARY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"
#include "PIDReportType.h"

Wheel_Status ffb_init();
void ffb_updateAxis(int32_t updated_value);
uint8_t* FfbOnPIDBlockLoad();
uint8_t* FfbOnPIDPool();
uint8_t* FfbOnPIDStatus();
void FfbOnUsbData(uint8_t *data, uint8_t size);
void FfbOnCreateNewEffect(USB_FFBReport_CreateNewEffect_Feature_Data_t *data);
void ffb_getForces(int32_t *forces);

#ifdef __cplusplus
}
#endif

#endif /* ARDUINOJOYSTICKWITHFFBLIBRARY_FFB_LIBRARY_H_ */
