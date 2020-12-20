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

#define FRAME_SIZE 					1024
#define DISPLAY_SIZE	       	  	8	// Display side number of digits (8x8)
#define FULL_SCALE 				  	300
#define SELECTED_BRIGHTNESS		  	0.6
#define DEFAULT_BRIGHTNESS			0.02

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void keypadCallback(keypad_events_t event);	// static void privateFunction(void);
static void moveEqBand(uint32_t side);
static void upEqGain(void);
static void downEqGain(void);
static void updateGain(void);
static void updateBand(void);

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
static uint32_t currentEqBand = 0;
static float colValues[DISPLAY_SIZE];
static double colBrightness[DISPLAY_SIZE];
static ws2812_pixel_t clearPixel = {0,0,0};

static double eqGainValues[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
static uint32_t eqGains[DISPLAY_SIZE] = {9, 9, 9, 9, 9, 9, 9, 9};		// All eq bands start with 1.0 eq gain.

static float32_t filteredOutput[FRAME_SIZE];
static float32_t cfftInput[FRAME_SIZE*2];
static float32_t cfftOutput[FRAME_SIZE*2];
static float32_t cfftMagOutput[FRAME_SIZE];


/*******************************************************************************
 * EXTERN VARIABLES
 ******************************************************************************/

extern float32_t sineInput[FRAME_SIZE];

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
	cfftInit(CFFT_1024);

	//Local variables
	newKeypadEv = false;

	colBrightness[0] = SELECTED_BRIGHTNESS;
	for (uint32_t i = 1; i < DISPLAY_SIZE; i++)
	{
		colBrightness[i] = DEFAULT_BRIGHTNESS;
	}

	updateGain();
	updateBand();
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
					upEqGain();
					updateBand();
				}
				else if(keypadEv.id == KEYPAD_ROTATION_ANTICLKW)
				{
					downEqGain();
					updateBand();
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

void moveEqBand(uint32_t side)
{
	if (side == MOVE_EQ_RIGHT)
	{
		vumeterSingle((pixel_t *)kernelDisplayMatrix + currentEqBand, colValues[currentEqBand], DISPLAY_SIZE, FULL_SCALE, BAR_MODE + LINEAR_MODE, DEFAULT_BRIGHTNESS);
		currentEqBand = (currentEqBand + 1) % 8;
		vumeterSingle((pixel_t *)kernelDisplayMatrix + currentEqBand, colValues[currentEqBand], DISPLAY_SIZE, FULL_SCALE, BAR_MODE + LINEAR_MODE, SELECTED_BRIGHTNESS);
        WS2812Update();
	}
	else
	{
		vumeterSingle(kernelDisplayMatrix + currentEqBand, colValues[currentEqBand], DISPLAY_SIZE, FULL_SCALE, BAR_MODE + LINEAR_MODE, DEFAULT_BRIGHTNESS);
		currentEqBand == 0 ? currentEqBand = 7 : (currentEqBand--);
		vumeterSingle(kernelDisplayMatrix + currentEqBand, colValues[currentEqBand], DISPLAY_SIZE, FULL_SCALE, BAR_MODE + LINEAR_MODE, SELECTED_BRIGHTNESS);
        WS2812Update();
	}
}

void upEqGain(void)
{
	if (eqGains[currentEqBand] < 18)
		eqGains[currentEqBand]++;

	updateGain();
}

void downEqGain(void)
{
	if (eqGains[currentEqBand] > 0)
		eqGains[currentEqBand]--; 
	
	updateGain();
}

void updateGain(void)
{
	eqSetFilterGain(eqGainValues[eqGains[currentEqBand]], currentEqBand);
	eqFilterFrame(sineInput, filteredOutput);

	for (uint32_t i = 0; i < FRAME_SIZE; i++)
	{
		cfftInput[i*2] = filteredOutput[i];
	}

	cfft(cfftInput, cfftOutput, true);
	cfftGetMag(cfftOutput, cfftMagOutput);

	for (uint32_t i = 0; i < DISPLAY_SIZE; i++)
	{
		arm_mean_f32(cfftMagOutput + i * FRAME_SIZE / DISPLAY_SIZE, FRAME_SIZE / DISPLAY_SIZE, colValues + i);
	}
}

void updateBand(void)
{
	for(int i = 0; i < DISPLAY_SIZE; i++)
	{
		for(int j = 0; j < DISPLAY_SIZE; j++)
		{
			kernelDisplayMatrix[i][j] = clearPixel;
		}
	}
	vumeterMultiple(kernelDisplayMatrix, colValues, DISPLAY_SIZE, FULL_SCALE, BAR_MODE + LINEAR_MODE, colBrightness);
    WS2812Update();
}

/*******************************************************************************
 ******************************************************************************/
