/* 
 * servo.c --- 
 * 
 * Author: Joshua M. Meise
 * Created: 01-28-2024
 * Version: 1.0
 * 
 * Description: 
 * 
 */

#include "servo.h"

// Global variables.
static XTmrCtr servoTimer;

/*
 * Initialize the servo, setting the duty cycle to 7.5%.
 * Inputs: none.
 * Outputs: none.
 */
void servo_init(void) {
	// Initialize the timer for the servomotor.
	if(XTmrCtr_Initialize(&servoTimer, XPAR_AXI_TIMER_0_DEVICE_ID) != XST_SUCCESS)
		printf("Servo timer not successfully initialized.\n");

	// Start PWM by starting both timers.
	XTmrCtr_Stop(&servoTimer, 0);
	XTmrCtr_Stop(&servoTimer, 1);

	// Set the value at which the timer is to be reset for a period of 20ms.
	XTmrCtr_SetResetValue(&servoTimer, 0, 1000000);

	// Set a duty cycle of 7.5% -> 1ms.
	XTmrCtr_SetResetValue(&servoTimer, 1, 75000);

	// Enable PWM on the timers.
	XTmrCtr_SetOptions(&servoTimer, 0, XTC_PWM_ENABLE_OPTION | XTC_EXT_COMPARE_OPTION | XTC_DOWN_COUNT_OPTION);
	XTmrCtr_SetOptions(&servoTimer, 1, XTC_PWM_ENABLE_OPTION | XTC_EXT_COMPARE_OPTION | XTC_DOWN_COUNT_OPTION);

	// Start PWM by starting both timers.
	XTmrCtr_Start(&servoTimer, 0);
	XTmrCtr_Start(&servoTimer, 1);
}

/*
 * Set the dutycycle of the servo.
 * Inputs: percentage of total period.
 * Outputs: none.
 */
void servo_set(double dutycycle) {
	// Set a duty cycle.
	XTmrCtr_SetResetValue(&servoTimer, 1, 1000000*dutycycle/100);
}
