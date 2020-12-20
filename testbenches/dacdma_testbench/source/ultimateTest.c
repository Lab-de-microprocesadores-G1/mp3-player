/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/MCAL/dac_dma/dac_dma.h"
#include <math.h>
#include "MK64F12.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define SIGNAL_FREQ		441
#define PI 				3.14159265
#define SAMPLE_RATE 	44100
#define BUFFER_SIZE 	(SAMPLE_RATE / SIGNAL_FREQ * 20) // (SAMPLE_RATE / SIGNAL_FREQ)
#define FRAME_SIZE		1024
#define PIT_CHANNEL 	1
#define PEAK_VALUE		4096

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void updateCallback(uint16_t * frameToUpdate);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

static uint16_t adjustSample(double sample, uint32_t peakValue, bool bipolar);

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint16_t signal[BUFFER_SIZE];
static uint16_t signalIndex = 0;

static uint16_t buffers[2][FRAME_SIZE];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
    dacdmaInit();

    dacdmaSetFreq(SAMPLE_RATE);

    dacdmaSetBuffers(buffers[0], buffers[1], FRAME_SIZE, updateCallback);

	// Fill buffers
	for (uint32_t i = 0 ; i < BUFFER_SIZE ; i++)
	{
		double aux = 2 * PI * i * SIGNAL_FREQ / (double) SAMPLE_RATE;
		signal[i] = adjustSample(sin(aux), 1, true);
	}

    dacdmaStart();
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{

}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void updateCallback(uint16_t * frameToUpdate)
{
	for (uint16_t j = 0 ; j < FRAME_SIZE ; j++)
	{
		frameToUpdate[j] = signal[ (signalIndex + j) % BUFFER_SIZE];
	}
	signalIndex = (signalIndex + FRAME_SIZE) % BUFFER_SIZE;
}

uint16_t adjustSample(double sample, uint32_t peakValue, bool bipolar)
{
	double result;

	// Scale to DAC range
	result = (sample * PEAK_VALUE) / peakValue;

	// If bipolar, peak2peak voltage is 2 * peakValue, divide by 2
	if (bipolar)
	{
		result /= 2;
	}

	// Adjust offset to go from 0 DAC_PEAK
	return	(uint16_t)(result + PEAK_VALUE / 2 - 1);
}


/*******************************************************************************
 ******************************************************************************/
