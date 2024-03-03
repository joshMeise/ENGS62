/* 
 * Author: Joshua M. Meise
 *
 * fsm.c :
 * This program implements a finite state machie simulating a railway station.
 *
 * 5 statess: - Traffic: cars can cross over railway; green light.
 *            - Train: train is crossing; red light and gate closed.
 *            - Maintenance: technician is allowing train to undergo maintenance; blu light flash; gate closed and then under manual control.
 *            - Pedestrian: pedestrians can cross road; all LEDS on and red light.
 *            - Transition: opens gate, waits 10 seconds, causes lights to go from red to yellow to green on transition out of maintanance/train to traffic.
 *
 * Transitions between states: - Button 0 or 1 send into pedestrian state.
 *                             - Switch 0 indicates maintenance entry/exit.
 *                             - Switch 1 indicates train arriving/departing.
 *                             - 0/1 on UART0 indcate train arriving/passing respectively.
 *                             - 2/3 on UART0 indicate maintenance entry/exit respectively.
 *
 */

// Library inclusions.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "xgpio.h"
#include "xil_types.h"
#include "platform.h"
#include "xparameters.h"
#include "led.h"
#include "io.h"
#include "servo.h"
#include "gic.h"
#include "adc.h"
#include "ttc.h"

// Predefined constants.
#define SERVO_MIN 2.5
#define SERVO_MAX 11
#define PING 1
#define UPDATE 2
#define ID 17

// Various states.
typedef enum {
	MAINTENANCE, TRAIN, PEDESTRIAN, TRAFFIC, TRANSITION;
} state_t;

// Structure definition for update message.
typedef struct {
	int type;
	int id;
	int value;
} update_t;

// Structure definition for update response.
typedef struct {
	int type;
	int id;
	int average;
	int values[30];
} update_response_t;

// Global variables.
static bool done;
static u32 ctr;
static u32 secondsOfTraffic;
static u32 seconds;
static state_t state;

/*
 * Switch state to given state.
 * Inputs: State to which to switch.
 * Outputs: None.
 */
static void switchState(state_t toState) {
	// Switch to state.
	state = toState;
	
	// Save seconds of traffic when transitioning to pedestrian mode.
	if (toState == PEDESTRIAN)
		secondsOfTraffic = seconds;
	
	// Reset the number of seconds to show how ong we have been in the state.
	seconds = 0;
	ctr = 0;
	
}

/*
 * Handles interrupts on UART0.
 * Inputs: Pointer to instance of UART0; interrupt event; data from event.
 * Outputs: None.
 */
static void handler0(void *CallBackRef, u32 Event, u32 EventData) {
	
}

/*
 * Handles interrupts by invoking built-in interrupt handler.
 * Inputs: Pointer to UART instance.
 * Outputs: None.
 */
static void interruptHandler(void *devP) {
	// Variable declarations and coercion.
	XUartPs *dev = (XUartPs *)devP;

	// Invoke interrupt handler.
	XUartPs_InterruptHandler(dev);

}

/*
 * This function initialises a given UART.
 * Inputs: Pointer to UART to be initialized.
 * Outputs: None.
 */
static void UART0_init(XUartPs *devP) {
	// Variable declarations.
	XUartPs_Config *conf;	
	
	// Look up the config of the UART (UART0 in this case).
	conf = XUartPs_LookupConfig(XPAR_PS7_UART_0_DEVICE_ID);

	// Initialize the UART.
	if (XUartPs_CfgInitialize(devP, conf, conf->BaseAddress) != XST_SUCCESS)
		printf("Error initializing UART0.\n");

	// Disable the UART.
	XUartPs_DisableUart(devP);

	// Set the baud rate.
	if (XUartPs_SetBaudRate(devP, 9600) != XST_SUCCESS)
		printf("Error setting BAUD rate.\n");

	// Set the fifo threshod.
	XUartPs_SetFifoThreshold(devP, 1);

	// Set the interrupt handler.
	XUartPs_SetInterruptMask(devP, XUARTPS_IXR_RXOVR);

	// Set the handler.
	XUartPs_SetHandler(devP, handler0, (void *)devP);

	// Connect interrupt handler to gic.
	if (gic_connect(XPAR_XUARTPS_0_INTR, interruptHandler, (void *)devP) != XST_SUCCESS)
		printf("Connecting interrupt not successful.\n");

	// Enable the UART.
	XUartPs_EnableUart(devP);

}

/*
 * This function handles interrupts from button pushes appropriately.
 * Inputs: Button that was pushed.
 * Outputs: None.
 */
void btn_callback(u32 btn) {
	// Send into pedestrian mode if currently in traffic and button pressed.
	if ((btn == 0 || btn == 1) && state == TRAFFIC)
		switchState(PEDESTRIAN);
	// End if button is button 3.
	else if (btn == 3)
		done = true;
}

/*
 * This function handles interrupts from switch movements appropriately.
 * Inputs: Switch that was toggled that was pushed.
 * Outputs: None.
 */
void swt_callback(u32 swt) {
	// If switch 0 is moved and not in train move to train state.
	if (swt == 0 && state != TRAIN && state != MAINTENANCE)
		switchState(TRAIN);
	// Move out of train mode.
	else if (swt == 0 && state == TRAIN)
		switchState(TRANSITION);
	// If switch 1 is moved and mode is not maintenance send to maintenance mode.
	else if (swt == 1 && state != MAINTENANCE)
		switchState(MAINTENANCE);
	// Move out of maintenance.
	else if (swt == 1 && state == MAINTENANCE)
		switchState(TRANSITION);
	
}

