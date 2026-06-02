/*
 * util.c
 *
 *  Created on: Aug 11, 2025
 *      Author: raffi
 */
#include "util.h"
#include "common_types.h"

void debug_start_external_time_test() {
	HAL_GPIO_WritePin(test_GPIO_Port, test_Pin, 1);
}

void debug_stop_external_time_test() {
	HAL_GPIO_WritePin(test_GPIO_Port, test_Pin, 0);
}

float remapf(float old_min, float old_max, float old_value, float new_min,
		float new_max) {
	if (old_max == old_min) {
		return new_min; // or old_value, or 0.0f — your choice
	}

	return (old_value - old_min) * (new_max - new_min) / (old_max - old_min)
			+ new_min;
}

float clamp(float x, float min, float max) {
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}
