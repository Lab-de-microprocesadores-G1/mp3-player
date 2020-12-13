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

#define ROTATION_PERIOD	500

#define ICON_POSITION	0										// Lower left
#define HOUR_POSITION	(HD44780_COL_COUNT - strlen(hour) - 2)	// Lower right
#define SIGNAL_POSITION	(HD44780_COL_COUNT - 2)					// Lower right

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

const char * songs[3] = { "Pizza conmigo - Alfredo Casero - Casaeirus", "Angie - The Rolling Stones - Goats Head Soup", "Gardeliando - Los Gardelitos - Gardeliando" };

const char * hour = "2:42";

static uint8_t currentSong = 0;

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
	static bool written = false;
	if ( HD44780LcdInitReady() && !written )
	{
		writeDisplay();
		written = true;
	}
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void onSWPressed(void)
{
	currentSong = (currentSong + 1) % 3;
	writeDisplay();
}

void writeDisplay(void)
{
	HD44780WriteRotatingString(0, songs[currentSong], strlen(songs[currentSong]), ROTATION_PERIOD);

	// Write icon, signal and hour
	HD44780WriteChar(1, ICON_POSITION, HD44780_CUSTOM_MUSIC);
	for (uint8_t i = 1 ; i < HOUR_POSITION ; i += 2)
	{
		HD44780WriteCharAtCursor(HD44780_CUSTOM_MUSIC);
	}
	HD44780WriteString(1, HOUR_POSITION, hour, strlen(hour));
	HD44780WriteCharAtCursor(HD44780_CUSTOM_SIGNAL_A);
	HD44780WriteCharAtCursor(HD44780_CUSTOM_SIGNAL_B);
}

/*******************************************************************************
 ******************************************************************************/
