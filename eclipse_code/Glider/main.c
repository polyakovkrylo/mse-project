#include "ch.h"
#include "hal.h"
#include "test.h"
#include "chprintf.h"
#include "spi.h"
#include "i2c.h"
#include "usbcfg.h"

#include <stdlib.h>


#define ABS(x)         (x < 0) ? (-x) : x
//palSetPad(GPIOE, GPIOE_LED6_GREEN);
//palClearPad(GPIOE, GPIOE_LED5_ORANGE);
//palReadPad(GPIOA, GPIOA_BUTTON);
//GPIOE_LED3_RED
//GPIOE_LED4_BLUE
//GPIOE_LED5_ORANGE
//GPIOE_LED6_GREEN
//GPIOE_LED7_GREEN
//GPIOE_LED8_ORANGE
//GPIOE_LED9_BLUE
//GPIOE_LED10_RED

/*********************gyroscope*********************/
uint8_t Xval, Yval = 0x00;

static float mdps_per_digit = 8.75;
float gyroData[3];

static const SPIConfig spi1cfg = {
  NULL,
  //HW dependent part.
  GPIOE,
  GPIOE_SPI1_CS,
  SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA,
  0
};

static const I2CConfig i2cconfig = {
  0x00902025, //voodoo magic
  0,
  0
};

static uint8_t writeByteSPI(uint8_t reg, uint8_t val){
	char txbuf[2] = {reg, val};
	char rxbuf[2];
	spiSelect(&SPID1);
	spiExchange(&SPID1, 2, txbuf, rxbuf);
	spiUnselect(&SPID1);
	return rxbuf[1];
}


static void initGyro(void){
    //see the L3GD20 Datasheet
    writeByteSPI(0x20, 0xcF);
}


static uint8_t readGyro(float* data){
    // read from L3GD20 registers and assemble data
    // 0xc0 sets read and address increment
    char txbuf[8] = {0xc0 | 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char rxbuf[8];
    spiSelect(&SPID1);
    spiExchange(&SPID1, 8, txbuf, rxbuf);
    spiUnselect(&SPID1);
    if (rxbuf[1] & 0x7) {
        int16_t val_x = (rxbuf[3] << 8) | rxbuf[2];
        int16_t val_y = (rxbuf[5] << 8) | rxbuf[4];
        int16_t val_z = (rxbuf[7] << 8) | rxbuf[6];
        data[0] = (((float)val_x) * mdps_per_digit)/1000.0;
        data[1] = (((float)val_y) * mdps_per_digit)/1000.0;
        data[2] = (((float)val_z) * mdps_per_digit)/1000.0;
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
  0//,
  //0
};

/*********************PWM**************************/

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

    (void)arg;
    chRegSetThreadName("Thread1");

    while (TRUE) {

		float gyroData[3];

		while (readGyro(gyroData)==0) {}


			palClearPad(GPIOE, GPIOE_LED3_RED);
			palClearPad(GPIOE, GPIOE_LED4_BLUE);
			palClearPad(GPIOE, GPIOE_LED5_ORANGE);
			palClearPad(GPIOE, GPIOE_LED6_GREEN);
			palClearPad(GPIOE, GPIOE_LED7_GREEN);
			palClearPad(GPIOE, GPIOE_LED8_ORANGE);
			palClearPad(GPIOE, GPIOE_LED9_BLUE);
			palClearPad(GPIOE, GPIOE_LED10_RED);


			Xval = ABS((int8_t)(gyroData[0]));
			Yval = ABS((int8_t)(gyroData[1]));

			if ( Xval>Yval){
				if ((int8_t)gyroData[0] > 5.0f){
				  palSetPad(GPIOE, GPIOE_LED10_RED);
				}

				if ((int8_t)gyroData[0] < -5.0f){
					palSetPad(GPIOE, GPIOE_LED3_RED);
				}

			}

			else{
				if ((int8_t)gyroData[1] < -5.0f){
					palSetPad(GPIOE, GPIOE_LED6_GREEN);
				}

				if ((int8_t)gyroData[1] > 5.0f){
					palSetPad(GPIOE, GPIOE_LED7_GREEN);

				}
			}
	}
}


int main(void) {

	enum {UP, DOWN};
	static int dir = UP, step = 50, width = 700; /* starts at .7ms, ends at 2.0ms */
    halInit();
    chSysInit();
    spiStart(&SPID1, &spi1cfg);
	i2cStart(&I2CD1, &i2cconfig);
	initGyro();

    palSetPadMode(GPIOD, 12, PAL_MODE_ALTERNATE(2));
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO+1, Thread1, NULL);

    pwmStart(&PWMD4, &pwmcfg);

    while (TRUE) {

    	pwmEnableChannel(&PWMD4, 0, 700);	//700 = 0º
    	chThdSleepMilliseconds(3000);
    	pwmEnableChannel(&PWMD4, 0, 1350);	//1350 = 90º
    	chThdSleepMilliseconds(3000);
    	pwmEnableChannel(&PWMD4, 0, 2000);	//2000 = 180º
    	chThdSleepMilliseconds(3000);
    	pwmEnableChannel(&PWMD4, 0, 1350);
    	chThdSleepMilliseconds(3000);
    	//pwmEnableChannel(&PWMD4, 0, PWM_DEGREES_TO_WIDTH(&PWMD4, 9000));
    	if(width == 700){
    		dir = UP;
    	}
		else if (width == 2000){
			dir = DOWN;
		}
		if (dir == UP){
			width += step;
		}
		else if (dir == DOWN){
			width -= step;
		}

		chThdSleepMilliseconds(100);

    }
}





