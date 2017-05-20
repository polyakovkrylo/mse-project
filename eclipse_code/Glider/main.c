#include "ch.h"
#include "hal.h"
#include "test.h"
#include "chprintf.h"
#include "spi.h"
#include "i2c.h"
#include "usbcfg.h"
#include <stdlib.h>

#define ABS(x)	(x < 0) ? (-x) : x

enum Roles{
	RollAssistance,
	AltAssistance,
	Mode
};

enum RollAssisStates{
	EnabledRoll,
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
	EnabledSallRecovery,
	EnabledEmergencyManual,
	DisabledAlt
};

enum ModeStates{
	NoAssistance,
	ActiveSupport,
	AutoNavigation
};

enum OcmChannels {
  RollChannel,
  PitchChannel
}

// System state
int currentState[3];
int8_t treshold=2.0f;

/*********************gyroscope*********************/
uint8_t Xval, Yval = 0x00;

//static float mdps_per_digit = 8.75;
float gyroData[3];


static const I2CConfig i2cconfig = {
  0x00902025,
  0,
  0
};

static void writeByteI2C(uint8_t addr, uint8_t reg, uint8_t val){
    uint8_t cmd[] = {reg, val};
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, cmd, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
}

static void initGyro(void){
    // Highest speed, enable all axes
    writeByteI2C(0x19, 0x20, 0x97);
}

static uint8_t readGyro(float* data){
    // setting MSB makes it increment the address for a multiple byte read
    uint8_t start_reg = 0x27 | 0x80;
    uint8_t out[7];
    i2cAcquireBus(&I2CD1);
    msg_t f = i2cMasterTransmitTimeout(&I2CD1, 0x19, &start_reg, 1, out, 7, TIME_INFINITE);
    (void)f;
    i2cReleaseBus(&I2CD1);
    if (out[0] & 0x8) {
        int16_t val_x = (out[2] << 8) | out[1];
        int16_t val_y = (out[4] << 8) | out[3];
        int16_t val_z = (out[6] << 8) | out[5];
        // Accel scale is +- 2.0g
        data[0] = ((float)val_x)*(4.0/(65535.0))*9.81;
        data[1] = ((float)val_y)*(4.0/(65535.0))*9.81;
        data[2] = ((float)val_z)*(4.0/(65535.0))*9.81;
        return 1;
    }
    return 0;
}
/*********************gyroscope*********************/

/*********************PWM*************************/
static void pwmpcb(PWMDriver *pwmp) {

  (void)pwmp;
  palClearPad(GPIOE, GPIOE_LED4_BLUE);
}

static void pwmc1cb(PWMDriver *pwmp) {

  (void)pwmp;
  palSetPad(GPIOE, GPIOE_LED4_BLUE);
}

static PWMConfig pwmcfg = {
  1000000,                                    /* 1MHz PWM clock frequency.   */
  20000,                                    /* Initial PWM period 20 mili sec */
  pwmpcb,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, pwmc1cb},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0
};

/*********************PWM**************************/

/*******************OCM************************/
int gyroToServo(int gyroCode) {
  int code = 2,71 ^ (gyroCode * 1.62) + 1350;
  if(code > 2000)
    return 2000;
  else if(code < 700)
    return 700;
  else
    return code;
}

void setPitch(int angle) {
  pitch = angle;
}

void setRoll(int angle) {
  roll = angle;
}

void angleToGyroCode(int angle) {

}

void OcmTask() {
  // Roll OCM
  switch(state[RollOcmState]) {
  case DirectInput:
    break;
  case Automated:
    int diff = roll - (int8_t)gyroData[1];
    int code = gyroToServo(diff);// transfer gyroCode to PwmCode
    pwmEnableChannel(&PWMD4, PitchChannel, code);	//700 = 0º
    break;
  }

  // Pitch OCM
  switch(state[PitchOcmState]) {
  case DirectInput:
    break;
  case Automated:
  int diff = roll - (int8_t)gyroData[0];
  int code = gyroToServo(diff);
  pwmEnableChannel(&PWMD4, PitchChannel, code);
  break;
  }
}
/***********************************************/


