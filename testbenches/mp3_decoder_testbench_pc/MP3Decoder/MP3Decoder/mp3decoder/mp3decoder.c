/***************************************************************************//**
  @file     mp3decoder.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/
/**** TODO FOR mp3decoder.c ****/
//todo  circular buffer issues (differentiate empty queue and 1 element in queue scenarios)
//todo  handle errors when decoding
//todo  handle errors when finding header pointer


/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "mp3decoder.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>



/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define MP3DECODER_MODE_NORMAL  0

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef struct
{
  // Helix structures
  HMP3Decoder   helixDecoder;                                                   // Helix MP3 decoder instance 
  MP3FrameInfo  lastFrameInfo;                                                  // current MP3 frame info
  
  // MP3 file 
  FILE*         mp3File;                                                        // MP3 file object
  uint32_t      fileSize;                                                       // file size
  bool          fileOpened;                                                     // true if another file is open
  
  // MP3-encoded buffer
  uint8_t       mp3FrameBuffer[MP3_FRAME_BUFFER_BYTES];                         // buffer for MP3-encoded frames
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

static mp3decoder_context_t dec;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void  mp3DecoderInit(void)
{
  dec.helixDecoder = MP3InitDecoder();
  dec.mp3File = NULL;
  dec.fileOpened = false;
  dec.bottomByte = 0;
  dec.headByte = 0;
  dec.fileSize = 0;
  printf("Decoder initialized. Buffer size is %d bytes\n", MP3_FRAME_BUFFER_BYTES);
}

bool  loadFile(const char* filename)
{
  if(dec.fileOpened)
  {
    //f_close(&(dec.mp3File));          // close prev file
    fclose(dec.mp3File);
    
    // reset context pointers and vars 
    dec.fileOpened = false;
    dec.bottomByte = 0;
    dec.headByte = 0;
    dec.fileSize = 0;
  }

  //FRESULT fr = f_open(&(dec.mp3File), filename, FA_READ);
  FILE* fp = fopen(filename, "rb");


  if(fp == NULL)
  {
    return false;
  }

  else
  {
    //dec.fileSize = f_size(&(dec.mp3File));
    /* getting file size */
    fseek(fp, 0L, SEEK_END);
    dec.fileSize = ftell(fp);
    rewind(fp);

    dec.mp3File = fp;
    dec.fileOpened = true;

    printf("File opened successfully!\n");
    printf("File size is %d bytes\n", dec.fileSize);
    return true;
  }
}

uint32_t getFrameSampleRate(void)
{
  return dec.lastFrameInfo.samprate;
}

mp3decoder_result_t getMP3DecodedFrame(int16_t* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded)
{
  mp3decoder_result_t ret = MP3DECODER_NO_ERROR;

  if(!dec.fileOpened)
  {
    ret = MP3DECODER_NO_FILE;
  }

  else if(dec.fileSize) // check if info remaining in file
  {
      printf("Entered decoding. Head = %d - Bottom = %d\n", dec.headByte, dec.bottomByte);
    // scroll encoded info up in array if necessary (TESTED-WORKING)
    if( (dec.headByte > 0)  && ( (dec.bottomByte - dec.headByte ) > 0) )
    {      
        memcpy(
            dec.mp3FrameBuffer,
            dec.mp3FrameBuffer + dec.headByte,
            (dec.bottomByte - dec.headByte) + 1 ); 
        printf("Copied %d bytes from %d to %d\n", (dec.bottomByte - dec.headByte) + 1, dec.headByte, 0);
    }

    // and then update top and bottom info pointers (TESTED-WORKING)
    dec.bottomByte = (dec.bottomByte - dec.headByte);
    dec.headByte = 0;
  
    // fill buffer with info in mp3 file
    uint16_t  bytesRead;
    uint8_t   *dest = dec.mp3FrameBuffer + dec.bottomByte + 1; // different conditions
    //f_read(&(dec.mp3File), dest, (MP3_FRAME_BUFFER_BYTES - dec.bottomByte), &bytesRead); //! check what happens when bottomByte = 0 (1 element or zero elements)
    bytesRead = fread(dest, 1, (MP3_FRAME_BUFFER_BYTES - dec.bottomByte), dec.mp3File);

    // update bottom pointer
    dec.bottomByte += bytesRead;
 
    // update remaining useful bytes in file
    dec.fileSize -= bytesRead;

    printf("[?] Read %d bytes from file. Head = %d - Bottom = %d\n",bytesRead, dec.headByte, dec.bottomByte);

    // seek mp3 header beginning 
    int offset = MP3FindSyncWord(dec.mp3FrameBuffer + dec.headByte, MP3_FRAME_BUFFER_BYTES - dec.headByte);



    if (offset >= 0)
    {
        //! check errors in searching for sync words (there shouldnt be)
        dec.headByte += offset; // updating top pointer
        dec.fileSize -= offset; // subtract garbage info to file size
        printf("Sync word found @ %d offset\n", offset);
    }


     //check samples in next frame (to avoid segmentation fault)
     MP3FrameInfo nextFrameInfo;
     int err = MP3GetNextFrameInfo(dec.helixDecoder, &nextFrameInfo, dec.mp3FrameBuffer + dec.headByte);
     if (err == 0)
     {
         printf("Frame to decode has %d samples\n", nextFrameInfo.outputSamps);
         if (nextFrameInfo.outputSamps > bufferSize)
         {
             return MP3DECODER_BUFFER_OVERFLOW;
         }
     }
        

     if(nextFrameInfo.outputSamps > bufferSize)
     {
       return MP3DECODER_BUFFER_OVERFLOW;
     }

    // with array organized, lets decode a frame
    uint8_t * decPointer = dec.mp3FrameBuffer + dec.headByte;
    int decodedBytes = dec.bottomByte - dec.headByte;
    int res = MP3Decode(dec.helixDecoder, &decPointer, &(decodedBytes), outBuffer, MP3DECODER_MODE_NORMAL); //! autodecrements fileSize with bytes decoded. updated inbuf pointer, updated bytesLeft
   
    if(res == ERR_MP3_NONE) // if decoding successful
    {
      printf("Frame decoded!. MP3 frame size was %d bytes\n", dec.bottomByte - dec.headByte - decodedBytes);
      // update header pointer and file size
      dec.headByte += (dec.bottomByte - dec.headByte - decodedBytes);
      dec.fileSize -= (dec.bottomByte - dec.headByte - decodedBytes);

      // update samples decoded
      *samplesDecoded = decodedBytes;

      // update last frame decoded info
      MP3GetLastFrameInfo(dec.helixDecoder, &(dec.lastFrameInfo));

      // return success code
      ret = MP3DECODER_NO_ERROR;
    }
    else if (res == ERR_MP3_INDATA_UNDERFLOW || res == ERR_MP3_MAINDATA_UNDERFLOW)
    {
        printf("Underflow error (code %d)\n", res);
      return getMP3DecodedFrame(outBuffer, bufferSize, samplesDecoded); //! H-quearlo
    }
    else // if (res == -6)
    {        
      //dec.headByte = (dec.headByte + dec.bottomByte) / 2;
      //dec.fileSize -= dec.headByte;
      dec.headByte++;
      dec.fileSize--;
      printf("Error: %d\n", res);
      return getMP3DecodedFrame(outBuffer, bufferSize, samplesDecoded); //! H-quearlo
    }
    
  }
  else
  {
    ret = MP3DECODER_FILE_END;
  }
  
  return ret;
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