/*#include "ch.h"
#include "hal.h"
#include "test.h"

#include "chprintf.h"
#include "spi.h"
#include "i2c.h"

#include "usbcfg.h"

//palSetPad(GPIOE, GPIOE_LED6_GREEN);
//palClearPad(GPIOE, GPIOE_LED5_ORANGE);
//palReadPad(GPIOA, GPIOA_BUTTON);
//GPIOE_LED3_RED
//GPIOE_LED4_BLUE
//GPIOE_LED5_ORANGE
//GPIOE_LED6_GREEN
//GPIOE_LED7_GREEN
//GPIOE_LED8_ORANGE
//GPIOE_LED9_BLUE
//GPIOE_LED10_RED

#define usb_lld_connect_bus(usbp)
#define usb_lld_disconnect_bus(usbp)

///Virtual serial port over USB.
SerialUSBDriver SDU1;

static float mdps_per_digit = 8.75;


static const SPIConfig spi1cfg = {
  NULL,
  //HW dependent part.
  GPIOE,
  GPIOE_SPI1_CS,
  SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA,
  0
};

static const I2CConfig i2cconfig = {
  0x00902025, //voodoo magic
  0,
  0
};

static uint8_t readByteSPI(uint8_t reg){
	char txbuf[2] = {0x80 | reg, 0xFF};
	char rxbuf[2];
	spiSelect(&SPID1);
	spiExchange(&SPID1, 2, txbuf, rxbuf);
	spiUnselect(&SPID1);
	return rxbuf[1];
}

static uint8_t writeByteSPI(uint8_t reg, uint8_t val){
	char txbuf[2] = {reg, val};
	char rxbuf[2];
	spiSelect(&SPID1);
	spiExchange(&SPID1, 2, txbuf, rxbuf);
	spiUnselect(&SPID1);
	return rxbuf[1];
}

static uint8_t readByteI2C(uint8_t addr, uint8_t reg){
    uint8_t data;
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, &reg, 1, &data, 1, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
    return data;
}

static void writeByteI2C(uint8_t addr, uint8_t reg, uint8_t val){
    uint8_t cmd[] = {reg, val};
    i2cAcquireBus(&I2CD1);
    (void)i2cMasterTransmitTimeout(&I2CD1, addr, cmd, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(&I2CD1);
}

static void initGyro(void){
    //see the L3GD20 Datasheet
    writeByteSPI(0x20, 0xcF);
}

static void initAccel(void){
    // Highest speed, enable all axes
    writeByteI2C(0x19, 0x20, 0x97);
}

static void initMag(void){
    // Highest speed
    writeByteI2C(0x1E, 0x00, 0x1C);
    writeByteI2C(0x1E, 0x02, 0x00);
}

static uint8_t readGyro(float* data){
    // read from L3GD20 registers and assemble data
    // 0xc0 sets read and address increment
    char txbuf[8] = {0xc0 | 0x27, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    char rxbuf[8];
    spiSelect(&SPID1);
    spiExchange(&SPID1, 8, txbuf, rxbuf);
    spiUnselect(&SPID1);
    if (rxbuf[1] & 0x7) {
        int16_t val_x = (rxbuf[3] << 8) | rxbuf[2];
        int16_t val_y = (rxbuf[5] << 8) | rxbuf[4];
        int16_t val_z = (rxbuf[7] << 8) | rxbuf[6];
        data[0] = (((float)val_x) * mdps_per_digit)/1000.0;
        data[1] = (((float)val_y) * mdps_per_digit)/1000.0;
        data[2] = (((float)val_z) * mdps_per_digit)/1000.0;
        return 1;
    }
    return 0;
}

static uint8_t readAccel(float* data){
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

static uint8_t readMag(float* data){
    uint8_t start_reg = 0x03;
    uint8_t out[7];
    i2cAcquireBus(&I2CD1);
    msg_t f = i2cMasterTransmitTimeout(&I2CD1, 0x1E, &start_reg, 1, out, 7, TIME_INFINITE);
    (void)f;
    i2cReleaseBus(&I2CD1);
    //out[6] doesn't seem to reflect actual new data, so just push out every time
    int16_t val_x = (out[0] << 8) | out[1];
    int16_t val_z = (out[2] << 8) | out[3];
    int16_t val_y = (out[4] << 8) | out[5];
    data[0] = ((float)val_x)*1.22;
    data[1] = ((float)val_y)*1.22;
    data[2] = ((float)val_z)*1.22;
    return 1;
}

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

    (void)arg;
    chRegSetThreadName("Thread1");
    while (true) {
      palSetPad(GPIOE, GPIOE_LED10_RED);
      chThdSleepMilliseconds(200);
      palClearPad(GPIOE, GPIOE_LED10_RED);
      chThdSleepMilliseconds(200);
    }
  }
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
