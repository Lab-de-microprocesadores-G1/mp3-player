/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "board.h"
#include "lib/fatfs/ff.h"
#include "lib/mp3decoder/mp3decoder.h"
#include "drivers/MCAL/gpio/gpio.h"
#include "drivers/MCAL/dac_dma/dac_dma.h"
#include <math.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define FILEPATH 	"440tone.mp3"
#define FRAME_SIZE	2048
#define SAMPLE_RATE	44100

#define DAC_PEAK_VALUE	4096

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint16_t mp3ToDac(int16_t sample);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool     init;
static FATFS	fat;
static FRESULT	fr;
static int16_t  decodedBuffer[MP3_DECODED_BUFFER_SIZE +  1];
static uint16_t samplesDecoded;
static mp3decoder_frame_data_t frameData;
static mp3decoder_result_t res;
static mp3decoder_tag_data_t ID3Tag;

static uint32_t 	availableSamples = 0;
static uint16_t 	frames[2][FRAME_SIZE];
static uint8_t 		channelCount;
static bool 		dmaInterrupted = false;

static int huevo = 0;
int16_t currDiff, maxDiff;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	init = false;

	/* Default initialization of the board */
	boardInit();

	/* Config test pins */
	gpioMode(PIN_INTERRUPT, OUTPUT);
	gpioMode(PIN_APP_BLOCK, OUTPUT);
	gpioWrite(PIN_INTERRUPT, LOW);
	gpioWrite(PIN_APP_BLOCK, LOW);

	/* Initialization of the MP3 decoder*/
	MP3DecoderInit();
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
  if (!init)
  {
	do
	{
		/* Mount the default drive */
		fr = f_mount(&fat, "", 1);
	}
	while (fr == FR_NOT_READY);

	/* Change current directory */
	fr = f_chdir(".3pm");

	init = true;
  }

  if (MP3LoadFile(FILEPATH))
  {
	/* Get ID3v2 information */
	MP3GetTagData(&ID3Tag);

	/* Get info from first frame to be decoded */
	MP3GetNextFrameData(&frameData);

	channelCount = frameData.channelCount;

	res = MP3DECODER_NO_ERROR;

	while (res == MP3DECODER_NO_ERROR)
	{
		/* Decode mp3 frame */
		res = MP3GetDecodedFrame(decodedBuffer + 1, MP3_DECODED_BUFFER_SIZE, &samplesDecoded);

		uint16_t aux1, aux2;
		maxDiff = 0;
		for (uint16_t i = 0 ; i < samplesDecoded / channelCount; i++)
		{
			if (i > 0)
			{
				aux1 = mp3ToDac(decodedBuffer[channelCount * i]);
				aux2 = mp3ToDac(decodedBuffer[channelCount * (i-1)]);
				currDiff = abs(aux1 - aux2);
				if (currDiff > maxDiff)
				{
					maxDiff = currDiff;
					huevo++;
					if (maxDiff == 154)
					{
						huevo++;
						huevo--;
					}
				}
			}
		 }
		 // Copy last sample to beginning of array
		decodedBuffer[0] = decodedBuffer[samplesDecoded - 1];
	}

	while(1); // end program

  }
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************/

uint16_t mp3ToDac(int16_t sample)
{
	return sample / 16 + DAC_PEAK_VALUE / 2;
}


/*******************************************************************************
 ******************************************************************************/
