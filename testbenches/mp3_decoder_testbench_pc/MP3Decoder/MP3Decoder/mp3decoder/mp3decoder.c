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
#define MP3_FRAME_BUFFER_BYTES  6913                                         // MP3 buffer size (in bytes)

#define MP3_PC_TESTBENCH
//#define MP3_ARM_TESTBENCH

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

/*
 * @brief Copies next file data available to the available size on internal buffer
 * Increments bottom index to keep pointing to the end of the data
 */
static void flushFileToBuffer();

/*
 * @brief Copies from Helix data structure to own structure
 */
static void copyFrameInfo(mp3decoder_frame_data_t* mp3Data, MP3FrameInfo* helixData);

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
  bool ret = false;

  // Close previous file and context if necessary
  if(dec.fileOpened)
  {
    //f_close(&(dec.mp3File));         
    fclose(dec.mp3File);
    
    // Reset context pointers and vars 
    dec.fileOpened = false;
    dec.bottom = 0;
    dec.top = 0;
    dec.fileSize = 0;
    dec.bytesRemaining = 0;
  }

  // Open new file
  //FRESULT fr = f_open(&(dec.mp3File), filename, FA_READ);
  FILE* fp = fopen(filename, "rb");

  // If successfully opened
  if (fp)
  {
    //dec.fileSize = f_size(&(dec.mp3File));
    /* getting file size */
    fseek(fp, 0L, SEEK_END);
    dec.fileSize = ftell(fp); //CREO que aca juega el terminador
    dec.bytesRemaining = dec.fileSize;
    rewind(fp);

    dec.mp3File = fp;
    dec.fileOpened = true;
    
    flushFileToBuffer();

    #ifdef MP3_PC_TESTBENCH
    printf("File opened successfully!\n");
    printf("File size is %d bytes\n", dec.fileSize);
    #endif
    
    ret = true;
  }
  return ret;
}

bool MP3GetLastFrameData(mp3decoder_frame_data_t* data)
{
    bool ret = false;
    if (dec.bytesRemaining < dec.fileSize)
    {
        copyFrameInfo(data, &dec.lastFrameInfo);
        ret = true;
    }

    return ret;
}

bool MP3GetNextFrameData(mp3decoder_frame_data_t* data)
{
    bool ret = false;
    MP3FrameInfo nextFrame;
    int offset = MP3FindSyncWord(dec.mp3FrameBuffer + dec.top, dec.bottom - dec.top);
    if (offset >= 0)
    {
        int res = MP3GetNextFrameInfo(dec.helixDecoder, &nextFrame, dec.mp3FrameBuffer + dec.top + offset);
        if (res == 0)
        {
            copyFrameInfo(data, &nextFrame);
            ret = true;
        }
    }
    return ret;
}

mp3decoder_result_t MP3GetDecodedFrame(short* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded)
{
  mp3decoder_result_t ret = MP3DECODER_NO_ERROR;    // Return value of the function
  
  #ifdef MP3_PC_TESTBENCH
  printf("Entered decoding. File has %d bytes to decode\n", dec.fileSize);
  printf("Buffer has %d bytes to decode\n", dec.bottom - dec.top);
  #endif
  
  if (!dec.fileOpened)
  {
    ret = MP3DECODER_NO_FILE;
  }
  else if (dec.bytesRemaining) // check if there is remaining info to be decoded
  {
    #ifdef MP3_PC_TESTBENCH
    printf("Current pointers are Head = %d - Bottom = %d\n", dec.top, dec.bottom);
    #endif
    
    // scroll encoded info up in array if necessary (TESTED-WORKING)
    if( (dec.top > 0)  && ( (dec.bottom - dec.top ) > 0) && (dec.bottom - dec.top < MP3_FRAME_BUFFER_BYTES))
    {  
        //memcopy(dec.mp3FrameBuffer , dec.mp3FrameBuffer + dec.top, dec.bottom - dec.top);
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

    // Read encoded data from file
    flushFileToBuffer();
    
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

      // update samples decoded, just keep one channel
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

void flushFileToBuffer()
{
    uint16_t bytesRead;

    // Fill buffer with info in mp3 file
    uint8_t* dest = dec.mp3FrameBuffer + dec.bottom;    

    #ifdef MP3_ARM_TESTBENCH
    f_read(&(dec.mp3File), dest, (MP3_FRAME_BUFFER_BYTES - dec.bottomByte), &bytesRead); //! check what happens when bottomByte = 0 (1 element or zero elements)
    // H-Quearlo
    #endif
    
    #ifdef MP3_PC_TESTBENCH
    bytesRead = fread(dest, 1, (MP3_FRAME_BUFFER_BYTES - dec.bottom), dec.mp3File);
    #endif

    // Update bottom pointer
    dec.bottom += bytesRead;

    #ifdef MP3_PC_TESTBENCH
    if (bytesRead == 0)
    {
        printf("File was read completely.\n");
    }
    printf("[?] Read %d bytes from file. Head = %d - Bottom = %d\n", bytesRead, dec.top, dec.bottom);
    #endif
}


void copyFrameInfo(mp3decoder_frame_data_t* mp3Data, MP3FrameInfo* helixData)
{
    mp3Data->bitRate = helixData->bitrate;
    mp3Data->binitsPerSample = helixData->bitsPerSample;
    mp3Data->channelCount = helixData->nChans;
    mp3Data->sampleRate = helixData->samprate;
    mp3Data->sampleCount = helixData->outputSamps;
}


/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/


