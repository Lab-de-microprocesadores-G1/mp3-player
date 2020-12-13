/***************************************************************************//**
  @file     HD44780_LCD.h
  @brief    Hitachi HD44780 LCD High Level Driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef HITACHI_LCD_H_
#define HITACHI_LCD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define HD44780_TOTAL_LINES		2
#define HD44780_TOTAL_COLS		16

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initializes the HD44780 LCD driver
 */
void HD44780LcdInit(void);

// WRITING SERVICES

/**
 * @brief Writes the given character to the specified position
 * @param line		Cursor position line, can be 0 or 1
 * @param col		Cursor position column, can be 0 through 15
 * @param character Character to be written
 * Warning: If rotating a string this will stop the rotation.
 * 			Consider introducing the character into the string
 */
void HD44780WriteChar(uint8_t line, uint8_t col, uint8_t character);

/**
 * @brief Writes the given string from the specified position up to col + len.
 * 			If col + len is larger than HD44780_TOTAL_COLS, extra characters are ignored.
 * 			To write longer strings see HD44780WriteRotatingString()
 * @param line		Cursor position line, can be 0 or 1
 * @param col		Cursor position column, can be 0 through 15
 * @param buffer	String to be written
 * @param len		Amount of character to be written
 */
void HD44780WriteString(uint8_t line, uint8_t col, uint8_t * buffer, size_t len);

/**
 * @brief Clears and writes the line with the given characters. If len is less than
 * 			HD44780_TOTAL_COLS, the line is filled with spaces. If it is largen than
 * 			total cols, extra characters are ignored.
 * 			To write longer strings see HD44780WriteRotatingString()
 * @param line		Cursor position line, can be 0 or 1
 * @param buffer	String to be written
 * @param len		Amount of character to be written
 */
void HD44780WriteNewLine(uint8_t line, uint8_t * buffer, size_t len);

/**
 * @brief Clears and writes the line with the given characters. If len is greater than
 * 			HD44780_TOTAL_COLS, the string will be rotated.
 * @param line		Cursor position line, can be 0 or 1
 * @param buffer	String to be written
 * @param len		Amount of character to be written
 * @param ms		Rotation period in milliseconds
 * Note: For safe operation, rotation period should be higher than 20ms
 */
void HD44780WriteRotatingString(uint8_t line, uint8_t * buffer, size_t len, uint32_t ms);

// WRITING SERVICES

/**
 * @brief Clears the given line
 * @param line	Line to be cleared, 0 or 1
 */
void HD44780ClearLine(uint8_t line);

/**
 * @brief Clears the entire display
 */
void HD44780ClearDisplay(void);

/*******************************************************************************
 ******************************************************************************/


#endif /* HITACHI_LCD_H_ */
