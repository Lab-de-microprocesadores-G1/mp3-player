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

// Display pins
#define DISPLAY_SEGA_PIN            PORTNUM2PIN(PC,5)   //! (definir adecuadamente)
#define DISPLAY_SEGB_PIN            PORTNUM2PIN(PC,7)   //! (definir adecuadamente)
#define DISPLAY_SEGC_PIN            PORTNUM2PIN(PC,0)   //! (definir adecuadamente)
#define DISPLAY_SEGD_PIN            PORTNUM2PIN(PC,9)   //! (definir adecuadamente)
#define DISPLAY_SEGE_PIN            PORTNUM2PIN(PC,8)   //!  (definir adecuadamente)
#define DISPLAY_SEGF_PIN            PORTNUM2PIN(PC,1)   //! (definir adecuadamente)
#define DISPLAY_SEGG_PIN            PORTNUM2PIN(PB,19)  //! (definir adecuadamente)
#define DISPLAY_DPOINT_PIN          PORTNUM2PIN(PB,18)  //! (definir adecuadamente)
#define DISPLAY_DECODER_PIN_A       PORTNUM2PIN(PC,3)   //! decoder MSB (definir adecuadamente)
#define DISPLAY_DECODER_PIN_B       PORTNUM2PIN(PC,2)   //! decoder LSB (definir adecuadamente)
#define DISPLAY_ISR_DEV_PIN         PORTNUM2PIN(PA,2)
    

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
