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

// Extern Switches
#define PIN_PREVIOUS	PORTNUM2PIN(PB, 2)	// PTB2
#define PIN_PLAY_STOP	PORTNUM2PIN(PB, 3)	// PTB3
#define PIN_NEXT		PORTNUM2PIN(PB, 10)	// PTB10
#define PIN_LEFT_ENCA	PORTNUM2PIN(PC, 5)	// PTC5
#define PIN_LEFT_ENCB	PORTNUM2PIN(PC, 7)	// PTC7
#define PIN_RIGHT_ENCA	PORTNUM2PIN(PC, 11)	// PTC11
#define PIN_RIGHT_ENCB	PORTNUM2PIN(PC, 10)	// PTC10

// On Board User LEDs
#define PIN_LED_RED     PORTNUM2PIN(PB, 22) // PTB22
#define PIN_LED_GREEN   PORTNUM2PIN(PE, 26) // PTE26
#define PIN_LED_BLUE    PORTNUM2PIN(PB, 21) // PTB21

// On Board User Switches
#define PIN_SW2         PORTNUM2PIN(PC, 6)  // PTC6
#define PIN_SW3         PORTNUM2PIN(PA, 4)  // PTA4

#define SW2_ACTIVE		LOW
#define SW3_ACTIVE		LOW

// Active Status
#define LED_ACTIVE      LOW
    

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
