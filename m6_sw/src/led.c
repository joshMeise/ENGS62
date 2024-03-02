/* 
 * led.c --- module to implement led.h interface
 * 
 * Author: Joshua M. Meise
 * Created: 01-09-2024
 * Version: 1.0
 * 
 * Description: This module implements the finctions described in the led.h interface provided by Professor Taylor.
 * 
 */

// Header file inclusions.
#include "led.h"

// Predefined constants.
#define CHANNEL1 1

// Global variables.
static XGpio port;
static XGpio rgbPort;
static XGpioPs PSport;

/*
 * Calculates the exponent of a function.
 * Inputs: base; exponent
 * Outputs: result
 */
static u32 power(u32 base, u32 exp) {
	// Variable declarations.
	u32 i, res;

	// Initialize result.
	res = base;

	// If exponent is 0.
	if (exp == 0)
		return 1;
	else {
		// Multiply i times.
		for (i = 0; i < exp - 1; i++)
			res *= base;

		return res;
	}

}

/*
 * Initialize the led module.
 * Inputs: none.
 * Outputs: none.
 */
void led_init(void) {
	// Variable declarations.
	XGpioPs_Config *conf;

	// Initialize device AXI_GPIO_0.
	XGpio_Initialize(&port, XPAR_AXI_GPIO_0_DEVICE_ID);

	// Set tristate buffer to output on channel 1 LEDs.
	XGpio_SetDataDirection(&port, CHANNEL1, 0X0);

	// Initialize device AXI_GPIO_3 for rgb LED.
	XGpio_Initialize(&rgbPort, XPAR_AXI_GPIO_3_DEVICE_ID);
	
	// Set tristate buffer to output on rgb LED.
	XGpio_SetDataDirection(&rgbPort, CHANNEL1, 0X1);

	// Look up the configuration of the device.
	conf = XGpioPs_LookupConfig(XPAR_AXI_GPIO_0_DEVICE_ID);

	// Initialize XGpioPS instance.
	XGpioPs_CfgInitialize(&PSport, conf, conf->BaseAddr);

	// Set to output.
	XGpioPs_SetDirectionPin(&PSport, 7, 1);

	// Enable output enables.
	XGpioPs_SetOutputEnable(&PSport, XPAR_AXI_GPIO_0_DEVICE_ID, 7);

}

/*
 * Set <led> to one of {LED_ON,LED_OFF,...}
 * Inputs: led number; on or off state of led.
 * Outputs: none.
 */
void led_set(u32 led, bool tostate) {
	// Checks validity of arguments; does nothing if invalid argument.
	if (led >= 0 && (tostate == LED_ON || tostate == LED_OFF)) {
		// Check to see if turning LED on.
		if (tostate == LED_ON && led >= 0 && led <= 3)
			// Turn relevant LED on.
			XGpio_DiscreteWrite(&port, CHANNEL1, XGpio_DiscreteRead(&port, CHANNEL1) | power(2, led));
		// Check to see if turning LED off.
		else if (tostate == LED_OFF && led >= 0 && led <= 3)
			// Turn relevant LED off.
			XGpio_DiscreteWrite(&port, CHANNEL1, (XGpio_DiscreteRead(&port, CHANNEL1) & ~power(2, led)));
		// If colored LED.
		else if (led >= 5 && led != ALL) {
			// First turn off all colors.
			XGpio_DiscreteWrite(&rgbPort, CHANNEL1, 0b000);

			if (tostate == LED_ON) {
				// Then turn on relevant color.
				if (led - 5 == RED)
					XGpio_DiscreteWrite(&rgbPort, CHANNEL1, 0b100);
				else if (led - 5 == GREEN)
					XGpio_DiscreteWrite(&rgbPort, CHANNEL1, 0b010);
				else if (led - 5 == BLUE)
					XGpio_DiscreteWrite(&rgbPort, CHANNEL1, 0b001);
				else if (led - 5 == YELLOW)
					XGpio_DiscreteWrite(&rgbPort, CHANNEL1, 0b110);
			}
		}
		// If LED 4.
		else if (led == 4) {
			// If turning off.
			if (tostate == LED_OFF)
				XGpioPs_WritePin(&PSport, 7, 0);
			// If turning on.
			else if (tostate == LED_ON)
				XGpioPs_WritePin(&PSport, 7, 1);
		}
		// If turning all off.
		else if (led == ALL && tostate == LED_OFF) {
			XGpio_DiscreteWrite(&rgbPort, CHANNEL1, 0b000);
			XGpio_DiscreteWrite(&port, CHANNEL1, 0b0000);
		}
	}
}

/*
 * Get the status of <led>.
 * Inputs: led number for which we are obtaining status.
 * Outputs: status of LED - LED_ON or LED_OFF; off if <led> is invalid.
 */
bool led_get(u32 led) {
	// Check validity of arguments; number greater than 0 and less than 3 for 4 LEDs.
	if (led >= 0 && led <= 3) {
		// Read status of LED to check if on or off.
		if (XGpio_DiscreteRead(&port, CHANNEL1) & power(2, led))
			return LED_ON;
		else
			return LED_OFF;
	}
	else
		return LED_OFF;
		
}

/*
 * Toggle <led>.
 * Inputs: led to toggle.
 * Outputs: none.
 */
void led_toggle(u32 led) {
	// Check that a valid led number was entered.
	if (led >= 0) {
		// If led is on.
		if (led_get(led) == LED_ON)
			// Turn led off.
			led_set(led, LED_OFF);
		// If led is off.
		else if (led_get(led) == LED_OFF)
			// Turn led on.
			led_set(led, LED_ON);
	}

}
