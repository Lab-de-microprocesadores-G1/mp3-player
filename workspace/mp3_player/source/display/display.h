/*******************************************************************************
  @file     display.h
  @brief    RGB display abstraction layer
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef DISPLAY_DISPLAY_H_
#define DISPLAY_DISPLAY_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/HAL/WS2812/WS2812.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DISPLAY_ROW_SIZE		(8)
#define DISPLAY_COL_SIZE		(8)
#define DISPLAY_SIZE			  (DISPLAY_ROW_SIZE * DISPLAY_COL_SIZE)
#define DISPLAY_FPS_MS			(1000)

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
 * @brief Initialization of the display.
 */
void displayInit(void);

/**
 * @brief Flip the current display buffer, which must be of the fixed size
 * 		    declared by the DISPLAY_ROW_SIZE and DISPLAY_COL_SIZE.
 * @param buffer	Pointer to the pixel matrix
 */
void displayFlip(ws2812_pixel_t* buffer);

/*******************************************************************************
 ******************************************************************************/

#endif /* DISPLAY_DISPLAY_H_ */
