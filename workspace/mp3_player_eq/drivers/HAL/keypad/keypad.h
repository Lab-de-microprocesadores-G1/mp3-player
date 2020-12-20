/*******************************************************************************
  @file     keypad.h
  @brief    Keypad driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef KEYPAD_H_
#define KEYPAD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define KEYPAD_DOUBLE_PRESS_MS		(450)

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
	KEYPAD_PRESSED,								// Button was pressed
	KEYPAD_DOUBLE_PRESSED,				// Button was pressed two times
	KEYPAD_ROTATION_CLKW,					// Clockwise rotation of a rotative encoder
	KEYPAD_ROTATION_COUNTER_CLKW	// Counter clockwise rotation of a rotative encoder
} keypad_ev_id_t;

typedef enum {
	KEYPAD_ENCODER_LEFT,
	KEYPAD_ENCODER_RIGHT,
	KEYPAD_BUTTON_PREVIOUS,
	KEYPAD_BUTTON_PLAY_PAUSE,
	KEYPAD_BUTTON_NEXT,

	KEYPAD_BUTTON_COUNT
} keypad_source_t;

typedef struct {
	keypad_source_t source;
	keypad_ev_id_t  id;
} keypad_events_t;

typedef void (*keypad_callback_t)(keypad_events_t);

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initialize keypad driver.
 */
void keypadInit(void);

/**
 * @brief Subscribe to keypad events.
 * @param keypadCallback 	Function to call when when keypad produces an event
 */
void keypadSubscribe(keypad_callback_t keypadCallback);

#endif /* KEYPAD_H_ */
