/***************************************************************************//**
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

//Callbacks for keypad functionalities
static void leftEncoderRight();
static void leftEncoderLeft();
static void leftEncoderPressed();
static void rightEncoderRight();
static void rightEncoderLeft();
static void rightEncoderPressed();
static void button0Pressed();
static void button1Pressed();
static void button2Pressed();

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static keypad_events_t event;
static keypad_callback_t userCallback;
/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void keypadInit(void)
{
  //Initialises buttons
	buttonInit();
	buttonSubscribe(BUTTON_ENCODER_LEFT, BUTTON_PRESS, leftEncoderPressed);
	buttonSubscribe(BUTTON_ENCODER_RIGHT, BUTTON_PRESS, rightEncoderPressed);
	buttonSubscribe(BUTTON_SW0, BUTTON_PRESS, button0Pressed);
	buttonSubscribe(BUTTON_SW1, BUTTON_PRESS, button1Pressed);
	buttonSubscribe(BUTTON_SW2, BUTTON_PRESS, button2Pressed);

  //Initialises encoders
	encoderInit();
	encoderRegisterCallbacks(ENCODER_0, leftEncoderRight, leftEncoderLeft);
	encoderRegisterCallbacks(ENCODER_1, rightEncoderRight, rightEncoderLeft);

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

static void leftEncoderRight()
{
	event.source = KEYPAD_ENCODER_LEFT;
	event.id = KEYPAD_ROTATION_CLKW;
	userCallback(event);
}

static void leftEncoderLeft()
{
	event.source = KEYPAD_ENCODER_LEFT;
	event.id = KEYPAD_ROTATION_ANTICLKW;
	userCallback(event);
}

static void leftEncoderPressed()
{
	event.source = KEYPAD_ENCODER_LEFT;
	event.id = KEYPAD_PRESSED;
	userCallback(event);
}

static void rightEncoderRight()
{
	event.source = KEYPAD_ENCODER_RIGHT;
	event.id = KEYPAD_ROTATION_CLKW;
	userCallback(event);
}

static void rightEncoderLeft()
{
	event.source = KEYPAD_ENCODER_RIGHT;
	event.id = KEYPAD_ROTATION_ANTICLKW;
	userCallback(event);
}

static void rightEncoderPressed()
{
	event.source = KEYPAD_ENCODER_RIGHT;
	event.id = KEYPAD_PRESSED;
	userCallback(event);
}

static void button0Pressed()
{
	event.source = KEYPAD_BUTTON_SW0;
	event.id = KEYPAD_PRESSED;
	userCallback(event);
}

static void button1Pressed()
{
	event.source = KEYPAD_BUTTON_SW1;
	event.id = KEYPAD_PRESSED;
	userCallback(event);
}

static void button2Pressed()
{
	event.source = KEYPAD_BUTTON_SW2;
	event.id = KEYPAD_PRESSED;
	userCallback(event);
}
/******************************************************************************/
