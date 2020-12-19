/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "../drivers/MCAL/equaliser/equaliser.h"
#include "../libs/spectral_matrix.h"
#include "../drivers/MCAL/cfft/cfft.h"
#include "../drivers/HAL/keypad/keypad.h"
#include "../drivers/HAL/WS2812/WS2812.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define FRAME_SIZE 1042
#define DISPLAY_SIZE	       	  8	// Display side number of digits (8x8)
#define FULL_SCALE 				  7

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void keypadCallback(keypad_events_t event);	// static void privateFunction(void);
static void moveEqBand(uint8_t side);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

typedef enum{
	MOVE_EQ_RIGHT,
	MOVE_EQ_LEFT
}move_eq_band_t;

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static keypad_events_t  keypadEv;
static bool newKeypadEv;

static ws2812_pixel_t kernelDisplayMatrix[DISPLAY_SIZE][DISPLAY_SIZE];
static uint8_t currentEqBand = 0;
static double colValues[8];
static ws2812_pixel_t clear = {0,0,0};


/*******************************************************************************
 * EXTERN VARIABLES
 ******************************************************************************/

extern float32_t sineInputs[8][FRAME_SIZE];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
    //Matrix initialisation
	WS2812Init();
	WS2812SetDisplayBuffer(kernelDisplayMatrix, DISPLAY_SIZE * DISPLAY_SIZE);

	//equalisator initialisation
	eqInit(FRAME_SIZE);

	//Keypad initialisation
	keypadInit();
	keypadSubscribe(keypadCallback);

	//fft initialisation
	cfftInit(FRAME_SIZE);

	//Local variables
	newKeypadEv = false;
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
	if(newKeypadEv)
	{
		switch (keypadEv.source)
		{
			case KEYPAD_ENCODER_LEFT:	// Left encoder moves between selected eq bands.
			{
				if(keypadEv.id == KEYPAD_PRESSED)
				{
					
				}
				else if(keypadEv.id == KEYPAD_ROTATION_CLKW)
				{
					moveEqBand(MOVE_EQ_RIGHT);
				}
				else if(keypadEv.id == KEYPAD_ROTATION_ANTICLKW)
				{
					moveEqBand(MOVE_EQ_LEFT);
				}
				break;
			}
			case KEYPAD_ENCODER_RIGHT:	// Changes equalisation gains of current selected eq band.
			{
				if(keypadEv.id == KEYPAD_PRESSED)
				{

				}
				else if(keypadEv.id == KEYPAD_ROTATION_CLKW)
				{
					
				}
				else if(keypadEv.id == KEYPAD_ROTATION_ANTICLKW)
				{
					
				}
				break;
			}
			case KEYPAD_BUTTON_SW0:
			{
				break;
			}
			case KEYPAD_BUTTON_SW1:
			{
				break;
			}
			case KEYPAD_BUTTON_SW2:
			{
				break;
			}
		}
		newKeypadEv = false;
	}
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void keypadCallback(keypad_events_t event)
{
	keypadEv.source = event.source;
	keypadEv.id = event.id;
	newKeypadEv = true;
}


void moveEqBand(uint8_t side)
{
	if (side == MOVE_EQ_RIGHT)
	{
		
	}
	else
	{
		
	}
	
}

/*******************************************************************************
 ******************************************************************************/
