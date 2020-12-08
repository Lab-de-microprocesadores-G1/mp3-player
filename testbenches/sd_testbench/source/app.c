/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "MK64F12.h"
#include "drivers/MCAL/sdhc/sdhc.h"
#include "drivers/HAL/led/led.h"
#include "drivers/HAL/button/button.h"
#include "board.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onCardInserted(void);
static void onCardRemoved(void);
static void onButtonPressed(void);
static void onTransferCompleted(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static sdhc_command_t command;
static sdhc_config_t config = {
	.frequency = 400000,
	.readWatermarkLevel = 64,
	.writeWatermarkLevel = 64
};

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

	/* SDHC Initialization Sequence */
	sdhcInit(config);
	sdhcOnCardInserted(onCardInserted);
	sdhcOnCardRemoved(onCardRemoved);
	sdhcOnTransferCompleted(onTransferCompleted);
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
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
}

static void onButtonPressed(void)
{
	command.index = 0;
	command.argument = 0;
	command.commandType = SDHC_COMMAND_TYPE_NORMAL;
	command.responseType = SDHC_RESPONSE_TYPE_R2;

	sdhcStartTransfer(&command, NULL);
}

static void onTransferCompleted(void)
{
	ledSet(LED_RED);
}

/*******************************************************************************
 ******************************************************************************/
