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

#define MP3_PC_TESTBENCH    1


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef struct
{
  // Helix structures
  HMP3Decoder   helixDecoder;                                   // Helix MP3 decoder instance 
  MP3FrameInfo  lastFrameInfo;                                  // current MP3 frame info
  
  // MP3 file
  FILE*         mp3File;                                        // MP3 file object
  uint32_t      fileSize;                                       // file size
  uint32_t      bytesRemaining;                                 // Encoded MP3 bytes remaining to be processed by either offset or decodeMP3
  bool          fileOpened;                                     // true if there is a loaded file
  
  // MP3-encoded buffer
  uint8_t       mp3FrameBuffer[MP3_FRAME_BUFFER_BYTES];         // buffer for MP3-encoded frames
  uint32_t      top;                                            // current position in frame buffer (points to top)
  uint32_t      bottom;                                         // current position at info end in frame buffer

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

void MP3DecoderInit(void)
{
  dec.helixDecoder = MP3InitDecoder();
  dec.mp3File = NULL;
  dec.fileOpened = false;
  dec.bottom = 0;
  dec.top = 0;
  dec.fileSize = 0;
  dec.bytesRemaining = 0;
  printf("Decoder initialized. Buffer size is %d bytes\n", MP3_FRAME_BUFFER_BYTES);
}

bool MP3LoadFile(const char* filename)
{
  if(dec.fileOpened)
  {
    //f_close(&(dec.mp3File));          // close prev file
    fclose(dec.mp3File);
    
    // reset context pointers and vars 
    dec.fileOpened = false;
    dec.bottom = 0;
    dec.top = 0;
    dec.fileSize = 0;
    dec.bytesRemaining = 0;
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
    dec.fileSize = ftell(fp); //CREO que aca juega el terminador
    dec.bytesRemaining = dec.fileSize;
    rewind(fp);

    dec.mp3File = fp;
    dec.fileOpened = true;
    
    #ifdef MP3_PC_TESTBENCH
    printf("File opened successfully!\n");
    printf("File size is %d bytes\n", dec.fileSize);
    #endif
    
    return true;
  }
}

uint32_t MP3GetFrameSampleRate(void)
{
  return dec.lastFrameInfo.samprate;
}

mp3decoder_result_t MP3GetDecodedFrame(short* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded)
{
  mp3decoder_result_t ret = MP3DECODER_NO_ERROR;    // Return value of the function
  
  #ifdef MP3_PC_TESTBENCH
  printf("Entered decoding. File has %d bytes to decode\n", dec.fileSize);
  printf("Buffer has %d bytes to decode\n", dec.bottom - dec.top);
  #endif
  
  if(!dec.fileOpened)
  {
    ret = MP3DECODER_NO_FILE;
  }

  //else if(dec.fileSize) // check if info remaining in file and in buffer
  else if (dec.bytesRemaining) // check if there is remaining info to be decoded
  {
    #ifdef MP3_PC_TESTBENCH
    printf("Current pointers are Head = %d - Bottom = %d\n", dec.top, dec.bottom);
    #endif
    
    // scroll encoded info up in array if necessary (TESTED-WORKING)
    if( (dec.top > 0)  && ( (dec.bottom - dec.top ) > 0) && (dec.bottom - dec.top < MP3_FRAME_BUFFER_BYTES))
    {      
        memmove(dec.mp3FrameBuffer , dec.mp3FrameBuffer + dec.top, dec.bottom - dec.top);
        dec.bottom = dec.bottom - dec.top;
        dec.top = 0;
        
        #ifdef MP3_PC_TESTBENCH
        printf("Copied %d bytes from %d to %d\n", (dec.bottom - dec.top), dec.top, 0);
        #endif
    }
    else if (dec.bottom == dec.top)
    {
        // If arrived here, there is nothing else to do
        #ifdef MP3_PC_TESTBENCH
        printf("Empty buffer.\n");
        #endif
    }
    else if (dec.bottom == MP3_DECODED_BUFFER_SIZE)
    {
        #ifdef MP3_PC_TESTBENCH
        printf("Full buffer.\n");
        #endif
    }
  
    // fill buffer with info in mp3 file
    uint8_t* dest = dec.mp3FrameBuffer + dec.bottom; 
    //f_read(&(dec.mp3File), dest, (MP3_FRAME_BUFFER_BYTES - dec.bottomByte), &bytesRead); //! check what happens when bottomByte = 0 (1 element or zero elements)
    uint16_t  bytesRead = fread(dest, 1, (MP3_FRAME_BUFFER_BYTES - dec.bottom), dec.mp3File);

    if (bytesRead == 0)
    {
        #ifdef MP3_PC_TESTBENCH
        printf("File was read completely.\n");
        #endif
    }

    // update bottom pointer
    dec.bottom += bytesRead;
 
    #ifdef MP3_PC_TESTBENCH
    printf("[?] Read %d bytes from file. Head = %d - Bottom = %d\n",bytesRead, dec.top, dec.bottom);
    #endif
    
    // seek mp3 header beginning 
    int offset = MP3FindSyncWord(dec.mp3FrameBuffer + dec.top, dec.bottom);

    if (offset >= 0)
    {
        //! check errors in searching for sync words (there shouldnt be)
        dec.top += offset; // updating top pointer
        dec.bytesRemaining -= offset;  // subtract garbage info to file size
        
        #ifdef MP3_PC_TESTBENCH
        printf("Sync word found @ %d offset\n", offset);
        #endif
    }

     //check samples in next frame (to avoid segmentation fault)
     MP3FrameInfo nextFrameInfo;
     int err = MP3GetNextFrameInfo(dec.helixDecoder, &nextFrameInfo, dec.mp3FrameBuffer + dec.top);
     if (err == 0)
     {
        #ifdef MP3_PC_TESTBENCH
        printf("Frame to decode has %d samples\n", nextFrameInfo.outputSamps);
        #endif
        if (nextFrameInfo.outputSamps > bufferSize)
        {
            #ifdef MP3_PC_TESTBENCH
            printf("Out buffer isnt big enough to hold samples.\n");
            #endif
            return MP3DECODER_BUFFER_OVERFLOW;
        }
     }
     
    // with array organized, lets decode a frame
    uint8_t * decPointer = dec.mp3FrameBuffer + dec.top;
    int bytesLeft = dec.bottom - dec.top;
    int res = MP3Decode(dec.helixDecoder, &decPointer, &(bytesLeft), outBuffer, MP3DECODER_MODE_NORMAL); //! autodecrements fileSize with bytes decoded. updated inbuf pointer, updated bytesLeft
   
    if(res == ERR_MP3_NONE) // if decoding successful
    {
      uint16_t decodedBytes = dec.bottom - dec.top - bytesLeft;

      #ifdef MP3_PC_TESTBENCH
      printf("Frame decoded!. MP3 frame size was %d bytes\n", decodedBytes);
      #endif
      
      // update header pointer and file size
      dec.top += decodedBytes;
      dec.bytesRemaining -= decodedBytes;

      // update last frame decoded info
      MP3GetLastFrameInfo(dec.helixDecoder, &(dec.lastFrameInfo));

      // update samples decoded
      uint8_t scaler = dec.lastFrameInfo.nChans;
      *samplesDecoded = dec.lastFrameInfo.outputSamps / scaler;

      // return success code
      ret = MP3DECODER_NO_ERROR;
    }
    else if (res == ERR_MP3_INDATA_UNDERFLOW || res == ERR_MP3_MAINDATA_UNDERFLOW)
    {
      if (dec.bytesRemaining == 0)
      {
        #ifdef MP3_PC_TESTBENCH
        printf("[Error] Buffer underflow and file empty\n");
        #endif
            
        return MP3DECODER_FILE_END;
      }
      #ifdef MP3_PC_TESTBENCH
      printf("Underflow error (code %d)\n", res);
      #endif
      
      // If there weren't enough bytes on the buffer, try again
      return MP3GetDecodedFrame(outBuffer, bufferSize, samplesDecoded); //! H-quearlo
    }
    else // if (res == -6)
    {        
      //dec.headByte = (dec.headByte + dec.bottomByte) / 2;
      //dec.fileSize -= dec.headByte;
      dec.top++;
      dec.bytesRemaining--;
      #ifdef MP3_PC_TESTBENCH
      printf("Error: %d\n", res);
      #endif
      
      // If invalid header, try with next frame
      return MP3GetDecodedFrame(outBuffer, bufferSize, samplesDecoded); //! H-quearlo
    }
    
  }
  else
  {    ret = MP3DECODER_FILE_END;
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


