/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "board/board.h"
#include "events/events.h"
#include "display/display.h"
#include "drivers/HAL/HD44780_LCD/HD44780_LCD.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

typedef struct
{
    uint16_t            volume;                   			 // Music volume
    bool                musicPaused;                         // Pause status
} mp3_player_context_t;

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static mp3_player_context_t	mp3Context;
static event_t event;												// Application loop events
static ws2812_pixel_t display[DISPLAY_COL_SIZE][DISPLAY_ROW_SIZE];	// Application display

/****************************
 * Initial Menu declaration *
 ****************************/
enum {
	MENU_EQUALISER,
	MENU_FILE_EXPLORER,

	MENU_LENGTH
};

const char* initMenuOptions[] = {
		"Equaliser",
		"File explorer"
};

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void appInit (void)
{
	boardInit();
 	eventsInit();
	displayInit();
	HD44780LcdInit();

	//Esto deberiamos ponerlo en otro lado y hacer alguna especie de reproductor
	dacdmaInit();
}

void appRun (void)
{
	event = eventsGetNextEvent();
	if (event.id != EVENTS_NONE)
	{
		if (event.id == EVENTS_PLAY_PAUSE)
		{

		}
		else if (event.id == EVENTS_PREVIOUS)
		{

		}
		else if (event.id == EVENTS_NEXT)
		{

		}
		else if (event.id == EVENTS_LEFT)
		{
			
		}
		else if (event.id == EVENTS_RIGHT)
		{
			
		}
		else if (event.id == EVENTS_ENTER)
		{
			
		}
		else if (event.id == EVENTS_EXIT)
		{
			
		}
		else if (event.id == EVENTS_VOLUME_INCREASE)
		{
			
		}
		else if (event.id == EVENTS_VOLUME_DECREASE)
		{
			
		}
		else if (event.id == EVENTS_VOLUME_TOGGLE)
		{
			
		}
	}
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/


/*******************************************************************************
 ******************************************************************************/
