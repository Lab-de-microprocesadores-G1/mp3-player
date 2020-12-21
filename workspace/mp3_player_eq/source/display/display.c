/*******************************************************************************
  @file     display.c
  @brief    RGB display abstraction layer
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/HAL/timer/timer.h"
#include "display.h"

#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DEFAULT_COLUMN 	0.05
#define SELECTED_COLUMN	0.9

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*
 * @brief Callback to be called on FPS event triggered by the timer driver,
 * 		  used to update the display.
 */
static void	onDisplayFpsUpdate(void);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static const ws2812_pixel_t    clearPixel = WS2812_COLOR_BLACK;			// Clear byte

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static ws2812_pixel_t 	displayBuffer[DISPLAY_SIZE];	// Internal display buffer
static bool 			alreadyInit = false;			// Internal flag for initialization process
static bool				displayLocked = false;			// Internal flag for avoid updates when changing display content
static uint8_t			currentCol;						// Column selected by user
static bool				displayChanged = false;			// Internal flag for display changes

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void displayInit(void)
{
	if (!alreadyInit)
	{
		// Raise the already initialized flag, to avoid multiple initialization
		alreadyInit = true;
		currentCol = DISPLAY_UNSELECT_COLUMN;

		// Initialization of the lower layer WS2812 driver
		WS2812Init();
		WS2812SetDisplayBuffer(displayBuffer, DISPLAY_SIZE);

		// Initialization of the timer driver
		timerInit();
		timerStart(timerGetId(), TIMER_MS2TICKS(DISPLAY_FPS_MS), TIM_MODE_PERIODIC, onDisplayFpsUpdate);

		// Clear the display
		displayClear();
	}
}

void displayFlip(ws2812_pixel_t* buffer)
{
	displayLocked = true;
	for (uint32_t i = 0; i < DISPLAY_COL_SIZE ; i++)
	{
		for (uint32_t j = 0 ; j < DISPLAY_ROW_SIZE ; j++)
		{
			if (currentCol == DISPLAY_UNSELECT_COLUMN)
			{
				displayBuffer[i * DISPLAY_ROW_SIZE + j] = buffer[i * DISPLAY_ROW_SIZE + j];
			}
			else
			{
				displayBuffer[i * DISPLAY_ROW_SIZE + j].r = buffer[i * DISPLAY_ROW_SIZE + j].r * (currentCol == j ? SELECTED_COLUMN : DEFAULT_COLUMN);
				displayBuffer[i * DISPLAY_ROW_SIZE + j].g = buffer[i * DISPLAY_ROW_SIZE + j].g * (currentCol == j ? SELECTED_COLUMN : DEFAULT_COLUMN);
				displayBuffer[i * DISPLAY_ROW_SIZE + j].b = buffer[i * DISPLAY_ROW_SIZE + j].b * (currentCol == j ? SELECTED_COLUMN : DEFAULT_COLUMN);
			}
		}
	}
	displayLocked = false;
	displayChanged = true;
}

void displaySelectColumn(uint8_t colNumber)
{
	currentCol = colNumber;
}

void displayClear(void)
{
	displayLocked = true;
	for(int i = 0; i < DISPLAY_COL_SIZE; i++)
	{
		for(int j = 0; j < DISPLAY_COL_SIZE; j++)
		{
		  displayBuffer[i * DISPLAY_ROW_SIZE + j] = clearPixel;
		}
	}
	displayLocked = false;
	displayChanged = true;
}
/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void	onDisplayFpsUpdate(void)
{
	if (!displayLocked)
	{
		if (displayChanged)
		{
			WS2812Update();
		}
	}
}

/*******************************************************************************
 *******************************************************************************
						INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
