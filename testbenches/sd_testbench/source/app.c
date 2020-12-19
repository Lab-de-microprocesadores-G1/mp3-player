/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "MK64F12.h"
#include "drivers/HAL/sd/sd.h"
#include "drivers/HAL/led/led.h"
#include "drivers/HAL/button/button.h"
#include "drivers/MCAL/sdhc/sdhc.h"
#include "board.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define BUFFER_SIZE		4096

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onCardInserted(void);
static void onCardRemoved(void);
static void onButtonPressed(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool 	runInitCardFlag;
static bool		readCardFlag;

static bool		alreadyInitCard;
static uint8_t	buffer[BUFFER_SIZE];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	/* Default initialization of the board */
	boardInit();

	/* Initialization of the Button driver */
	buttonInit();
	buttonSubscribe(BUTTON_2, BUTTON_PRESS, onButtonPressed);

	/* Initialization of the LED driver */
	ledInit();

	/* SD Initialization Sequence */
	sdInit();
	sdOnCardInserted(onCardInserted);
	sdOnCardRemoved(onCardRemoved);

	/* Status led initialization */
	if (sdIsCardInserted())
	{
		ledSet(LED_GREEN);

		runInitCardFlag = true;
	}
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
	if (runInitCardFlag)
	{
		runInitCardFlag = false;

		if (!alreadyInitCard)
		{
			if (sdCardInit())
			{
				alreadyInitCard = true;
				ledSet(LED_BLUE);
			}
			else
			{
				ledSet(LED_RED);
			}
		}
	}

	if (readCardFlag)
	{
		readCardFlag = false;
		if (sdRead((uint32_t*)buffer, 225343 * 512, 8))
		{
			ledClear(LED_BLUE);
		}
	}
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************/

static void onCardInserted(void)
{
	ledSet(LED_GREEN);
}

static void onCardRemoved(void)
{
	ledClear(LED_GREEN);
	ledClear(LED_BLUE);
	ledClear(LED_RED);
}

static void onButtonPressed(void)
{
	if (alreadyInitCard)
	{
		readCardFlag = true;
	}
	else
	{
		runInitCardFlag = true;
	}
}

/*******************************************************************************
 ******************************************************************************/
