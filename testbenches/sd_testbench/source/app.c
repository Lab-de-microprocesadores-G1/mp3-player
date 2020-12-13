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

#define BUFFER_SIZE	512

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onCardInserted(void);
static void onCardRemoved(void);
static void onButtonPressed(void);

bool sdCardInit(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint32_t buffer[BUFFER_SIZE];
static bool start;
static sdhc_data_t data;
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

	/* Status led initialization */
	if (sdhcIsCardInserted())
	{
		ledSet(LED_GREEN);
	}
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
	if (start)
	{
		start = false;
		sdCardInit();
	}
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************/

bool sdCardInit(void)
{
	bool success = false;
	bool forceExit = false;
	uint16_t attempts = 1000;

	// Send 80 clocks to the card.
	// https://electronics.stackexchange.com/questions/303745/sd-card-initialization-problem-cmd8-wrong-response
	sdhcInitializationClocks();

	// GO_IDLE_STATE: Send CMD0, to reset all MMC and SD cards.
	// Comments:
	// 	1. I tested this with the SD card connected, the transfer completed correctly.
	//	2. I tested this with no SD card connected, a CMD line conflit error was raised.
	command.index = 0;
	command.argument = 0;
	command.commandType = SDHC_COMMAND_TYPE_NORMAL;
	command.responseType = SDHC_RESPONSE_TYPE_NONE;
	if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
	{
		// SEND_IF_COND: Send CMD8, asks the SD card if works with the given voltage range, and
		// and sends the check pattern.
		command.index = 8;
		command.argument = 0x1AA;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R7;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			if ((command.response[0] & 0xFF) == 0xAA)
			{
				while (!forceExit && attempts--)
				{
					// APP_CMD: Send CMD55, tells the SD card that the next command is an application specific
					// command. For initialization process a default RCA is used, 0x0000.
					command.index = 55;
					command.argument = 0;
					command.commandType = SDHC_COMMAND_TYPE_NORMAL;
					command.responseType = SDHC_RESPONSE_TYPE_R1;
					if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
					{
						if (command.response[0] & 0x20)
						{
								// SD_SEND_OP_COND: Send ACMD41, sends information about the host to the card to match capabilities
								// HERE YOU COULD BE STORING THE "OCR" REGISTER
								command.index = 41;
								command.argument = (0x1 << 16) | (0x1 << 20) | (0x1 << 21);
								command.commandType = SDHC_COMMAND_TYPE_NORMAL;
								command.responseType = SDHC_RESPONSE_TYPE_R3;
								if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
								{
									if (command.response[0] & (0x1 << 31))
									{
										forceExit = true;
									}
								}
						}
					}
				}

				if (forceExit)
				{
					// ALL_SEND_CID: Send the CMD2, ask all SD cards to send their CID.
					// HERE YOU COULD BE STORING THE "CID" REGISTER
					command.index = 2;
					command.argument = 0;
					command.commandType = SDHC_COMMAND_TYPE_NORMAL;
					command.responseType = SDHC_RESPONSE_TYPE_R2;
					if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
					{
						// SEND_RELATIVE_ADDR: Send the CMD3, ask the card to publish its relative address
						// HERE YOU COULD BE STORING THE "RCA" REGISTER
						command.index = 3;
						command.argument = 0;
						command.commandType = SDHC_COMMAND_TYPE_NORMAL;
						command.responseType = SDHC_RESPONSE_TYPE_R6;
						if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
						{
							ledSet(LED_BLUE);
							success = true;
						}
					}
				}
			}
		}
	}

	if (!success)
	{
		ledSet(LED_RED);
	}
	return success;
}

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
	if (!start)
	{
		start = true;
	}
}

/*******************************************************************************
 ******************************************************************************/
