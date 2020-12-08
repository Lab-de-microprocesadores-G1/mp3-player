/***************************************************************************//**
  @file     mp3decoder.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "mp3decoder.h"
#include <cstring.h>



/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef struct
{
  // Helix structures
  HMP3Decoder   helixDecoder;                                                   // Helix MP3 decoder instance 
  MP3FrameInfo  lastFrameInfo;                                                  // current MP3 frame info
  
  // MP3 file 
  FIL           mp3File;                                                        // MP3 file object
  uint32_t      fileSize                                                        // file size
  bool          fileOpened;                                                     // true if another file is open
  
  // MP3-encoded buffers
  uint8_t       mp3FrameBuffer[MP3_FRAME_BUFFER_BYTES];                         // buffer for MP3-encoded frames
  uint8_t       mp3CurrentBuffer;                                               // current ping-pong buffer
  uint32_t      headByte;                                                       // current position in frame buffer (points to header)
  uint32_t      bottomByte;                                                     // current position at info end in frame buffer


}mp3decoder_context_t;


/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static mp3decoder_context_t decoderContext;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void  mp3DecoderInit(void)
{
  decoderContext.helixDecoder = MP3InitDecoder();
  decoderContext.mp3File = NULL;
  decoderContext.fileOpened = false;
  decoderContext.bottomByte = MP3_DECODED_BUFFER_SIZE;
  decoderContext.headerByte = 0;
  decoderContext.fileSize = 0;
}

bool  loadFile(const char* filename)
{
  if(decoderContext.fileOpened)
  {
    f_close(&(decoderContext.mp3File));          // close prev file
    
    // reset context pointers and vars 
    decoderContext.fileOpened = false;
    decoderContext.bottomByte = MP3_DECODED_BUFFER_SIZE;
    decoderContext.headerByte = 0;
    decoderContext.fileSize = 0;
  }

  FRESULT fr = f_open(&(decoderContext.mp3File), filename, FA_READ);

  if(fr)
  {
    return false;
  }

  else
  {
    decoderContext.fileSize = f_size(&(decoderContext.mp3File));
    decoderContext.fileOpened = true;
    return true;
  }
}


uint32_t getFrameSampleRate(void)
{
  return decoderContext.lastFrameInfo.samprate;
}

mp3decoder_result_t getMP3DecodedFrame(int16_t* outBuffer, uint16_t outBufferSize, uint16_t* samplesDecoded)
{
  if(decoderContext.fileSize)
  {
    //memcpy(decoderContext.mp3FrameBuffer, decoderContext.mp3FrameBuffer + decoderContext.bottomByte, )
  }
}
/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/


