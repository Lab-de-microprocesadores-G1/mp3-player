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



/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define MP3_DECODED_BUFFER_SIZE (4*1152)                                     // maximum frame size if max bitrate is used (in samples)
#define ID3_MAX_FIELD_SIZE      50

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum
{
  MP3DECODER_NO_ERROR,
  MP3DECODER_ERROR,
  MP3DECODER_FILE_END,
  MP3DECODER_NO_FILE, 
  MP3DECODER_BUFFER_OVERFLOW
} mp3decoder_result_t;

typedef struct
{
    uint16_t    bitRate;
    uint8_t     channelCount;
    uint16_t	sampleRate;
    uint16_t    binitsPerSample;
    uint16_t    sampleCount;
} mp3decoder_frame_data_t;

typedef struct
{
    uint8_t title[ID3_MAX_FIELD_SIZE];
    uint8_t artist[ID3_MAX_FIELD_SIZE];
    uint8_t album[ID3_MAX_FIELD_SIZE];
    uint8_t trackNum[10];
    uint8_t year[10];

} mp3decoder_tag_data_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
* @brief Initialices the mp3 decoder driver
*/
void  MP3DecoderInit(void);

/*
* @brief Initialices the mp3 decoder driver
* @param fileptr pointer to the mp3 file
*/
bool  MP3LoadFile(const char* filename);

/*
* @brief Gives the song's tag data like name, artist, etc
* @param data Pointer to object to be filled with info
* @returns True if there is a valid tag
*/
bool MP3GetTagData(mp3decoder_tag_data_t* data);

/*
* @brief Returns the information of last frame decoded
* @param Pointer to object to be filled with info
* @returns True if there is a valid frame to decode, false if not.
*/
bool MP3GetLastFrameData(mp3decoder_frame_data_t *data);

/*
* @brief Returns the information of next frame to be decoded (if exists)
* @param Pointer to object to be filled with info
* @returns True if there is a valid frame to decode, false if not.
*/
bool MP3GetNextFrameData(mp3decoder_frame_data_t *data);

/*
* @brief  Decodes one mp3 frame (if available) to WAV format
* 
* @param  *outbuffer      pointer to output buffer (should have space for at least one frame samples)
* @param  buffersize      number of available bytes in output buffer
* @param  *samplesDecoded pointer to variable that will be updated with number of samples decoded (if process is successful)
* 
* @returns  result code (MP3DECODER_ERROR, MP3DECODER_NOERROR, MP3DECODER_FILE_END, MP3DECODER_NO_FILE, MP3DECODER_BUFFER_OVERFLOW)
*/
mp3decoder_result_t MP3GetDecodedFrame(short* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded);



/*******************************************************************************
 ******************************************************************************/

#endif /* _MP3DECODER_H_ */