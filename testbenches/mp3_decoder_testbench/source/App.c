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

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define FILEPATH 	"PizzaConmigo.mp3"
#define FRAME_SIZE	1024
#define SAMPLE_RATE	44100

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void updateCallback(uint16_t * frameToUpdate);

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool     init;
static FATFS	fat;
static FRESULT	fr;
int16_t         decodedBuffer[MP3_DECODED_BUFFER_SIZE + FRAME_SIZE];
static uint16_t samplesDecoded;
static mp3decoder_frame_data_t frameData;
static mp3decoder_result_t res;
static mp3decoder_tag_data_t ID3Tag;

static uint32_t availableSamples = 0;
static uint16_t frames[2][FRAME_SIZE];

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

	/* Initialize output DAC_DMA */
	dacdmaInit();
	dacdmaSetBuffers(frames[0], frames[1], FRAME_SIZE, updateCallback);
	dacdmaSetFreq(SAMPLE_RATE);

	/* Config test pins */
	gpioMode(PIN_DECODE_TIME, OUTPUT);
	gpioMode(PIN_FILE_TIME, OUTPUT);
	gpioMode(PIN_HELIX_TIME, OUTPUT);
	gpioWrite(PIN_DECODE_TIME, LOW);
	gpioWrite(PIN_FILE_TIME, LOW);
	gpioWrite(PIN_HELIX_TIME, LOW);

	/* Initialization of the MP3 decoder*/
	MP3DecoderInit();
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{
  if (!init)
  {
	/* Mount the default drive */
	fr = f_mount(&fat, "", 1);

	/* Change current directory */
	fr = f_chdir("mp3");

	init = true;
  }

  if (MP3LoadFile(FILEPATH))
  {
	/* Get ID3v2 information */
	MP3GetTagData(&ID3Tag);

	/* Get info from first frame to be decoded */
	MP3GetNextFrameData(&frameData);

	while ((availableSamples <  (3 * FRAME_SIZE)) && (res == MP3DECODER_NO_ERROR))
	{
        gpioWrite(PIN_DECODE_TIME, HIGH);
		/* Decode mp3 frame */
		res = MP3GetDecodedFrame(decodedBuffer + availableSamples, MP3_DECODED_BUFFER_SIZE, &samplesDecoded);
        gpioWrite(PIN_DECODE_TIME, LOW);
		availableSamples += samplesDecoded;
	}
	// dacdmaStart();
	availableSamples = 0;

	while (res == MP3DECODER_NO_ERROR)
	{
		if (true)
		{
	        gpioWrite(PIN_DECODE_TIME, HIGH);
			/* Decode mp3 frame */
			res = MP3GetDecodedFrame(decodedBuffer + availableSamples, MP3_DECODED_BUFFER_SIZE, &samplesDecoded);
	        gpioWrite(PIN_DECODE_TIME, LOW);
	        for (uint32_t i = 0 ; i < 10000 ; i++);
//			availableSamples += samplesDecoded;
	        availableSamples = 0;
		}
	}
	dacdmaStop();
  }
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************/

static void updateCallback(uint16_t * frameToUpdate)
{
	for (uint16_t i = 0 ; i < FRAME_SIZE ; i++)
	{
		frameToUpdate[i] = decodedBuffer[i];
	}
	availableSamples -= FRAME_SIZE;
	memmove(decodedBuffer, decodedBuffer + FRAME_SIZE, availableSamples);
}

/*******************************************************************************
 ******************************************************************************/
