/*
 * util.c
 *
 *  Created on: Aug 11, 2025
 *      Author: raffi
 */
#include "util.h"
#include "common_types.h"

void debug_start_external_time_test(){
	HAL_GPIO_WritePin(test_GPIO_Port, test_Pin, 1);
}

void debug_stop_external_time_test(){
	HAL_GPIO_WritePin(test_GPIO_Port, test_Pin, 0);
}
