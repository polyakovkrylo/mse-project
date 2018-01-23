/*
 *
 * Glider code
 * Authors: Vladimir Poliakov
 * 			Hector Gerardo Muñoz Hernandez
 *
 *
 *
 *
 */

/****************************************INCLUDES************************************/
#include "ch.h"
#include "hal.h"
#include "test.h"
#include "spi.h"
#include "i2c.h"
#include "routine.h"
#include "gyroconf.h"
#include "pwmconf.h"
#include <stdlib.h>
#include <math.h>

/**************************************ENUMERATONS*********************************/
enum Roles{
	RollAssistance,
	AltAssistance,
	Mode,
	RollOcm,
	PitchOcm
};

enum RollAssisStates{
	EnabledMonitoring,
	EnabledRolling,
	EnabledCompensating,
	EnabledRollBlocking,
	EnabledTresholdUpdate,
	DisabledRoll
};

enum AltAssisStates{
	EnabledSteady,
	EnabledFlight,
	EnabledVerticalClimbing,
	EnabledLanding,
	EnabledAltBlocking,
	EnabledStallRecovery,
	EnabledEmergencyManual,
	DisabledAlt
};

enum ModeStates{
	NoAssistance,
	ActiveSupport,
	AutoNavigation
};

enum OcmStates{
	DirectInput,
	Automated
};

enum OcmChannels {
  RollChannel,
  PitchChannel
};

/************************************GLOBAL VARIABLES**************************************/
int currentState[5];
float gyroData[3];
static int landingAltitude =		 50;
static int8_t minLandingPitch =		-2.0f;
static int8_t rollTreshold =		 5.0f;
static int8_t stallTreshold = 		-5.0f;
static int8_t verticalAngle =		 5.0f;


/************************************SET STATE FUNCTIONS***********************************/
void setState(int role, int state){
	currentState[role]=state;

	switch(role){
		case RollAssistance:
			switch(state){

				case EnabledMonitoring:
					setState(RollOcm, DirectInput);
				break;

				case EnabledRolling:
					setState(RollOcm, DirectInput);
				break;

				case EnabledCompensating:
					setState(RollOcm, Automated);
					setRoll(0);
				break;

				case EnabledRollBlocking:
					setState(RollOcm, Automated);
					setRoll(rollTreshold*sgn(gyroData[1]));
				break;

				case EnabledTresholdUpdate:
				break;
			}
		break;

		case AltAssistance:
			switch(state){

				case EnabledSteady:
				break;

				case EnabledFlight:
					setState(PitchOcm, DirectInput);
				break;

				case EnabledVerticalClimbing:
				break;

				case EnabledLanding:
				break;

				case EnabledAltBlocking:
					setPitch(0);
					setState(PitchOcm, Automated);
				break;

				case EnabledStallRecovery:
					setState(RollAssistance, EnabledCompensating);
					setPitch(1.0f);
					setState(PitchOcm, Automated);
				break;

				case EnabledEmergencyManual:
				break;

				case DisabledAlt:
				break;
			}
		break;

		case Mode:
		break;
	}
}

void updateTreshold(int altitudeModeChanged){
	rollTreshold=altitudeModeChanged;
}

/************************************TASKS*******************************************/

/************************GYROSCOPE TASK***************************/
static THD_WORKING_AREA(waGyrosTask, 128);
static THD_FUNCTION(GyrosTask, arg) {
    (void)arg;

    while (TRUE) {
		while (readGyro(gyroData)==0) {}
		chThdSleepMilliseconds(20);
    }
}

/*********************ROLL ASSISTANCE TASK***********************/
static THD_WORKING_AREA(waRollAssisTask, 128);
static THD_FUNCTION(RollAssisTask, arg) {
	(void)arg;

	int altitudeModeChanged=0;
	int rollInput;

	while(TRUE){
		rollInput=palReadPad(GPIOA, GPIOA_BUTTON);
		switch(currentState[RollAssistance]){

			case EnabledMonitoring:
				if(fabs((int8_t)gyroData[1])>1.0f){							//roll != 0
					setState(RollAssistance, EnabledRolling);
				}
				else if(altitudeModeChanged==1){							//altitude_mode_changed
					setState(RollAssistance, EnabledTresholdUpdate);
				}
				/*else if((int8_t)gyroData[0] < -3.0f){						//nose dive (led10 position)
					setState(RollAssistance, EnabledCompensating);
				}*/
			break;

			case EnabledRolling:
				if(rollInput==0){ 											//nosedive == true || roll_input == false
					setState(RollAssistance, EnabledCompensating);
				}
				else if(fabs((int8_t)gyroData[1])>rollTreshold){			//roll > rollTreshold (2.0f initial)
					setState(RollAssistance, EnabledRollBlocking);
				}
			break;

			case EnabledCompensating:
				if((fabs((int8_t)gyroData[1])<0.5f)){							//roll ~= 0
					setState(RollAssistance, EnabledMonitoring);
				}
				else if((currentState[AltAssistance]!=EnabledStallRecovery)&&(rollInput==1)){	//nosedive == false || roll_input == true
					setState(RollAssistance, EnabledRolling);
				}
			break;

			case EnabledRollBlocking:
				if(rollInput==0){											//nosedive == true || roll_input == false
					setState(RollAssistance, EnabledCompensating);
				}
			break;

			case EnabledTresholdUpdate:
				updateTreshold(2.0f);
				chThdSleepMilliseconds(100);
				setState(RollAssistance, EnabledMonitoring);
			break;
		}
		chThdSleepMilliseconds(20);
	}
}

