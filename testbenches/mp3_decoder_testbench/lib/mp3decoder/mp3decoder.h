/***************************************************************************//**
  @file     mp3decoder.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef _MP3DECODER_H_
#define _MP3DECODER_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include  <stdbool.h>
#include  <stdint.h>
#include  "../helix/pub/mp3dec.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define MP3_FRAME_BUFFER_BYTES  6913                                         // MP3 buffer size (in bytes)
#define MP3_DECODED_BUFFER_SIZE (4*1152)                                     // maximum frame size if max bitrate is used (in samples)


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum
{
  MP3DECODER_NO_ERROR,
  MP3DECODER_ERROR,
  MP3DECODER_FILE_END,
  MP3DECODER_NO_FILE 
  MP3DECODER_BUFFER_OVERFLOW
}mp3decoder_result_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
* @brief Initialices the mp3 decoder driver
*/
void  mp3DecoderInit(void);

/*
* @brief Initialices the mp3 decoder driver
* @param fileptr pointer to the mp3 file
*/
bool  loadFile(const char* filename);

/*
* @brief Returns the bitrate of the actual frame
*/
uint32_t getFrameSampleRate(void);


/*
* @brief  Decodes one mp3 frame (if available) to WAV format
* 
* @param  *outbuffer      pointer to output buffer (should have space for at least one frame samples)
* @param  buffersize      number of available bytes in output buffer
* @param  *samplesDecoded pointer to variable that will be updated with number of samples decoded (if process is successful)
* 
* @returns  result code (MP3DECODER_ERROR, MP3DECODER_NOERROR, MP3DECODER_FILE_END, MP3DECODER_NO_FILE, MP3DECODER_BUFFER_OVERFLOW)
*/
mp3decoder_result_t getMP3DecodedFrame(int16_t* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded);


/*******************************************************************************
 ******************************************************************************/



#endif /* _MP3DECODER_H_ */