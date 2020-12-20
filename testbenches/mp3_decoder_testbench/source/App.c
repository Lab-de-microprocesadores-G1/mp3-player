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

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define BUFFER_SIZE	100
#define FILEPATH "PizzaConmigo.mp3"

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool     init;
static FATFS	fat;
static FRESULT	fr;
int16_t         decodedBuffer[MP3_DECODED_BUFFER_SIZE];
static uint16_t samplesDecoded;
static mp3decoder_frame_data_t frameData;
static mp3decoder_result_t res;
static mp3decoder_tag_data_t ID3Tag;
static uint32_t framesDecoded = 0;

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


	/* Get info from last frame decoded */
	MP3GetLastFrameData(&frameData);

	do
	{
		/* Decode mp3 frame */
		res = MP3GetDecodedFrame(decodedBuffer, MP3_DECODED_BUFFER_SIZE, &samplesDecoded);
		framesDecoded++;
	}
	while (res == MP3DECODER_NO_ERROR);

	if (res == MP3DECODER_FILE_END)
	{
		while(1);
	}

	while(1);
  }
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************/

/*******************************************************************************
 ******************************************************************************/