/*********************ALTITUDE ASSISTANCE TASK***********************/
static THD_WORKING_AREA(waAltAssisTask, 128);
static THD_FUNCTION(AltAssisTask, arg) {
	(void)arg;

	int throttle=1;
	int altitudeVal=100;
	int pitchInput;

	while(TRUE){
		pitchInput=palReadPad(GPIOA, GPIOA_BUTTON);

		switch(currentState[AltAssistance]){
			case EnabledSteady:
				if(throttle==1){
					setState(AltAssistance, EnabledFlight);
				}
			break;

			case EnabledFlight:
				if(altitudeVal < landingAltitude){						//altitude<LANDING_ALTITUDE
					setState(AltAssistance, EnabledLanding);
				}
				else if((int8_t)gyroData[0] > verticalAngle){			//pitch>Vertical_Angle
					setState(AltAssistance, EnabledVerticalClimbing);
				}
				else if(((int8_t)gyroData[0] < stallTreshold)){			//nose dive >0
					setState(AltAssistance, EnabledStallRecovery);
				}
			break;

			case EnabledVerticalClimbing:
				if((int8_t)gyroData[0] < verticalAngle){				//pitch<Vertical_Angle
					setState(AltAssistance, EnabledFlight);
				}
			break;

			case EnabledLanding:
				if((int8_t)gyroData[0] < minLandingPitch){				//pitch<MIN_LANDING_PITCH
					setState(AltAssistance, EnabledAltBlocking);
				}
				else if(altitudeVal > landingAltitude){					//altitude>LANDING_ALTITUDE
					setState(AltAssistance, EnabledFlight);
				}
			break;

			case EnabledAltBlocking:
				if((int8_t)gyroData[0] > minLandingPitch){ 				//pitch>MIN_LANDING_PITCH
					setState(AltAssistance, EnabledLanding);
				}
				else if(altitudeVal > landingAltitude){					//altitude>LANDING_ALTITUDE
					setState(AltAssistance, EnabledFlight);
				}
			break;

			case EnabledStallRecovery:
				if((int8_t)gyroData[0] > 0.2f){							//pitch>0
					setState(AltAssistance, EnabledFlight);
				}
				//Transition from here to emergency state if the timeout is reached
			break;

			case EnabledEmergencyManual:
				if((int8_t)gyroData[0] > 1.0f){							//pitch>0
					setState(AltAssistance, EnabledFlight);
				}
			break;
		}
		chThdSleepMilliseconds(20);
	}
}

/*******************************OCM TASK**************************/
static THD_WORKING_AREA(waOcmTask, 128);
static THD_FUNCTION(OcmTask, arg) {
	(void)arg;

	//PWM channels init
	palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(2));
	palSetPadMode(GPIOD, 13, PAL_MODE_ALTERNATE(2));
	pwmStart(&PWMD4, &pwmcfg);

	while (TRUE) {
		float diff;
		int code;

		switch(currentState[RollOcm]) {

			case DirectInput:
				pwmEnableChannel(&PWMD4, RollChannel, 1400);
			break;

			case Automated:
				diff = roll - gyroData[1];
				code = gyroToServo(diff);							// transfer gyroCode to PwmCode
				pwmEnableChannel(&PWMD4, RollChannel, code);
			break;
		}

		switch(currentState[PitchOcm]) {

			case DirectInput:
				pwmEnableChannel(&PWMD4, PitchChannel, 1400);
			break;

			case Automated:
				diff = pitch - (int8_t)gyroData[0];
				code = gyroToServo(diff);							// transfer gyroCode to PwmCode
				pwmEnableChannel(&PWMD4, PitchChannel, code);
		break;
		}

	chThdSleepMilliseconds(20);
	}
}

/**************************MODE TASK***************************/
static THD_WORKING_AREA(waModeTask, 128);
static THD_FUNCTION(ModeTask, arg) {
	(void)arg;
	while(TRUE){
		switch(currentState[Mode]){
			case NoAssistance:
			break;

			case ActiveSupport:
			break;

			case AutoNavigation:
			break;
		}
		chThdSleepMilliseconds(100);
	}
}


/*********************MAIN TASK***********************/
int main(void) {

	//System inits
    halInit();
    chSysInit();
	i2cStart(&I2CD1, &i2cconfig);
	initGyro();


    //initial states
    setState(RollOcm, Automated);
    setState(RollAssistance, EnabledMonitoring);
    setState(AltAssistance, EnabledFlight);
    //TODO create tasks as dynamic tasks

    //tasks init
    chThdCreateStatic(waGyrosTask, sizeof(waGyrosTask), NORMALPRIO+1, GyrosTask, NULL);
    chThdCreateStatic(waRollAssisTask, sizeof(waRollAssisTask), NORMALPRIO+2, RollAssisTask, NULL);
    chThdCreateStatic(waOcmTask, sizeof(waOcmTask), NORMALPRIO+3, OcmTask, NULL);
    chThdCreateStatic(waAltAssisTask, sizeof(waAltAssisTask), NORMALPRIO+4, AltAssisTask, NULL);

    while (TRUE) {
    	chThdSleepMilliseconds(100);
    }
}
/************************************END OF CODE**************************************/
