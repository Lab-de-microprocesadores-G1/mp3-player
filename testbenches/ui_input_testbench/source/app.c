/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "board.h"

#include "drivers/HAL/encoder/encoder.h"
#include "drivers/HAL/button/button.h"
#include "drivers/HAL/led/led.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// #define SOME_CONSTANT    20
// #define MACRO(x)         (x)

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onLeftClockwise(void);
static void onLeftCounterClockwise(void);
static void onRightClockwise(void);
static void onRightCounterClockwise(void);
static void onPreviousPressed(void);
static void onPlayStopPressed(void);
static void onNextPressed(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

// typedef int  my_int_t;

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

// static int myVar;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	// Initialization of the board
	boardInit();

	// Initialization of the button driver
	buttonInit();
	buttonSubscribe(BUTTON_PREVIOUS, BUTTON_PRESS, onPreviousPressed);
	buttonSubscribe(BUTTON_PLAY_STOP, BUTTON_PRESS, onPlayStopPressed);
	buttonSubscribe(BUTTON_NEXT, BUTTON_PRESS, onNextPressed);

	// Initialization of the led driver
	ledInit();

	// Initialization of the encoder
	encoderInit();
	registerCallbacks(ENCODER_LEFT, onLeftClockwise, onLeftCounterClockwise);
	registerCallbacks(ENCODER_RIGHT, onRightClockwise, onRightCounterClockwise);
	enableEncoder(ENCODER_LEFT);
	enableEncoder(ENCODER_RIGHT);
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
    // Application iterative tasks, every loop runs this function
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void onPreviousPressed(void)
{
	ledClear(LED_RED);
}

static void onPlayStopPressed(void)
{
	ledClear(LED_GREEN);
}

static void onNextPressed(void)
{
	ledClear(LED_BLUE);
}

static void onLeftClockwise(void)
{
	ledClear(LED_RED);
	ledClear(LED_GREEN);
	ledClear(LED_BLUE);

	ledSet(LED_RED);
	ledSet(LED_GREEN);
}

static void onLeftCounterClockwise(void)
{
	ledClear(LED_RED);
	ledClear(LED_GREEN);
	ledClear(LED_BLUE);

	ledSet(LED_BLUE);
	ledSet(LED_GREEN);
}

static void onRightClockwise(void)
{
	ledClear(LED_RED);
	ledClear(LED_GREEN);
	ledClear(LED_BLUE);

	ledSet(LED_BLUE);
	ledSet(LED_RED);
}

static void onRightCounterClockwise(void)
{
	ledClear(LED_RED);
	ledClear(LED_GREEN);
	ledClear(LED_BLUE);

	ledSet(LED_BLUE);
	ledSet(LED_RED);
	ledSet(LED_GREEN);
}


/*******************************************************************************
 ******************************************************************************/
