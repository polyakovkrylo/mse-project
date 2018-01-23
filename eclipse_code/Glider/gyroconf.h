/*
 * gyroconf.h
 *
 *  Created on: May 22, 2017
 *      Author: hector
 */

#ifndef GYROCONF_H_
#define GYROCONF_H_

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

#endif /* GYROCONF_H_ */
