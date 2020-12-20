
/*******************************************************************************
  @file     app.c
  @brief    App 
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/


/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "matrix_wrapper.h"
#include  "../drivers/HAL/WS2812/WS2812.h"
#include  "../drivers/HAL/timer/timer.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DISPLAY_SIZE	       	  8	// Display side number of digits (8x8)
#define FULL_SCALE 				  7

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static ws2812_pixel_t kernelDisplayMatrix[DISPLAY_SIZE][DISPLAY_SIZE];

static tim_id_t timer_id;

static float colValues[8] = {7,2,0,4,1,0,7,6};
static ws2812_pixel_t clear = {0,0,0};

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void appInit(void)
{
    WS2812Init();
    timerInit();
    WS2812SetDisplayBuffer(kernelDisplayMatrix, DISPLAY_SIZE*DISPLAY_SIZE);
    timer_id = timerGetId();
    timerStart(timer_id, TIMER_MS2TICKS(300), TIM_MODE_PERIODIC, NULL);
}

void appRun()
{
    if(timerExpired(timer_id))
    {
    	for(int i = 0; i < DISPLAY_SIZE; i++)
    	{
    		for(int j = 0; j < DISPLAY_SIZE; j++)
			{
				kernelDisplayMatrix[i][j] = clear;
			}
    	}
    	for(int i = 0; i < 8; i++)
    	{
    		colValues[i] = rand() % (FULL_SCALE + 1);
    	}
    	vumeterMultiple(kernelDisplayMatrix, colValues, 8, FULL_SCALE, BAR_MODE + LINEAR_MODE);
        WS2812Update();
    }

}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
