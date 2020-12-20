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

// Encoder Pins
#define ENCODER0_A_PIN        			PORTNUM2PIN(PA, 2)	// PTA2
#define ENCODER0_B_PIN        			PORTNUM2PIN(PB, 23)	// PTB23
#define PIN_ENCODER_0_BUTTON			PORTNUM2PIN(PB, 9)	// PTB9

#define ENCODER1_A_PIN            		PORTNUM2PIN(PC, 5)  // PTC5
#define ENCODER1_B_PIN            		PORTNUM2PIN(PC, 7)  // PTC7
#define PIN_ENCODER_1_BUTTON			PORTNUM2PIN(PC, 0)	// PTC0

//Button pins
#define PIN_BUTTON0                   	PORTNUM2PIN(PC, 9)  //PTC9
#define PIN_BUTTON1                   	PORTNUM2PIN(PC, 8)  //PTC8
#define PIN_BUTTON2                   	PORTNUM2PIN(PB, 19)  //PTB19

#define BUTTONS_ACTIVE  LOW
    

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
