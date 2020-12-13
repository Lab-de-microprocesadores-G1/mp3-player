/***************************************************************************//**
  @file     HD44780.h
  @brief    Hitachi HD44780 LCD Display Low-Level Driver using SPI
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef HD44780_H_
#define HD44780_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// HD44780 instruction definitions
// There are two groups of instructions:

// (1) Instructions without parameters
#define HD44780_CLEAR_DISPLAY	0x01
#define HD44780_RETURN_HOME		0x02

// (2) Instructions with parameters
// Example of usage: HD44780_DISPLAY_CONTROL(1,0,0) turns on the display without
// 					 showing the cursor nor blinking the cursor position
#define HD44780_FUNCTION_SET(dl,n,f) 	((((dl)<<4) | ((n)<<3) | ((f)<<2)) | 0x20)
#define HD44780_DISPLAY_CONTROL(d,c,b)	((((d)<<2)  | ((c)<<2) | ((b)<<0)) | 0x08)
#define HD44780_ENTRY_MODE_SET(id,s)	((((id)<<1) | ((s)<<0)) | 0x04)
#define HD44780_SET_DDRAM_ADD(add)		(((add) & 0x7F) | 0x80)

// Instructions with fixed parameters
#define HD44780_DISPLAY_ON		0x0C
#define HD44780_DISPLAY_OFF		0x08

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// HD44780 Configuration constants
typedef enum {
	HD44780_ONE_LINE,
	HD44780_BOTH_LINES
} hd44780_lines_t;

typedef enum {
	HD44780_5x8_FONT,
	HD44780_5x10_FONT
} hd44780_char_font_t;

typedef enum {
	HD44780_DEC_CURSOR,
	HD44780_INC_CURSOR
} hd44780_inc_t;

typedef enum {
	HD44780_NO_SHIFT,
	HD44780_SHIFT
} hd44780_shift_t;

typedef enum {
	HD44780_NO_CURSOR,
	HD44780_SHOW_CURSOR
} hd44780_cursor_t;

typedef enum {
	HD44780_NO_BLINK,
	HD44780_DO_BLINK
} hd44780_blink_t;

// HD44780 Initial Configuration structure (see constants above)
typedef struct {
	uint8_t bothLines	: 1; // 1 for two lines, 0 for one line
	uint8_t charFont 	: 1; // 1 for 5x10 dot characters (only if 1 line used), 0 for 5x8 dots.
	uint8_t inc			: 1; // 1 cursor increments each write, 0 decrements
	uint8_t shift		: 1; // 1 display shifts after writing
	uint8_t cursor		: 1; // 1 to display cursor
	uint8_t blink		: 1; // 1 for the cursor to blink
} hd44780_cfg_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Starts initialization of the HD44780 LCD display for 4 bit operation
 * @param config	LCD configurations
 */
void HD44780Init(hd44780_cfg_t config);

/**
 * @brief Returns true if the display has been initialized correctly
 */
bool HD44780InitReady(void);

/**
 * @brief Writes to the LCD IR. Asynchronous, can take up to 200us.
 * @param instruction 	HD44780 instruction
 */
void HD44780WriteInstruction(uint8_t instruction);

/**
 * @brief Writes to the LCD DR. Asynchronous, can take up to 200us.
 * @param data		Byte to be written
 */
void HD44780WriteData(uint8_t data);

/*******************************************************************************
 ******************************************************************************/

#endif /* HD44780_H_ */