/*
 * This function turns LED 4 on and off when we receive an interrupt on the timer.
 * Inputs: none.
 * Outputs: None.
 */
void timer_callback(void) {
	// Increase the counter 10 times per second.
	ctr++;
}

int main(void) {
	// Variable declarations.
	XUartPs UART0;
	float val;
	bool on, redLight, gateClosed;
	
	// Variable initializations.
	done = false;
	ctr = 0;
	seconds = 0;
	state = TRAFFIC;
	on = false;
	
	// Initialise the platform.
	init_platform();
	
	// Initialize the gic.
	if (gic_init() != XST_SUCCESS)
		printf("Error initializing gic.\n");

	// Initialize the LED module.
	led_init();
	
	// Initialise the servomotor module.
	servo_init();
	
	// Initialize the adc module.
	adc_init();
	
	// Initialize the ttc at frequency 10Hz.
	ttc_init(10, timer_callback);
	
	// Initialize interrupts on button.
	io_btn_init(btn_callback);
	
	// Initialize interrupts on switches.
	io_sw_init(swt_callback);

	// Initialize interrupts on UART0.
	UART0_init(&UART0);
	
	// Start the timer.
	ttc_start();

	printf("[hello!]\n");

	// Loop through until botton 3 terminates the program.
	while (!done) {
		// Check which state we are in and handle appropriately.
		switch (state) {
		case PEDESTRIAN:
			// Check to see traffic has been going for 10 seconds.
			if (secondsOfTraffic > 10) {
				// Transition light to red and then back to green at various time intervals.
				if (seconds == 0)
					led_set(YELLOW, LED_ON);
				else if (seconds == 3) {
					led_set(RED, LED_ON);
					redLight = true;

					led_set(ALL, LED_ON);
				}
				else if (seconds == 13) {
					led_set(ALL, LED_OFF);

					led_set(YELLOW, LED_ON);
					redLight = false;
				}
				else if (seconds == 16) {
					led_set(GREEN, LED_ON);
					switchState(TRAFFIC);
				}
			}		
			break;
			
		case TRRAFFIC:
			// Open gate on way into traffic mode and reset seconds counter.
			if (seconds == 0) {
				servo_set(SERVO_MIN);
				printf("Opening gate.\n");
				
				led_set(GREEN, LED_ON);
				redLight = false;
			}			
			break;
			
		case TRAIN:
			// Skip over transition parts if light already red.
			if (redLight == true)
				seconds == 3;
			
			// Turn yellow light on for 3 seconds.
			if (seconds == 0)
				led_set(YELLOW, LED_ON);
			// Turn red light on, turn pedestrian light on and close gate after 3 seconds.
			else if (seconds == 3) {
				led_set(RED, LED_ON);
				redLight = true;

				servo_set(SERVO_MAX);
				printf("Closing gate.\n");
				
				led_set(ALL, LED_ON);
			}			
			break;
			
		case MAINTENANCE:
			// If light alread red skip transitions.
			if (redLight == true)
				seconds = 6;
		 
			// Turn yellow light on for 3 seconds.
			if (seconds == 0)
				led_set(YELLOW, LED_ON);
			// Turn on red light, turn pedestrian light on and close gate after 3 seconds.
			else if (seconds == 3) {
				led_set(RED, LED_ON);
				redLight = true;

				led_set(ALL, LED_ON);
			}
			// Turn on flashing blue light and let gate be controlled by controller.
			else if (seconds >= 6) {
				// Turn on/off blue LED.
				led_set(BLUE, on);
				
				// Switch state of on.
				on = !on;

				// Read potentiometer value.
				val = adc_get_pot();

				// Set servo to potentiometer value.
				servo_set(SERVO_MIN + (SERVO_MAX - SERVO_MIN)*val);
			}
			break;
			
		case TRANSITION:
			// Open gate.
			if (seconds == 0) {
				servo_set(SERVO_MIN);
				printf("Opening gate.\n");
			}
			// Wait for 10 seconds for pedestrians to cross, then change lights back to green in sequence.
			else if (seconds == 10) {
				led_set(ALL, LED_OFF);
				
				led_set(YELLOW, LED_ON);
				redLight = false;
			}
			else if (seconds == 13) {
				led_set(GREEN, LED_ON);
				
				switchState(TRAFFIC);
			}			
			break;
			
		default:
			break;
			
		}

		// Increment the number of seconds every 10 times the counter increases.
		if (ctr%10 == 0) {
			seconds++;
			
			// Increase the seconds of traffic flow if in pedestrian or traffic mode every 10 times.
			if (state == PEDESTRIAN || state == TRAFFIC)
				secondsOfTraffic++;
		}
	}
	
	printf("[done]\n");

	// Stop the timer.
	ttc_stop();

	// Close the interrupts on switches and buttons.
	io_btn_close();
	io_sw_close();

	// Close down the timer.
	ttc_close();

	// Disable interrupts on UART0;
	XUartPs_Disable(&UART0);

	// Close the gic.
	gic_close();

	// Turn all LEDs off.
	led_set(ALL, LED_OFF);

	// Clean up the platform.
	cleanup_platform();
	
	return(0);
}
