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

// Custom characters row count (8th row is the same as the cursor)
#define HD44780_CHARACTER_ROWS				8

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

	// Custom characters set-up variables
	tim_id_t	customCharTimer;	// Timer to execute each step
	uint8_t		currentCharCode;	// Writen character codes counter
	bool		customCharsReady;	// Flag indicating the characters were written

	// One context for each line for independent management
	hd44780_line_context_t	lineContexts[HD44780_LINE_COUNT];

} hd44780_lcd_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Writes a new special character to the LCD CGRAM.
 * @param code 	character code of the new custom character
 */
static void writeCustomChar(uint8_t code);

/**
 * @brief Sets the cursor position. Next write operation will be displayed at the given place
 * @param line	Cursor position line, can be 0 or 1
 * @param col	Cursor position column, can be 0 through 15
 * @returns 	True if it is a valid cursor position
 */
static bool HD44780SetCursor(uint8_t line, uint8_t col);

// TIMER CALLBACKS
static void customTimerCallback(void);
static void line1TimerCallback(void);
static void line2TimerCallback(void);

/**
 * @brief Callback for the initReady event of the low-level driver.
 * 			Triggers the set-up of the custom characters
 */
static void onInitReady(void);

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

// LCD CUSTOM CHARACTERS DEFINITION

static const uint8_t customCharacters[HD44780_CUSTOM_COUNT][HD44780_CHARACTER_ROWS] =
{
		{},
	// 	HD44780_CUSTOM_FOLDER
	{
		0b11100,
		0b10111,
		0b10001,
		0b10001,
		0b10001,
		0b10001,
		0b10001,
		0b11111
	},
	// HD44780_CUSTOM_MUSIC
	{
		0b00110,
		0b00101,
		0b00111,
		0b00101,
		0b00101,
		0b11101,
		0b11011,
		0b00011
	},
	// HD44780_CUSTOM_PAUSE
	{
		0b00000,
		0b01010,
		0b01010,
		0b01010,
		0b01010,
		0b01010,
		0b01010,
		0b00000
	},
	// HD44780_CUSTOM_PLAY
	{
		0b10000,
		0b11000,
		0b11100,
		0b11110,
		0b11100,
		0b11000,
		0b10000,
		0b00000
	},
};

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
		// Initialize low level HD44780 driver and subscribe for initReady event
		HD44780Init(config);
		HD44780onInitReady(onInitReady);

		// Initialization of the timer driver for display rotation and custom characters setup
		timerInit();
		tim_callback_t timerCallbacks[HD44780_LINE_COUNT] = { line1TimerCallback, line2TimerCallback };
		for (uint8_t i = 0 ; i < HD44780_LINE_COUNT ; i++)
		{
			context.lineContexts[i].idTimer = timerGetId();
			context.lineContexts[i].timerCallback = timerCallbacks[i];
		}
		context.customCharTimer = timerGetId();

		// Set flags
		context.initialized = true;
		context.customCharsReady = false;
	}
}

bool HD44780LcdInitReady(void)
{
	return context.customCharsReady;
}

// WRITING CURSOR-INDEPENDENT SERVICES

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
		charAmount = (len + col) < HD44780_COL_COUNT ? len : (HD44780_COL_COUNT - col);

		// Write each character to the display
		for (uint8_t i = 0 ; i < charAmount ; i++)
		{
			HD44780WriteData(buffer[i]);
		}
	}
}

void HD44780WriteNewLine(uint8_t line, uint8_t * buffer, size_t len)
{
	if (line < HD44780_LINE_COUNT)
	{
		// Write given characters from line beggining
		HD44780WriteString(line, 0, buffer, len);

		// Complete with spaces if necessary
		if (len < HD44780_COL_COUNT)
		{
			for (uint8_t i = 0 ; i < (HD44780_COL_COUNT - len) ; i++ )
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
	if (len > HD44780_COL_COUNT)
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

// WRITING CURSOR-DEPENDENT SERVICES

void HD44780WriteCharAtCursor(uint8_t character)
{
	// Write character
	HD44780WriteData(character);
}

void HD44780WriteStringAtCursor(uint8_t * buffer, size_t len)
{
	// Write each character to the display
	for (uint8_t i = 0 ; i < len ; i++)
	{
		HD44780WriteData(buffer[i]);
	}
}

// CLEARING SERVICES

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
	for (uint8_t i = 0 ; i < HD44780_LINE_COUNT ; i++)
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
	if ((line < HD44780_LINE_COUNT) && (col < HD44780_COL_COUNT))
	{
		uint8_t cursorAddress = col + line * 0x40;
		HD44780WriteInstruction(HD44780_SET_DDRAM_ADD(cursorAddress));
		ret = true;
	}

	return ret;
}

void writeCustomChar(uint8_t code)
{
	// Write CGRAM address (6 bits), upper 3 bits determine character code
	HD44780WriteInstruction(HD44780_SET_CGRAM_ADD((code & 0x07) << 3) );

	// Write CGRAM data, once for each row of the character.
	for ( uint8_t i = 0 ; i < HD44780_CHARACTER_ROWS ; i++ )
	{
		HD44780WriteData(customCharacters[code][i]);
	}

	// Write a DDRAM address again so that next data write goes to DDRAM
	HD44780WriteInstruction(HD44780_SET_DDRAM_ADD(0));
}

void onInitReady(void)
{
	// Initialize sequence of steps to send to the LCD
	context.currentCharCode = 0;
	context.customCharsReady = false;
	timerStart(context.customCharTimer, 1, TIM_MODE_PERIODIC, customTimerCallback);
}

// TIMER CALLBACKS

void customTimerCallback(void)
{
	// If all steps were executed, stop timer
	if (context.currentCharCode == HD44780_CUSTOM_COUNT)
	{
		timerPause(context.customCharTimer);
		context.customCharsReady = true;
	}
	else
	{
		// Write current char code to CGRAM
		writeCustomChar(context.currentCharCode++);
	}


}

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
			for (uint8_t i = 0 ; i < HD44780_COL_COUNT ; i++)
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
