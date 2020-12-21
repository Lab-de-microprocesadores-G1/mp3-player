/***************************************************************************//**
  @file     board.h
  @brief    Board management
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef _BOARD_H_
#define _BOARD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "../drivers/MCAL/gpio/gpio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/***** BOARD defines **********************************************************/

// Testpoints
#define PIN_PROCESSING				PORTNUM2PIN(PC, 16)	// PTC16
#define PIN_FRAME_FINISHED			PORTNUM2PIN(PB, 18)	// PTB18

// On Board User Switches
#define PIN_SW2         			PORTNUM2PIN(PC, 6)  // PTC6
#define PIN_SW3         			PORTNUM2PIN(PA, 4)  // PTA4

// Matrix WS2812 pins
#define PIN_WS2812_DIN				PORTNUM2PIN(PC, 1)	// PTC1

// Left rotative encoder pins
#define PIN_LEFT_ENCODER_A      	PORTNUM2PIN(PA, 2)	// PTA2
#define PIN_LEFT_ENCODER_B			PORTNUM2PIN(PB, 23)	// PTB23
#define PIN_LEFT_ENCODER_BUTTON		PORTNUM2PIN(PB, 9)	// PTB9

// Right rotative encoder pins
#define PIN_RIGHT_ENCODER_A			PORTNUM2PIN(PC, 5)  // PTC5
#define PIN_RIGHT_ENCODER_B			PORTNUM2PIN(PC, 7)  // PTC7
#define PIN_RIGHT_ENCODER_BUTTON	PORTNUM2PIN(PC, 0)	// PTC0

// Button pins
#define PIN_PREVIOUS_BUTTON         PORTNUM2PIN(PC, 9)  // PTC9
#define PIN_PLAY_PAUSE_BUTTON       PORTNUM2PIN(PC, 8)  // PTC8
#define PIN_NEXT_BUTTON             PORTNUM2PIN(PB, 19) // PTC1

// On Board User LEDs
#define PIN_LED_RED     			PORTNUM2PIN(PB, 22) // PTB22
#define PIN_LED_GREEN   			PORTNUM2PIN(PE, 26) // PTE26
#define PIN_LED_BLUE    			PORTNUM2PIN(PB, 21) // PTB21

// Active Status
#define SW2_ACTIVE					LOW
#define SW3_ACTIVE					LOW
#define LED_ACTIVE      			LOW
#define BUTTONS_ACTIVE  			LOW

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Basic board initialization, default settings
 */
void boardInit(void);

/*******************************************************************************
 ******************************************************************************/

#endif // _BOARD_H_
