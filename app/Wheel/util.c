/*
 * util.c
 *
 *  Created on: Aug 11, 2025
 *      Author: raffi
 */
#include "util.h"
#include "common_types.h"

uint8_t instance_num;

void debug_start_external_time_test() {
  instance_num++;
  HAL_GPIO_WritePin(test_GPIO_Port, test_Pin, 1);
}

void debug_stop_external_time_test() {
  // A stop without its start: instance_num would wrap around
  if (instance_num == 0) {
    Error_Handler();
  }
  instance_num--;
  if (instance_num == 0) {
    HAL_GPIO_WritePin(test_GPIO_Port, test_Pin, 0);
  }
}
