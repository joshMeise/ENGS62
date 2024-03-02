/* 
 * ttc.c --- module that implements ttc.h interface
 * 
 * Author: Joshua M. Meise
 * Created: 01-21-2024
 * Version: 1.0
 * 
 * Description: Implementation of functions listed in ttc.h
 * 
 */

// Header file inclusions.
#include "ttc.h"
#include "gic.h"
#include "xparameters.h"

// Global variables.
static XTtcPs ttc;
static void (*ttc_callback_saved)(void);

/*
 * What happens when timer reaches end.
 * Inputs: pointer to device.
 * Outputs: none.
 */
void ttc_handler(void *devp) {
	// Variable declarations.
	XTtcPs *device;

	// Coerce into valid data type.
	device = (XTtcPs *)devp;

	// Calls callback function.
	ttc_callback_saved();

	// If there is an interrupt present, clear interrupt.
	if (XTtcPs_GetInterruptStatus(device) != 0)
		XTtcPs_ClearInterruptStatus(device, XTTCPS_IXR_INTERVAL_MASK);

}

/*
 * Initialize the ttc frequency and save the callback.
 * Inputs: frequency at which to operate, callback function.
 * Outputs: none.
 */
void ttc_init(u32 freq, void (*ttc_callback)(void)) {
	// Variable declarations.
	XTtcPs_Config *ttc_conf;
	u8 prescaler;
	XInterval interval;

	// Lookup device configuration.
	ttc_conf = XTtcPs_LookupConfig(XPAR_XTTCPS_0_DEVICE_ID);

	// Initialize the TTC.
	if (XTtcPs_CfgInitialize(&ttc, ttc_conf, ttc_conf->BaseAddress) != XST_SUCCESS)
		printf("TTC not initialised successfully.\n");

    // First disable interrupts.
	XTtcPs_DisableInterrupts(&ttc, XTTCPS_IXR_INTERVAL_MASK);

	// Connect to the gic.
	if (gic_connect(XPAR_XTTCPS_0_INTR, ttc_handler, &ttc) != XST_SUCCESS)
		printf("Error connecting to gic.\n");

	// Calculate interval length and prescaler for a given frequency.
	XTtcPs_CalcIntervalFromFreq(&ttc, freq, &interval, &prescaler);

	// Set prescaler.
	XTtcPs_SetPrescaler(&ttc, prescaler);

	// Set interval.
	XTtcPs_SetInterval(&ttc, interval);

	// Set options for TTC.
	XTtcPs_SetOptions(&ttc, XTTCPS_OPTION_INTERVAL_MODE);

	// Save the callback function.
	ttc_callback_saved = ttc_callback;

	// Enable interrupts again.
	XTtcPs_EnableInterrupts(&ttc, XTTCPS_IXR_INTERVAL_MASK);

}

/*
 * Start the ttc.
 * Inputs: none.
 * Outputs: none.
 */
void ttc_start(void) {
	// Start the timer.
	XTtcPs_Start(&ttc);
}

/*
 * Stop the ttc.
 * Inputs: none.
 * Outputs: none.
 */
void ttc_stop(void) {
	// Stop the timer.
	XTtcPs_Stop(&ttc);
}

/*
 * Close down the ttc.
 * Inputs: none.
 * Outputs: none.
 */
void ttc_close(void) {
	// Disconnect the timer from the gic.
	gic_disconnect(XPAR_XTTCPS_0_INTR);
}
