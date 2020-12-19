/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "MK64F12.h"
#include "drivers/HAL/sd/sd.h"
#include "drivers/HAL/led/led.h"
#include "drivers/HAL/button/button.h"
#include "drivers/MCAL/sdhc/sdhc.h"
#include "board.h"
#include "lib/fatfs/ff.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define BUFFER_SIZE	100

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onButtonPressed(void);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool     init;
static bool 	flag;
static FATFS	fat;
static FIL		file;
static FRESULT	fr;
static char		buffer[BUFFER_SIZE];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	/* Default initialization of the board */
	boardInit();

	/* Initialization of the Button driver */
	buttonInit();
	buttonSubscribe(BUTTON_2, BUTTON_PRESS, onButtonPressed);

	/* Initialization of the LED driver */
	ledInit();
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
	if (!init)
	{
		/* Mount the default drive */
		fr = f_mount(&fat, "", 1);

		init = true;
	}

	if (init && flag)
	{
		flag = false;

		// Test creating some folders
		fr = f_mkdir("user");
		fr = f_mkdir("backup");

		// Test changing current directory
		fr = f_chdir("user");

		// Tests creating a file
		fr = f_open(&file, "random_file.txt", FA_OPEN_ALWAYS | FA_WRITE);

		// Tests writing a file
		if (fr == FR_OK)
		{
			fr = f_printf(&file, "Hello World! JeJe, with ADMA");
		}

		// Tests closing a file
		fr = f_close(&file);
	}
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************/

static void onButtonPressed(void)
{
	if (!flag)
	{
		flag = true;
	}
}

/*******************************************************************************
 ******************************************************************************/
