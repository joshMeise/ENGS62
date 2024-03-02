/* 
 * io.c --- module implementing the io.h interface
 * 
 * Author: Joshua M. Meise
 * Created: 01-14-2024
 * Version: 1.0
 * 
 * Description: Implements the relevant interrupt functions listed in io.h
 * 
 */

// Include files.
#include "gic.h"
#include <stdio.h>

// Global variables.
static XGpio btnport;
static XGpio swport;
static void (*btn_callback_global)(u32 btn);
static void (*sw_callback_global)(u32 sw);
static u32 prevNum;

/*
 * Function to convert from port number to LED number for use in led.h; converts to a number 0 - 3.
 * Converts binary number to an integer.
 * Inputs: number to convert
 * Outputs: Converted number
 */
static u32 convertToLEDNum(u32 num) {
	// Variable declarations.
	u32 res;

	// Start result off as 0.
	res = 0;

	// Convert an LED number.
	while ((num /= 2) > 0)
		res++;

	return res;
}

/*
 * Control is passed to this function when a button is pressed.
 * Inputs: Interrupts handler function pointer.
 * Outputs: None.
 */
static void btn_handler(void *devp) {
	// Variable declarations.
	u32 num;
	XGpio *dev;

	// Coerce.
	dev = (XGpio *)devp;

	// Check if this is a button press (and not release) on any button.
	if ((num = XGpio_DiscreteRead(dev, 1)) != 0) {
		// Convert number to 0-3 that can be used in led.h.
		num = convertToLEDNum(num);

		// Call the callback function.
		btn_callback_global(num);
	}

	// Clear interrupts if here is an interrupt present.
	if (XGpio_InterruptGetStatus(dev) != 0)
		XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);

}

/*
 * Control is passed to this function when a switch is moved.
 * Inputs: Interrupts handler function pointer.
 * Outputs: None.
 */
static void sw_handler(void *devp) {
	// Variable declarations.
	u32 num;
	s32 diff;
	XGpio *dev;

	// Coerce.
	dev = (XGpio *)devp;

	// Check to see which switch was changed.
	num = XGpio_DiscreteRead(dev, 1);

	// Find which switch was changed.
	diff = num - prevNum;

	// Make positive.
	if (diff < 0)
		diff *= -1;

	// Convert number to 0-3 that can be used in led.h.
	diff = convertToLEDNum(diff);

	// Call the callback function.
	sw_callback_global(diff);

	// Clear interrupts if here is an interrupt present.
	if (XGpio_InterruptGetStatus(dev) != 0)
		XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);

	// Update to the current state of switch port.
	prevNum = num;

}

/*
 * Initialize the interrupts on the buttons.
 * Inputs: Interrupt handler function.
 * Outputs: None.
 */
void io_btn_init(void (*btn_callback)(u32 btn)) {
	// Initialize GPIO port for button.
	if (XGpio_Initialize(&btnport, XPAR_AXI_GPIO_1_DEVICE_ID) != XST_SUCCESS)
		printf("Error in initializing port.\n");

	// Disable global interrupts.
	XGpio_InterruptGlobalDisable(&btnport);

	// Disable interrupts on button.
	XGpio_InterruptDisable(&btnport, XGPIO_IR_CH1_MASK);

	// Connect interrupt handler to gic.
	if (gic_connect(XPAR_FABRIC_GPIO_1_VEC_ID, btn_handler, (void *)&btnport) != XST_SUCCESS)
		printf("Connecting interrupt not successful.\n");

	// Enable interrupts on channel.
	XGpio_InterruptEnable(&btnport, XGPIO_IR_CH1_MASK);

	// Enable interrupt to processor.
	XGpio_InterruptGlobalEnable(&btnport);

	// Save callback function.
	btn_callback_global = btn_callback;
}

/*
 * Close the button port.
 * Inputs: None.
 * Outputs: None.
 */
void io_btn_close(void) {
	// Disconnect interrupts from gic.
	gic_disconnect(XPAR_FABRIC_GPIO_1_VEC_ID);
}

/*
 * Initialize the interrupts on the switches.
 * Inputs: Interrupt handler function.
 * Outputs: None.
 */
void io_sw_init(void (*sw_callback)(u32 sw)) {
	// Initialize GPIO port for switch.
	if (XGpio_Initialize(&swport, XPAR_AXI_GPIO_2_DEVICE_ID) != XST_SUCCESS)
		printf("Error in initializing port.\n");

	// Disable global interrupts.
	XGpio_InterruptGlobalDisable(&swport);

	// Disable interrupts on button.
	XGpio_InterruptDisable(&swport, XGPIO_IR_CH1_MASK);

	// Connect interrupt handler to gic.
	if (gic_connect(XPAR_FABRIC_GPIO_2_VEC_ID, sw_handler, (void *)&swport) != XST_SUCCESS)
		printf("Connecting interrupt not successful.\n");

	// Enable interrupts on channel.
	XGpio_InterruptEnable(&swport, XGPIO_IR_CH1_MASK);

	// Enable interrupt to processor.
	XGpio_InterruptGlobalEnable(&swport);

	// Save callback function.
	sw_callback_global = sw_callback;

	// Get the initial state of the switch port.
	prevNum = XGpio_DiscreteRead(&swport, 1);

}

/*
 * Close the switch port.
 * Inputs: None.
 * Outputs: None.
 */
void io_sw_close(void) {
	// Disconnect interrupts from gic.
	gic_disconnect(XPAR_FABRIC_GPIO_2_VEC_ID);
}



