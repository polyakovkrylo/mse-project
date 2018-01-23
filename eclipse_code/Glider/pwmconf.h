/*
 * pwmconf.h
 *
 *  Created on: May 22, 2017
 *      Author: hector
 */

#ifndef PWMCONF_H_
#define PWMCONF_H_

static PWMConfig pwmcfg = {
  1000000,                                    /* 1MHz PWM clock frequency.   */
  19000,                                    /* Initial PWM period 20 mili sec */
  NULL,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0
};

#endif /* PWMCONF_H_ */
