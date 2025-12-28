/*
 Joystick.cpp

 Copyright (c) 2015-2017, Matthew Heironimus

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <FfbEngine.h>
#include "PIDReportHandler.h"
#include "string.h"
#include "math.h"

#define JOYSTICK_REPORT_ID_INDEX 7
#define JOYSTICK_AXIS_MINIMUM -32767
#define JOYSTICK_AXIS_MAXIMUM 32767
#define JOYSTICK_SIMULATOR_MINIMUM -32767
#define JOYSTICK_SIMULATOR_MAXIMUM 32767

#define JOYSTICK_INCLUDE_X_AXIS  0x01
#define JOYSTICK_INCLUDE_Y_AXIS  0x02
#define JOYSTICK_INCLUDE_Z_AXIS  0x04
#define JOYSTICK_INCLUDE_RX_AXIS 0x08
#define JOYSTICK_INCLUDE_RY_AXIS 0x10
#define JOYSTICK_INCLUDE_RZ_AXIS 0x20

#define JOYSTICK_INCLUDE_RUDDER      0x01
#define JOYSTICK_INCLUDE_THROTTLE    0x02
#define JOYSTICK_INCLUDE_ACCELERATOR 0x04
#define JOYSTICK_INCLUDE_BRAKE       0x08
#define JOYSTICK_INCLUDE_STEERING    0x10

unsigned int timecnt = 0;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void FfbEngine::getForce(int32_t *forces) {
	forceCalculator(forces);
}

int32_t FfbEngine::getEffectForce(volatile TEffectState &effect, Gains _gains,
		EffectParams _effect_params, uint8_t axis) {
	uint8_t direction;
	uint8_t condition;
	bool useForceDirectionForConditionEffect = (effect.enableAxis
			== DIRECTION_ENABLE && effect.conditionBlocksCount == 1);

	if (effect.enableAxis == DIRECTION_ENABLE) {
		direction = effect.directionX;
		if (effect.conditionBlocksCount > 1) {
			condition = axis;
		} else {
			condition = 0; // only one Condition Parameter Block is defined
		}
	} else {
		direction = (axis == 0) ? effect.directionX : effect.directionY;
		condition = axis;
	}

	float angle = (direction * 360.0 / 255.0) * DEG_TO_RAD;
	float angle_ratio = (axis == 0 )? sinf(angle) : -1 * cosf(angle);
	int32_t force = 0;
	switch (effect.effectType) {
	case USB_EFFECT_CONSTANT: //1
		force = ConstantForceCalculator(effect) * _gains.constantGain
				* angle_ratio;
		break;
	case USB_EFFECT_RAMP: //2
		force = RampForceCalculator(effect) * _gains.rampGain * angle_ratio;
		break;
	case USB_EFFECT_SQUARE: //3
		force = SquareForceCalculator(effect) * _gains.squareGain * angle_ratio;
		break;
	case USB_EFFECT_SINE: //4
		force = SinForceCalculator(effect) * _gains.sineGain * angle_ratio;
		break;
	case USB_EFFECT_TRIANGLE: //5
		force = TriangleForceCalculator(effect) * _gains.triangleGain
				* angle_ratio;
		break;
	case USB_EFFECT_SAWTOOTHDOWN: //6
		force = SawtoothDownForceCalculator(effect) * _gains.sawtoothdownGain
				* angle_ratio;
		break;
	case USB_EFFECT_SAWTOOTHUP: //7
		force = SawtoothUpForceCalculator(effect) * _gains.sawtoothupGain
				* angle_ratio;
		break;
	case USB_EFFECT_SPRING: //8
		force = ConditionForceCalculator(effect,
				NormalizeRange(_effect_params.springPosition,
						_effect_params.springMaxPosition), condition)
				* _gains.springGain;
		if (useForceDirectionForConditionEffect) {
			force *= angle_ratio;
		}
		break;
	case USB_EFFECT_DAMPER: //9
		force = ConditionForceCalculator(effect,
				NormalizeRange(_effect_params.damperVelocity,
						_effect_params.damperMaxVelocity), condition)
				* _gains.damperGain;
		if (useForceDirectionForConditionEffect) {
			force *= angle_ratio;
		}
		break;
	case USB_EFFECT_INERTIA: //10
		if (_effect_params.inertiaAcceleration < 0
				&& _effect_params.frictionPositionChange < 0) {
			force = ConditionForceCalculator(effect,
					abs(
							NormalizeRange(_effect_params.inertiaAcceleration,
									_effect_params.inertiaMaxAcceleration)),
					condition) * _gains.inertiaGain;
		} else if (_effect_params.inertiaAcceleration < 0
				&& _effect_params.frictionPositionChange > 0) {
			force =
					-1
							* ConditionForceCalculator(effect,
									abs(
											NormalizeRange(
													_effect_params.inertiaAcceleration,
													_effect_params.inertiaMaxAcceleration)),
									condition) * _gains.inertiaGain;
		}
		if (useForceDirectionForConditionEffect) {
			force *= angle_ratio;
		}
		break;
	case USB_EFFECT_FRICTION: //11
		force = ConditionForceCalculator(effect,
				NormalizeRange(_effect_params.frictionPositionChange,
						_effect_params.frictionMaxPositionChange), condition)
				* _gains.frictionGain;
		if (useForceDirectionForConditionEffect) {
			force *= angle_ratio;
		}
		break;
	case USB_EFFECT_CUSTOM: //12
		break;
	}
	effect.elapsedTime = (uint64_t) HAL_GetTick() - effect.startTime;
	return force;
}

void FfbEngine::forceCalculator(int32_t *forces) {
	forces[0] = 0;
	forces[1] = 0;
	for (int id = 0; id < MAX_EFFECTS; id++) {
		volatile TEffectState &effect = pidReportHandler->g_EffectStates[id];
		if ((effect.state == MEFFECTSTATE_PLAYING)
				&& ((effect.elapsedTime <= effect.duration)
						|| (effect.duration == USB_DURATION_INFINITE))
				&& !pidReportHandler->devicePaused) {
			forces[0] += (int32_t) (getEffectForce(effect, m_gains[0],
					m_effect_params[0], 0));
			forces[1] += (int32_t) (getEffectForce(effect, m_gains[1],
					m_effect_params[1], 1));
		}
	}
	forces[0] = (int32_t) ((float) 1.0 * forces[0] * m_gains[0].totalGain
			/ 10000); // each effect gain * total effect gain = 10000
	forces[1] = (int32_t) ((float) 1.0 * forces[1] * m_gains[1].totalGain
			/ 10000); // each effect gain * total effect gain = 10000
	forces[0] = map(forces[0], -10000, 10000, -250, 250);
	forces[1] = map(forces[1], -10000, 10000, -250, 250);
}

int32_t FfbEngine::ConstantForceCalculator(volatile TEffectState &effect) {
	return ApplyEnvelope(effect, (int32_t) effect.magnitude);
}

int32_t FfbEngine::RampForceCalculator(volatile TEffectState &effect) {
	int32_t tempforce = (int32_t) (effect.startMagnitude
			+ effect.elapsedTime * 1.0
					* (effect.endMagnitude - effect.startMagnitude)
					/ effect.duration);
	return ApplyEnvelope(effect, tempforce);
}

int32_t FfbEngine::SquareForceCalculator(volatile TEffectState &effect) {
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t phase = effect.phase;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t period = effect.period;

	int32_t maxMagnitude = offset + magnitude;
	int32_t minMagnitude = offset - magnitude;
	uint32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	uint32_t reminder = timeTemp % period;
	int32_t tempforce;
	if (reminder > (period / 2))
		tempforce = minMagnitude;
	else
		tempforce = maxMagnitude;
	return ApplyEnvelope(effect, tempforce);
}

int32_t FfbEngine::SinForceCalculator(volatile TEffectState &effect) {
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t phase = effect.phase;
	uint16_t timeTemp = effect.elapsedTime;
	uint16_t period = effect.period;
	float angle = 0.0;
	if (period != 0)
		angle = ((timeTemp * 1.0 / period) * 2 * M_PI + (phase / 36000.0));
	float sine = sin(angle);
	int32_t tempforce = (int32_t) (sine * magnitude);
	tempforce += offset;
	return ApplyEnvelope(effect, tempforce);
}

int32_t FfbEngine::TriangleForceCalculator(volatile TEffectState &effect) {
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t phase = effect.phase;
	uint16_t period = effect.period;
	uint16_t periodF = effect.period;

	int16_t maxMagnitude = offset + magnitude;
	int16_t minMagnitude = offset - magnitude;
	int32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	int32_t reminder = timeTemp % period;
	int32_t slope = ((maxMagnitude - minMagnitude) * 2) / periodF;
	int32_t tempforce = 0;
	if (reminder > (periodF / 2))
		tempforce = slope * (periodF - reminder);
	else
		tempforce = slope * reminder;
	tempforce += minMagnitude;
	return ApplyEnvelope(effect, tempforce);
}

int32_t FfbEngine::SawtoothDownForceCalculator(volatile TEffectState &effect) {
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t phase = effect.phase;
	uint16_t period = effect.period;
	uint16_t periodF = effect.period;

	int16_t maxMagnitude = offset + magnitude;
	int16_t minMagnitude = offset - magnitude;
	int32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	int32_t reminder = timeTemp % period;
	int32_t slope = (maxMagnitude - minMagnitude) / periodF;
	int32_t tempforce = 0;
	tempforce = slope * (period - reminder);
	tempforce += minMagnitude;
	return ApplyEnvelope(effect, tempforce);
}

int32_t FfbEngine::SawtoothUpForceCalculator(volatile TEffectState &effect) {
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t phase = effect.phase;
	uint16_t period = effect.period;
	uint16_t periodF = effect.period;

	int16_t maxMagnitude = offset + magnitude;
	int16_t minMagnitude = offset - magnitude;
	int32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	int32_t reminder = timeTemp % period;
	int32_t slope = (maxMagnitude - minMagnitude) / periodF;
	int32_t tempforce = 0;
	tempforce = slope * reminder;
	tempforce += minMagnitude;
	return ApplyEnvelope(effect, tempforce);
}

int32_t FfbEngine::ConditionForceCalculator(volatile TEffectState &effect,
		float metric, uint8_t axis) {
	float deadBand;
	float cpOffset;
	float positiveCoefficient;
	float negativeCoefficient;
	float positiveSaturation;
	float negativeSaturation;

	deadBand = effect.conditions[axis].deadBand;
	cpOffset = effect.conditions[axis].cpOffset;
	negativeCoefficient = effect.conditions[axis].negativeCoefficient;
	negativeSaturation = effect.conditions[axis].negativeSaturation;
	positiveSaturation = effect.conditions[axis].positiveSaturation;
	positiveCoefficient = effect.conditions[axis].positiveCoefficient;

	float tempForce = 0;
	if (metric < (cpOffset - deadBand)) {
		tempForce = (metric - (float) 1.00 * (cpOffset - deadBand) / 10000)
				* negativeCoefficient;
		tempForce = (
				tempForce < -negativeSaturation ?
						-negativeSaturation : tempForce);
	} else if (metric > (cpOffset + deadBand)) {
		tempForce = (metric - (float) 1.00 * (cpOffset + deadBand) / 10000)
				* positiveCoefficient;
		tempForce =
				(tempForce > positiveSaturation ? positiveSaturation : tempForce);
	} else
		return 0;
	tempForce = -tempForce * effect.gain / 255;
	switch (effect.effectType) {
	case USB_EFFECT_DAMPER:
		//tempForce = damperFilter.filterIn(tempForce);
		break;
	case USB_EFFECT_INERTIA:
		//tempForce = interiaFilter.filterIn(tempForce);
		break;
	case USB_EFFECT_FRICTION:
		//tempForce = frictionFilter.filterIn(tempForce);
		break;
	default:
		break;
	}
	return (int32_t) tempForce;
}

inline float FfbEngine::NormalizeRange(int32_t x, int32_t maxValue) {
	return (float) x * 1.00 / maxValue;
}

inline int32_t FfbEngine::ApplyGain(int16_t value, uint8_t gain) {
	int32_t value_32 = value;
	return ((value_32 * gain) / 255);
}

inline int32_t FfbEngine::ApplyEnvelope(volatile TEffectState &effect,
		int32_t value) {
	int32_t magnitude = ApplyGain(effect.magnitude, effect.gain);
	int32_t attackLevel = ApplyGain(effect.attackLevel, effect.gain);
	int32_t fadeLevel = ApplyGain(effect.fadeLevel, effect.gain);
	int32_t newValue = magnitude;
	int32_t attackTime = effect.attackTime;
	int32_t fadeTime = effect.fadeTime;
	int32_t elapsedTime = effect.elapsedTime;
	int32_t duration = effect.duration;

	if (elapsedTime < attackTime) {
		newValue = (magnitude - attackLevel) * elapsedTime / attackTime;
		newValue += attackLevel;
	}

	if (elapsedTime > (duration - fadeTime)) {
		newValue = (magnitude - fadeLevel) * (duration - elapsedTime);
		if (fadeTime != 0) {
			newValue /= fadeTime;
		}
		newValue += fadeLevel;
	}
	if (magnitude != 0) {
		newValue = newValue * value / magnitude;
	}
	return newValue;
}

