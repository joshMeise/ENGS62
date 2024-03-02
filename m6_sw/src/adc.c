/*
 * adc.c -- The ADC module implementation.
 */

// Library inclusions.
#include "adc.h"

// Global variables.
static XAdcPs xadc;

/*
 * Initialize the adc module.
 * Inputs: None.
 * Outputs: None.
 */
void adc_init(void) {
	// Variable declarations.
	XAdcPs_Config *conf;

	// Lookup the configuration for device.
	conf = XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID);

	// Initialize the XADC.
	if (XAdcPs_CfgInitialize(&xadc, conf, conf->BaseAddress) != XST_SUCCESS)
		printf("Problem initializing XADC.\n");

	// Set sequencer mode to be to be safe.
	XAdcPs_SetSequencerMode(&xadc, XADCPS_SEQ_MODE_SAFE);

	// Enable relevant channels.
	if (XAdcPs_SetSeqChEnables(&xadc, XADCPS_SEQ_CH_TEMP | XADCPS_SEQ_CH_VCCINT | XADCPS_SEQ_CH_AUX14) != XST_SUCCESS)
		printf("Sequencer channel not successfully enabled.\n");

	// Set sequencer mode to be to be continuous.
	XAdcPs_SetSequencerMode(&xadc, XADCPS_SEQ_MODE_CONTINPASS);
}

/*
 * Get the internal temperature in degree's centigrade.
 * Inputs: None.
 * Outputs: Temperature.
 */
float adc_get_temp(void) {
	// Variable declarations.
	u16 adc;

	// Get the adc value.
	adc = XAdcPs_GetAdcData(&xadc, XADCPS_CH_TEMP);

	// Convert to temperature.
	return(XAdcPs_RawToTemperature(adc));
}

/*
 * Get the internal vcc voltage (should be ~1.0v).
 * Inputs: None.
 * Outputs: Voltage.
 */
float adc_get_vccint(void) {
	// Variable declarations.
	u16 adc;

	// Get the adc value.
	adc = XAdcPs_GetAdcData(&xadc, XADCPS_CH_VCCINT);

	// Convert to voltage.
	return(XAdcPs_RawToVoltage(adc));
}

/*
 * Get the potentiometer voltage (should be between 0 and 1v).
 * Inputs: None.
 * Outputs: Voltage.
 */
float adc_get_pot(void) {
	// Variable declarations.
	u16 adc;

	// Get the adc value.
	adc = XAdcPs_GetAdcData(&xadc, XADCPS_CH_AUX_MAX - 0x1U);

	// Convert to voltage.
	return(XAdcPs_RawToVoltage(adc)/3.0);
}
