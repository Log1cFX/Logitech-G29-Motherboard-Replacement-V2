/*
 MIT License

 Copyright (c) 2025 Log1cFX

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

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
