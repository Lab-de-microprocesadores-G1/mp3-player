/*******************************************************************************
  @file     WS2812.h
  @brief    Smart led WS2812 controller
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef WS2812_H
#define WS2812_H

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// Pixel data structure for each led
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} ws2812_pixel_t;

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// Default hard-coded color palette
#define WS2812_COLOR_RED      { 50, 0, 0 }
#define WS2812_COLOR_BLUE     { 0, 0, 50 }
#define WS2812_COLOR_GREEN    { 0, 50, 0 }
#define WS2812_COLOR_WHITE    { 50, 50, 50 }

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
 * @brief Initializes the WS2812 driver
 */
void WS2812Init(void);

/*
 * @brief Sets display mirroring buffer
 * @param buffer  Array of pixels
 * @param size    Number of pixels in array
 */
void WS2812SetDisplayBuffer(ws2812_pixel_t* buffer, size_t size);

/*
 * @brief Mirrors display buffer to the array of WS2812 leds
 */
void WS2812Update(void);

/*******************************************************************************
 ******************************************************************************/


#endif /* WS2812_H_ */
