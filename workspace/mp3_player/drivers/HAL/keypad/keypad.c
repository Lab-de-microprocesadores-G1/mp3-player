/*******************************************************************************
  @file     keypad.c
  @brief    Keypad driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/


/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "keypad.h"
#include "../../MCAL/systick/systick.h"
#include "../../MCAL/gpio/gpio.h"
#include "../../../board/board.h"
#include "../button/button.h"
#include "../encoder/encoder.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

// Internal callback for keypad functionalities
static void onLeftEncoderClockwise(void);
static void onLeftEncoderCounterClockwise(void);
static void onLeftEncoderPressed(void);

static void onRightEncoderClockwise(void);
static void onRightEncoderCounterClockwise(void);
static void onRightEncoderPressed(void);

static void onPreviousButtonPressed(void);
static void onPlayPauseButtonPressed(void);
static void onNextButtonPressed(void);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static keypad_callback_t userCallback;
static bool alreadyInit = false;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void keypadInit(void)
{
	if (!alreadyInit)
	{
		// Raise the already initialized flag for the keypad driver
		alreadyInit = true;

		// Initializes buttons
		buttonInit();
		buttonSubscribe(BUTTON_ENCODER_LEFT, BUTTON_PRESS, onLeftEncoderPressed);
		buttonSubscribe(BUTTON_ENCODER_RIGHT, BUTTON_PRESS, onRightEncoderPressed);
		buttonSubscribe(BUTTON_PREVIOUS, BUTTON_PRESS, onPreviousButtonPressed);
		buttonSubscribe(BUTTON_PLAY_PAUSE, BUTTON_PRESS, onPlayPauseButtonPressed);
		buttonSubscribe(BUTTON_NEXT, BUTTON_PRESS, onNextButtonPressed);

		// Initializes encoders
		encoderInit();
		encoderRegisterCallbacks(ENCODER_LEFT, onLeftEncoderClockwise, onLeftEncoderCounterClockwise);
		encoderRegisterCallbacks(ENCODER_RIGHT, onRightEncoderClockwise, onRightEncoderCounterClockwise);
	}
}

void keypadSubscribe(keypad_callback_t keypadCallback)
{
	userCallback = keypadCallback;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void onLeftEncoderClockwise(void)
{
	keypad_events_t event = {
		.source = KEYPAD_ENCODER_LEFT,
		.id = KEYPAD_ROTATION_CLKW
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onLeftEncoderCounterClockwise(void)
{
	keypad_events_t event = {
		.source = KEYPAD_ENCODER_LEFT,
		.id = KEYPAD_ROTATION_COUNTER_CLKW
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onLeftEncoderPressed(void)
{
	keypad_events_t event = {
		.source = KEYPAD_ENCODER_LEFT,
		.id = KEYPAD_PRESSED
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onRightEncoderClockwise(void)
{
	keypad_events_t event = {
		.source = KEYPAD_ENCODER_RIGHT,
		.id = KEYPAD_ROTATION_CLKW
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onRightEncoderCounterClockwise(void)
{
	keypad_events_t event = {
		.source = KEYPAD_ENCODER_RIGHT,
		.id = KEYPAD_ROTATION_COUNTER_CLKW
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onRightEncoderPressed(void)
{
	keypad_events_t event = {
		.source = KEYPAD_ENCODER_RIGHT,
		.id = KEYPAD_PRESSED
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onPreviousButtonPressed(void)
{
	keypad_events_t event = {
		.source = KEYPAD_BUTTON_PREVIOUS,
		.id = KEYPAD_PRESSED
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onPlayPauseButtonPressed(void)
{
	keypad_events_t event = {
		.source = KEYPAD_BUTTON_PLAY_PAUSE,
		.id = KEYPAD_PRESSED
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

static void onNextButtonPressed(void)
{
	keypad_events_t event = {
		.source = KEYPAD_BUTTON_NEXT,
		.id = KEYPAD_PRESSED
	};

	if (userCallback)
	{
		userCallback(event);
	}
}

/******************************************************************************/
