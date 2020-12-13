/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/


#include "drivers/HAL/HD44780_LCD/HD44780_LCD.h"
#include "drivers/HAL/button/button.h"
#include <string.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onSWPressed(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	// Initialize the LCD driver
	HD44780LcdInit();

	// Initialize button driver and subscribe to PRESS event
	buttonInit();
	buttonSubscribe(BUTTON_1, BUTTON_PRESS, onSWPressed);
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{

}

void showMessage(void)
{
	static uint8_t display = 1;
	display = display ? 0 : 0;
	uint8_t cursor;

	for (uint8_t line = 0 ; line < 2 ; line++)
	{
		uint8_t msgLine = line + display * 2;
		for (cursor = 0 ; cursor < 16 ; cursor++)
		{
			if (cursor < strlen(messages[msgLine]))
			{
				HD44780WriteData(messages[msgLine][cursor]);
			}
			else
			{
				HD44780WriteData(' ');
			}
		}
		// Set cursor on 2nd line start
		HD44780WriteInstruction(HD44780_SET_DDRAM_ADD(0x40));
	}
	// Return cursor to 0
	HD44780WriteInstruction(HD44780_SET_DDRAM_ADD(0));
}
/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void onSWPressed(void)
{

}

/*******************************************************************************
 ******************************************************************************/
