/*
 * sw_shifter.c
 *
 *  Created on: Aug 14, 2025
 *      Author: raffi
 */

#include "sw_shifter.h"

static Wheel_Status Shifter_INIT(Shifter_HandleTypeDef *shifter,
		Shifter_ConfigHandleTypeDef *config);
static Wheel_Status Shifter_DeINIT(Shifter_HandleTypeDef *shifter);
static Wheel_Status Shifter_getSpeed(Shifter_HandleTypeDef *shifter);

Shifter_HandleTypeDef hShifter = { Shifter_INIT, Shifter_DeINIT,
		Shifter_getSpeed };

static Wheel_Status Shifter_INIT(Shifter_HandleTypeDef *shifter,
		Shifter_ConfigHandleTypeDef *config) {
	if (config->hw_analog == 0) {
		return WHEEL_ERROR;
	}
	memcpy(&shifter->Config, config, sizeof(Shifter_ConfigHandleTypeDef));
	shifter->min.x = SHIFTER_DEFAULT_MIN_X;
	shifter->min.y = SHIFTER_DEFAULT_MIN_Y;
	shifter->max.x = SHIFTER_DEFAULT_MAX_X;
	shifter->max.y = SHIFTER_DEFAULT_MAX_Y;
	return WHEEL_OK;
}

static Wheel_Status Shifter_DeINIT(Shifter_HandleTypeDef *shifter) {
	Analog_HandleTypeDef *hw_analog = shifter->Config.hw_analog;
	if (hw_analog->DeINIT(hw_analog) == WHEEL_ERROR) {
		Error_Handler();
	}
	memset(&shifter->Config, 0, sizeof(Shifter_ConfigHandleTypeDef));
	return WHEEL_OK;
}

// this works just fine, it does
static Wheel_Status Shifter_getSpeed(Shifter_HandleTypeDef *shifter) {
	shifter->current_pos.x = shifter->Config.hw_analog->axis[SHIFTER_IDX];
	shifter->current_pos.y = shifter->Config.hw_analog->axis[SHIFTER_IDX + 1];

	Point *min = &shifter->min;
	Point *max = &shifter->max;
	Point *current_pos = &shifter->current_pos;

	if (current_pos->x < min->x)
		current_pos->x = min->x;
	if (current_pos->y > min->y)
		current_pos->y = min->y;
	if (current_pos->x > max->x)
		current_pos->x = max->x;
	if (current_pos->y < max->y)
		current_pos->y = max->y;

	uint16_t width = max->x - min->x;
	uint16_t height = min->y - max->y;

	if (width == 0)
		width = 1;
	if (height == 0)
		height = 1;

	uint16_t cell_w = width / SHIFTER_GRID_COLS;
	uint16_t cell_h = height / SHIFTER_GRID_ROWS;
	if (cell_w == 0)
		cell_w = 1;
	if (cell_h == 0)
		cell_h = 1;

	Point grid_location = { 0 };

	grid_location.x = (current_pos->x - min->x) / cell_w;
	grid_location.y = (current_pos->y - max->y) / cell_h;

	if (grid_location.x > SHIFTER_GRID_COLS - 1)
		grid_location.x = SHIFTER_GRID_COLS - 1;
	if (grid_location.y > SHIFTER_GRID_ROWS - 1)
		grid_location.y = SHIFTER_GRID_ROWS - 1;

	static const uint8_t speed_table[SHIFTER_GRID_ROWS][SHIFTER_GRID_COLS
			+ SHIFTER_MODIFIER_COL_OFFSET] = {
	/**/{ 2, 4, 6, 0, 7 },/**/
	/**/{ 0, 0, 0, 0, 0 },/**/
	/**/{ 0, 0, 0, 0, 0 },/**/
	/**/{ 1, 3, 5, 0, 0 } /**/
	};

	if (HAL_GPIO_ReadPin(shifter->Config.modifier_port, shifter->Config.modifier_pin)) {
		grid_location.x += SHIFTER_MODIFIER_COL_OFFSET;
	}

	shifter->gear = speed_table[grid_location.y][grid_location.x];

	return WHEEL_OK;
}
