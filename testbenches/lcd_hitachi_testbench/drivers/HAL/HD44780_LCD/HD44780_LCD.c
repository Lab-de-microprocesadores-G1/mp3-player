/***************************************************************************//**
  @file     HD44780_LCD.h
  @brief    Hitachi HD44780 LCD High Level Driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <string.h>

#include "drivers/HAL/timer/timer.h"
#include "drivers/HAL/HD44780/HD44780.h"
#include "HD44780_LCD.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// Number of characters left blank before showing initial text
#define HD44780_SPACE_BETWEEN_ROTATIONS		10

// Max number of characters to buffer on each line
#define HD44780_MAX_LINE_LENGTH				50

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef struct {
	// Timer driver resources
	// One for each line to rotate independently
	tim_id_t		idTimer;
	tim_callback_t	timerCallback;

	// Data being showed on the display, if rotated
	// If not rotating, this is not used
	uint8_t 		buffer[HD44780_MAX_LINE_LENGTH];
	uint8_t			bufferSize;

	// Rotation index
	uint8_t			bufferPos;

	// Rotation flag
	bool			rotating;

	// Control flag, set when a function is changing the line content so that
	// the timer callback doesn't print anything
	bool 			changing;
} hd44780_line_context_t;


typedef struct {

	// Initialization flag
	bool initialized;

	// One context for each line for independent management
	hd44780_line_context_t	lineContexts[HD44780_TOTAL_LINES];

} hd44780_lcd_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Sets the cursor position. Next write operation will be displayed at the given place
 * @param line	Cursor position line, can be 0 or 1
 * @param col	Cursor position column, can be 0 through 15
 * @returns 	True if it is a valid cursor position
 */
static bool HD44780SetCursor(uint8_t line, uint8_t col);

// TIMER CALLBACKS
static void line1TimerCallback(void);
static void line2TimerCallback(void);

/**
 * @brief Handles the timeout event for the given line
 * @param line	Line wich has timed out
 */
static void timeoutHandler(uint8_t line);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static hd44780_lcd_context_t context;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void HD44780LcdInit(void)
{
	if (!context.initialized)
	{
		// LCD configurations
		hd44780_cfg_t config  = {
				.inc = HD44780_INC_CURSOR,
				.bothLines = HD44780_BOTH_LINES,
				.shift = HD44780_NO_SHIFT,
				.blink = HD44780_DO_BLINK,
				.cursor = HD44780_SHOW_CURSOR
		};
		// Initialize low level HD44780 driver
		HD44780Init(config);

		// Initialization of the timer driver for display rotation.
		timerInit();
		tim_callback_t timerCallbacks[HD44780_TOTAL_LINES] = { line1TimerCallback, line2TimerCallback };
		for (uint8_t i = 0 ; i < HD44780_TOTAL_LINES ; i++)
		{
			context.lineContexts[i].idTimer = timerGetId();
			context.lineContexts[i].timerCallback = timerCallbacks[i];
		}

		// Set flag
		context.initialized = true;
	}
}

void HD44780WriteChar(uint8_t line, uint8_t col, uint8_t character)
{
	// Stop rotating
	context.lineContexts[line].rotating = false;

	// Set cursor position, check it is valid
	if (HD44780SetCursor(line, col) )
	{
		// Write character
		HD44780WriteData(character);
	}
}

void HD44780WriteString(uint8_t line, uint8_t col, uint8_t * buffer, size_t len)
{
	size_t charAmount;

	// Stop rotating
	context.lineContexts[line].rotating = false;

	// Set cursor position, check it is valid
	if ( HD44780SetCursor(line, col) )
	{
		// Get number of characters to be written, min{len, TOTAL_COLS}
		charAmount = (len + col) < HD44780_TOTAL_COLS ? len : (HD44780_TOTAL_COLS - col);

		// Write each character to the display
		for (uint8_t i = 0 ; i < charAmount ; i++)
		{
			HD44780WriteData(buffer[i]);
		}
	}
}

