/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/MCAL/pit/pit.h"
#include "drivers/MCAL/dac/dac.h"
#include "MK64F12.h"
#include <stdbool.h>
#include <math.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define FRAME_SIZE	1024
#define SAMPLE_RATE	44100

#define PIT_CHANNEL	1
#define DAC_CHANNEL	0

#define DAC_PEAK_VALUE	4096
#define PI 				3.14159265

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onTimeout(void);
static uint16_t adjustSample(double sample, uint32_t peakValue, bool bipolar);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

// typedef int  my_int_t;

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint16_t 	buffer[FRAME_SIZE];
static uint32_t 	bufferIndex = 0;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	// Config PIT peripheral
	pitInit(PIT_CHANNEL);
	pitSetInterval(PIT_CHANNEL, PIT_HZ_TO_TICKS(SAMPLE_RATE));
	pitStart(PIT_CHANNEL);

	// Config DAC peripheral
    dac_cfg_t dacConfig;
    dacConfig.swTrigger = 1; // enable software trigger
	dacInit(DAC_CHANNEL, dacConfig);

	// Fill buffer
	for (uint32_t i = 0 ; i < FRAME_SIZE ; i++)
	{
		double aux = 2 * PI * i / FRAME_SIZE;
		buffer[i] = adjustSample(sin(aux), 1, true);
	}
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
	if (PIT->CHANNEL[PIT_CHANNEL].TFLG & PIT_TFLG_TIF_MASK)
	{
		// Clear flag
		PIT->CHANNEL[PIT_CHANNEL].TFLG = PIT_TFLG_TIF_MASK;

		onTimeout();
	}
}

uint16_t adjustSample(double sample, uint32_t peakValue, bool bipolar)
{
	double result;

	// Scale to DAC range
	result = (sample * DAC_PEAK_VALUE) / peakValue;

	// If bipolar, peak2peak voltage is 2 * peakValue, divide by 2
	if (bipolar)
	{
		result /= 2;
	}

	// Adjust offset to go from 0 DAC_PEAK
	return	(uint16_t)(result + DAC_PEAK_VALUE / 2 - 1);
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void onTimeout(void)
{
	// Write buffer to output
	dacWrite(DAC_CHANNEL, buffer[bufferIndex++]);

	bufferIndex %= FRAME_SIZE;
}


/*******************************************************************************
 ******************************************************************************/




