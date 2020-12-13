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
#include <stdint.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define ROTATION_PERIOD_LINE1	300
#define ROTATION_PERIOD_LINE2	500


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


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void onSWPressed(void)
{
	const char * messages[2] = { "Pizza conmigo - Alfredo Casero - Casaeirus", "            3:31" };

	HD44780WriteRotatingString(0, messages[0], strlen(messages[0]), ROTATION_PERIOD_LINE1);
	HD44780WriteRotatingString(1, messages[1], strlen(messages[1]), ROTATION_PERIOD_LINE2);
}

/*******************************************************************************
 ******************************************************************************/