void HD44780WriteNewLine(uint8_t line, uint8_t * buffer, size_t len)
{
	if (line < HD44780_TOTAL_LINES)
	{
		// Write given characters from line beggining
		HD44780WriteString(line, 0, buffer, len);

		// Complete with spaces if necessary
		if (len < HD44780_TOTAL_COLS)
		{
			for (uint8_t i = 0 ; i < (HD44780_TOTAL_COLS - len) ; i++ )
			{
				HD44780WriteData(' ');
			}
		}
	}
}

void HD44780WriteRotatingString(uint8_t line, uint8_t * buffer, size_t len, uint32_t ms)
{
	hd44780_line_context_t * pLineContext = &(context.lineContexts[line]);

	// Set changing flag to prevent callback from writing to the display
	pLineContext->changing = true;

	// Write string beggining
	HD44780WriteNewLine(line, buffer, len);

	// If rotation is needed
	if (len > HD44780_TOTAL_COLS)
	{
		size_t bufLen = len < HD44780_MAX_LINE_LENGTH ? len : HD44780_MAX_LINE_LENGTH;

		// Copy to internal buffer, save length
		memcpy(pLineContext->buffer, buffer, bufLen);
		pLineContext->bufferSize = bufLen;

		// Set buffer index position
		pLineContext->bufferPos = 1;

		// Start timer
		timerStart(pLineContext->idTimer, TIMER_MS2TICKS(ms), TIM_MODE_PERIODIC, pLineContext->timerCallback);

		// Set rotation flag
		pLineContext->rotating = true;
	}

	// Clear changing flag
	pLineContext->changing = false;

}

void HD44780ClearLine(uint8_t line)
{
	uint8_t aux;

	// Stop rotating
	context.lineContexts[line].rotating = false;

	// Write new line with len = 0, will complete with spaces
	HD44780WriteNewLine(line, &aux, 0);
}

void HD44780ClearDisplay(void)
{
	// Stop rotating both lines
	for (uint8_t i = 0 ; i < HD44780_TOTAL_LINES ; i++)
	{
		context.lineContexts[i].rotating = false;
	}

	// Write instruction
	HD44780WriteInstruction(HD44780_CLEAR_DISPLAY);
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

bool HD44780SetCursor(uint8_t line, uint8_t col)
{
	bool ret = false;
	if ((line < HD44780_TOTAL_LINES) && (col < HD44780_TOTAL_COLS))
	{
		uint8_t cursorAddress = col + line * 0x40;
		HD44780WriteInstruction(HD44780_SET_DDRAM_ADD(cursorAddress));
		ret = true;
	}

	return ret;
}

// TIMER CALLBACKS

void line1TimerCallback(void)
{
	timeoutHandler(0);
}


void line2TimerCallback(void)
{
	timeoutHandler(1);
}

void timeoutHandler(uint8_t line)
{
	hd44780_line_context_t lineContext = context.lineContexts[line];

	if (lineContext.rotating)
	{
		// Check if another service isn't changing something
		if (!lineContext.changing)
		{
			// Set cursor to the beggining of the line
			HD44780SetCursor(line, 0);

			uint8_t index;
			// Write the entire line
			for (uint8_t i = 0 ; i < HD44780_TOTAL_COLS ; i++)
			{
				index = ( (i + lineContext.bufferPos) % (lineContext.bufferSize + HD44780_SPACE_BETWEEN_ROTATIONS));
				if (index < lineContext.bufferSize)
				{	// If showing the message
					HD44780WriteData(lineContext.buffer[index]);
				}
				else
				{	// Spaces between the end of the message and the beggining
					HD44780WriteData(' ');
				}
			}
			// Increment position in the buffer
			context.lineContexts[line].bufferPos = (lineContext.bufferPos + 1) % (lineContext.bufferSize + HD44780_SPACE_BETWEEN_ROTATIONS);
		}
	}
	else
	{ 	// If not rotating, ignore timeout, and stop the timer.
		// Gets here if another service was used for this line
		timerPause(lineContext.idTimer);
	}
}


/******************************************************************************/
