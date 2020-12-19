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
/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void keypadCallback(keypad_events_t event);// static void privateFunction(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

// typedef int  my_int_t;

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static keypad_events_t  keypadEv;
static bool newKeypadEv;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
    eqInit(FRAME_SIZE);
	WS2812Init();
	keypadInit();
	keypadSubscribe(keypadCallback);
	cfftInit(FRAME_SIZE);
	newKeypadEv = false;

}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
	if(newKeypadEv)
	{
		switch (keypadEv.source)
		{
			case KEYPAD_PRESSED:
			{
				break;
			}
			case KEYPAD_REALEASED:
			{
				break;
			}
			case KEYPAD_PRESSED:
			{
				break;
			}
			case KEYPAD_PRESSED:
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

/*******************************************************************************
 ******************************************************************************/
