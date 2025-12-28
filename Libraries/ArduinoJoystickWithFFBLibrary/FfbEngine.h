/*
 Joystick.h

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

#ifndef JOYSTICK_h
#define JOYSTICK_h

#include "common_types.h"
#include "PIDReportType.h"
#include "PIDReportHandler.h"

#define FORCE_FEEDBACK_MAXGAIN              100
#define DEG_TO_RAD              ((float)((float)3.14159265359 / 180.0))

struct Gains {
	uint8_t totalGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t constantGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t rampGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t squareGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t sineGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t triangleGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t sawtoothdownGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t sawtoothupGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t springGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t damperGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t inertiaGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t frictionGain = FORCE_FEEDBACK_MAXGAIN;
	uint8_t customGain = FORCE_FEEDBACK_MAXGAIN;
};

struct EffectParams {
	int32_t springMaxPosition = 0;
	int32_t springPosition = 0;

	int32_t damperMaxVelocity = 0;
	int32_t damperVelocity = 0;

	int32_t inertiaMaxAcceleration = 0;
	int32_t inertiaAcceleration = 0;

	int32_t frictionMaxPositionChange = 0;
	int32_t frictionPositionChange = 0;
};

class FfbEngine {
public:
	FfbEngine(PIDReportHandler *pidReportHandler) { // @suppress("Class members should be properly initialized")
		this->pidReportHandler = pidReportHandler;
	}

	///force calculate function
	float NormalizeRange(int32_t x, int32_t maxValue);
	int32_t ApplyEnvelope(volatile TEffectState &effect, int32_t value);
	int32_t ApplyGain(int16_t value, uint8_t gain);
	int32_t ConstantForceCalculator(volatile TEffectState &effect);
	int32_t RampForceCalculator(volatile TEffectState &effect);
	int32_t SquareForceCalculator(volatile TEffectState &effect);
	int32_t SinForceCalculator(volatile TEffectState &effect);
	int32_t TriangleForceCalculator(volatile TEffectState &effect);
	int32_t SawtoothDownForceCalculator(volatile TEffectState &effect);
	int32_t SawtoothUpForceCalculator(volatile TEffectState &effect);
	int32_t ConditionForceCalculator(volatile TEffectState &effect,
			float metric, uint8_t axis);
	void forceCalculator(int32_t *forces);
	int32_t getEffectForce(volatile TEffectState &effect, Gains _gains,
			EffectParams _effect_params, uint8_t axis);
	//force feedback Interfaces
	void getForce(int32_t *forces);
	//set gain function
	int8_t setGains(Gains *_gains) {
		if (_gains != nullptr) {
			//it should be added some limition here,but im so tired,it's 2:24 A.M now!
			m_gains = _gains;
			return 0;
		}
		return -1;
	};
	//set effect params funtions
	int8_t setEffectParams(EffectParams *_effect_params) {
		if (_effect_params != nullptr) {
			m_effect_params = _effect_params;
			return 0;
		}
		return -1;
	};
private:
	PIDReportHandler *pidReportHandler;
	//force feedback gain
	Gains *m_gains;
	//force feedback effect params
	EffectParams *m_effect_params;
};

#endif // JOYSTICK_h
