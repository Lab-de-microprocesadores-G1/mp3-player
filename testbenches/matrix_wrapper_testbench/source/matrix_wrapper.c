/********************************************************************************
  @file     matrix_wrapper_testbench.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "matrix_wrapper.h"
#include  "../drivers/HAL/timer/timer.h"
#include  <math.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define GRAPHIC_MODE_MASK  	0xF
#define SCALE_MODE_MASK		0xC0
#define NUMBER_OF_ROWS		8

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void modeDispatcher(vumeter_modes_t mode, pixel_t* col);

/**
 * @brief Generates matrix for bar vumeter
 * @param stream 	Array of digits, length DISPLAY_LENGTH
 */
static void barMode(pixel_t* col);

static void centreMode(pixel_t* col);
/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

const pixel_t vumeterPixelColours[NUMBER_OF_ROWS] = {{250,0,0},
										 {250,130,0},
										 {250,240,0},
										 {20, 218, 0},
										 {11, 158, 227},
										 {113, 6, 227},
										 {151, 6, 227},
										 {223, 6, 227}};


/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint8_t normalizedValue;
static uint8_t amountOfCols;
static double col_brightness;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void vumeterMultiple(pixel_t* input, float* colValues, uint8_t colQty, double fullScale, vumeter_modes_t mode, double* brightness)
{
	for(int i = 0; i < colQty; i++)
	{
		vumeterSingle(input + i, colValues[i], colQty, fullScale, mode, brightness[i]);
	}
}

void vumeterSingle(pixel_t* col, float value, uint8_t colQty, double fullScale, vumeter_modes_t vumeterMode, double brightness)
{
	vumeter_modes_t graphicMode = vumeterMode & GRAPHIC_MODE_MASK;
	vumeter_modes_t scaleMode = vumeterMode & SCALE_MODE_MASK;
	amountOfCols = colQty;
	col_brightness = brightness;

	if(scaleMode == LOGARITHMIC_MODE)
	{
		if(value < 1)
		{
			normalizedValue = 0;
		}
		else
		{
			normalizedValue = (int)(log10(value) / log10(fullScale) * NUMBER_OF_ROWS + 0.5);
		}
	}
	else if(scaleMode == LINEAR_MODE)
	{
		normalizedValue = (int)(value / fullScale * NUMBER_OF_ROWS + 0.5);
	}
	modeDispatcher(graphicMode, col);
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void modeDispatcher(vumeter_modes_t mode, pixel_t* col)
{
	switch (mode)
	{
		case BAR_MODE:
		{
			barMode(col);
			break;
		}
		case CENTRE_MODE:
		{
			centreMode(col);
			break;
		}
		default:
			break;
	}
}

static void barMode(pixel_t* col)
{
	//Filling the colour matrix for bar mode
	for(uint8_t i = 0; i < normalizedValue; i++)
	{
		col[amountOfCols*i].r = vumeterPixelColours[i].r * col_brightness;
		col[amountOfCols*i].g = vumeterPixelColours[i].g * col_brightness;
		col[amountOfCols*i].b = vumeterPixelColours[i].b * col_brightness;
	}
}

static void centreMode(pixel_t* col)
{
	//Filling the colour matrix for centre mode
	for(uint8_t i = 0; i < normalizedValue/2; i++)
	{
		col[amountOfCols*NUMBER_OF_ROWS/2 + i*amountOfCols].r = vumeterPixelColours[i].r * col_brightness;
		col[amountOfCols*NUMBER_OF_ROWS/2 + i*amountOfCols].g = vumeterPixelColours[i].g * col_brightness;
		col[amountOfCols*NUMBER_OF_ROWS/2 + i*amountOfCols].b = vumeterPixelColours[i].b * col_brightness;
		col[amountOfCols*(NUMBER_OF_ROWS/2 - 1) - i*amountOfCols].r = vumeterPixelColours[i].r * col_brightness;
		col[amountOfCols*(NUMBER_OF_ROWS/2 - 1) - i*amountOfCols].g = vumeterPixelColours[i].g * col_brightness;
		col[amountOfCols*(NUMBER_OF_ROWS/2 - 1) - i*amountOfCols].b = vumeterPixelColours[i].b * col_brightness;
	}
}


/*******************************************************************************
 ******************************************************************************/
