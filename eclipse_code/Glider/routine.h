/*
 * routine.h
 *
 *  Created on: May 22, 2017
 *      Author: hector
 */

#ifndef ROUTINE_H_
#define ROUTINE_H_

#include <math.h>

static int8_t pitch =				 0;
static int8_t roll =				 0;

int sgn(int8_t x){
	if(x>0)
		return 1;
	else
		return -1;
}


int gyroToServo(float gyroCode) {
	//float code = gyroCode * 163.0f + 1350;

	float code = 1400 -sgn(gyroCode)*powf(2.72, (fabs(gyroCode)*1.62f));
	if(code > 2000)
		return 2100;
	else if(code < 800)
		return 800;
	else
		return code;
}

void setPitch(int angle) {
	pitch = angle;
}

void setRoll(int angle) {
	roll = angle;
}


#endif /* ROUTINE_H_ */