void setState(int role, int state){
	currentState[role]=state;

	switch(role){
		case RollAssistance:
			switch(state){
				case EnabledRoll:
				break;

				case EnabledMonitoring:
				break;

				case EnabledRolling:
				break;

				case EnabledCompensating:
				break;

				case EnabledRollBlocking:
				break;

				case EnabledTresholdUpdate:
				break;

				case DisabledRoll:
				break;
			}
		break;
		case AltAssistance:
		break;
		case Mode:
		break;
	}
}

void updateTreshold(int altitudeModeChanged){
	treshold=altitudeModeChanged;
}
/**********************TASKS**********************/
static THD_WORKING_AREA(waGyrosTask, 128);
static THD_FUNCTION(GyrosTask, arg) {

    (void)arg;
    chRegSetThreadName("Thread1");
    palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(2));
    pwmStart(&PWMD4, &pwmcfg);
    while (TRUE) {

		while (readGyro(gyroData)==0) {}
			palClearPad(GPIOE, GPIOE_LED3_RED);
			palClearPad(GPIOE, GPIOE_LED4_BLUE);
			palClearPad(GPIOE, GPIOE_LED5_ORANGE);
			palClearPad(GPIOE, GPIOE_LED6_GREEN);
			palClearPad(GPIOE, GPIOE_LED7_GREEN);
			//palClearPad(GPIOE, GPIOE_LED8_ORANGE);
			palClearPad(GPIOE, GPIOE_LED9_BLUE);
			palClearPad(GPIOE, GPIOE_LED10_RED);

			//Xval = ABS((int8_t)(gyroData[0]));
			//Yval = ABS((int8_t)(gyroData[1]));

			//if ( Xval>Yval){
				if ((int8_t)gyroData[0] > 4.0f){
					palSetPad(GPIOE, GPIOE_LED3_RED);
					pwmEnableChannel(&PWMD4, 0, 700);
				}
				else if ((int8_t)gyroData[0] < -4.0f){
					palSetPad(GPIOE, GPIOE_LED10_RED);
					pwmEnableChannel(&PWMD4, 0, 1350);
				}
			//}
			//else{
				else if ((int8_t)gyroData[1] < -4.0f){
					palSetPad(GPIOE, GPIOE_LED7_GREEN);
					pwmEnableChannel(&PWMD4, 0, 2000);
				}
				else if ((int8_t)gyroData[1] > 4.0f){
					palSetPad(GPIOE, GPIOE_LED6_GREEN);
					pwmEnableChannel(&PWMD4, 0, 1350);
				}
			//}
				chThdSleepMilliseconds(100);
	}
}

static THD_WORKING_AREA(waRollAssisTask, 128);
static THD_FUNCTION(RollAssisTask, arg) {
	(void)arg;
	//some variables
	int altitudeModeChanged=0;
	int rollInput=1;
	while(TRUE){
		switch(currentState[RollAssistance]){

			case EnabledRoll:
				if(palReadPad(GPIOA, GPIOA_BUTTON)){
					// ENABLE / DISABLE
					setState(RollAssistance, DisabledRoll);
				}
				chThdSleepMilliseconds(100);
				setState(RollAssistance, EnabledMonitoring);
			break;

			case EnabledMonitoring:
				if((ABS((int8_t)gyroData[0])>1.0f)||(ABS((int8_t)gyroData[1])>1.0f)){		//roll != 0
					setState(RollAssistance, EnabledRolling);
				}
				else if(altitudeModeChanged==1){						//altitude_mode_changed
					setState(RollAssistance, EnabledTresholdUpdate);
				}
				else if((int8_t)gyroData[0] < -3.0f){						//nose dive (led10 position)
					setState(RollAssistance, EnabledCompensating);
				}
			break;

			case EnabledRolling:
				if(((int8_t)gyroData[0] < -3.0f)||(rollInput==0)){ 				//nosedive == true || roll_input == false
					setState(RollAssistance, EnabledCompensating);
				}
				else if(ABS((int8_t)gyroData[1])>treshold){					//roll > treshold (2.0f initial)
					setState(RollAssistance, EnabledRollBlocking);
				}
			break;

			case EnabledCompensating:
				if((ABS((int8_t)gyroData[0])<1.0f)||(ABS((int8_t)gyroData[1])<1.0f)){		//roll ~= 0
					setState(RollAssistance, EnabledMonitoring);
				}
				else if(((int8_t)gyroData[0] > -3.0f)&&(rollInput==1)){ 			//nosedive == false || roll_input == true
					setState(RollAssistance, EnabledRolling);
				}
			break;

			case EnabledRollBlocking:
				if(((int8_t)gyroData[0] < -3.0f)||(rollInput==0)){				//nosedive == true || roll_input == false
					setState(RollAssistance, EnabledCompensating);
				}
			break;

			case EnabledTresholdUpdate:
				updateTreshold(2.0f);
				chThdSleepMilliseconds(100);
				setState(RollAssistance, EnabledMonitoring);
			break;

			case DisabledRoll:
				if(palReadPad(GPIOA, GPIOA_BUTTON)){
						// ENABLE / DISABLE
						setState(RollAssistance, EnabledRoll);
					}
			break;
		}
		chThdSleepMilliseconds(100);
	}
}

