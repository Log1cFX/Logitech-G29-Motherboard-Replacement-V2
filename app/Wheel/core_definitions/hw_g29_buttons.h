/*
 * hw_g29_buttons.h
 *
 *  Created on: Oct 17, 2026
 *      Author: raffi
 *
 *  The button board of the G29 (hw_g29_buttons.c): an implementation of
 *  hw_digital_input.h
 */

#ifndef CORE_DEFINITIONS_HW_G29_BUTTONS_H_
#define CORE_DEFINITIONS_HW_G29_BUTTONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_digital_input.h"

extern DigitalInput_HandleTypeDef hG29Buttons;

#ifdef __cplusplus
}
#endif

#endif /* CORE_DEFINITIONS_HW_G29_BUTTONS_H_ */
