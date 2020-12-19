/***************************************************************************//**
  @file     pit.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "pit.h"
#include "MK64F12.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void pitInit(pit_channel_t channel)
{
  	// Clock gating for PIT peripheral
	SIM->SCGC6 |= SIM_SCGC6_PIT(1);

	// Enable module
	PIT->MCR = PIT_MCR_MDIS(0);

	// // Configure desired sample rate
	// PIT->CHANNEL[0].LDVAL = (int)(PIT_CLOCK / (double)SAMPLE_RATE);

	// // Enable timer
	// PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TEN(1);
}

void pitStart(pit_channel_t channel)
{
  PIT->CHANNEL[channel].TCTRL = PIT_TCTRL_TEN(1); // start PIT (enable timer)
}

void pitStop(pit_channel_t channel)
{
  PIT->CHANNEL[channel].TCTRL = (PIT->CHANNEL[channel].TCTRL & ~PIT_TCTRL_TEN_MASK) // stop PIT (disable timer)
}

void pitSetInterval(pit_channel_t channel, uint32_t ticks)
{
  PIT->CHANNEL[channel].LDVAL = ticks; // load cnt value
}
/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