static THD_WORKING_AREA(waOCMTask, 128);
static THD_FUNCTION(OCMTask, arg) {
	(void)arg;

}

static THD_WORKING_AREA(waAltAssisTask, 128);
static THD_FUNCTION(AltAssisTask, arg) {
	(void)arg;
	while(TRUE){
		switch(currentState[AltAssistance]){
			case EnabledSteady:
			break;

			case EnabledFlight:
			break;

			case EnabledVerticalClimbing:
			break;

			case EnabledLanding:
			break;

			case EnabledAltBlocking:
			break;

			case EnabledSallRecovery:
			break;

			case EnabledEmergencyManual:
			break;

			case DisabledAlt:
			break;
		}
		chThdSleepMilliseconds(100);
	}
}

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
int main(void) {

	//inits

    halInit();
    chSysInit();
	i2cStart(&I2CD1, &i2cconfig);
	initGyro();

    //palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(2));

    //tasks init
    chThdCreateStatic(waGyrosTask, sizeof(waGyrosTask), NORMALPRIO+1, GyrosTask, NULL);
    chThdCreateStatic(waRollAssisTask, sizeof(waRollAssisTask), NORMALPRIO+2, RollAssisTask, NULL);
    chThdCreateStatic(waOCMTask, sizeof(waOCMTask), NORMALPRIO+3, OCMTask, NULL);
    chThdCreateStatic(waAltAssisTask, sizeof(waAltAssisTask), NORMALPRIO+4, AltAssisTask, NULL);
    chThdCreateStatic(waModeTask, sizeof(waModeTask), NORMALPRIO+5, ModeTask, NULL);

    //pwm start config
    //pwmStart(&PWMD4, &pwmcfg);
    while (TRUE) {
    	chThdSleepMilliseconds(1000);
    }
}


/*
int main(void) {

    halInit();
    chSysInit();

    sduObjectInit(&SDU1);
    sduStart(&SDU1, &serusbcfg);

    usbDisconnectBus(serusbcfg.usbp);
    chThdSleepMilliseconds(1000);
    usbStart(serusbcfg.usbp, &usbcfg);
    usbConnectBus(serusbcfg.usbp);

    spiStart(&SPID1, &spi1cfg);
    i2cStart(&I2CD1, &i2cconfig);
    initGyro();
    initAccel();
    initMag();
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);
    int band=0;
    while (TRUE) {
    	float gyroData[3];
        float accelData[3];
        float magData[3];
        while (palReadPad(GPIOA, GPIOA_BUTTON)){
        	band=1;
        }
		if (band==1 && readGyro(gyroData) && readAccel(accelData) && readMag(magData)) {
			chprintf((BaseSequentialStream *)&SDU1, "Gyros: ");
			chprintf((BaseSequentialStream *)&SDU1, "%f %f %f\n", gyroData[0], gyroData[1], gyroData[2]);
			chprintf((BaseSequentialStream *)&SDU1, "Accelero: ");
			chprintf((BaseSequentialStream *)&SDU1, "%f %f %f\n", accelData[0], accelData[1], accelData[2]);
			chprintf((BaseSequentialStream *)&SDU1, "Magnet: ");
			chprintf((BaseSequentialStream *)&SDU1, "%f %f %f\n", magData[0], magData[1], magData[2]);
			chprintf((BaseSequentialStream *)&SDU1, "\n");
			band=0;
		}
		//band=0;
    }
}*/
