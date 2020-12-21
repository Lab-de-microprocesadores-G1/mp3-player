/***************************************************************************//**
  @file     mp3decoder.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <string.h>
#include <stdbool.h>  
#include "mp3decoder.h"
#include  "lib/helix/pub/mp3dec.h"
#include "lib/id3tagParser/read_id3.h"
#include <math.h>

#ifdef __arm__
#include "lib/fatfs/ff.h"
#endif

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define MP3DECODER_MODE_NORMAL  0
#define MP3_FRAME_BUFFER_BYTES  6913            // MP3 buffer size (in bytes)
#define DEFAULT_ID3_FIELD       "Unknown"
#define MP3_REC_MAX_DEPTH       5

#ifndef __arm__
// #define MP3_PC_TESTBENCH
#endif

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef struct
{
  // Helix structures
  HMP3Decoder   helixDecoder;                                   // Helix MP3 decoder instance 
  MP3FrameInfo  lastFrameInfo;                                  // current MP3 frame info
  
  // MP3 file
  #ifdef __arm__
  FIL			file;
  FIL*          mp3File;
  #else
  FILE*         mp3File;                                        // MP3 file object
  #endif
  uint32_t      fileSize;                                       // file size
  uint32_t      bytesRemaining;                                 // Encoded MP3 bytes remaining to be processed by either offset or decodeMP3
  bool          fileOpened;                                     // true if there is a loaded file
  uint16_t      lastFrameLength;                                // Last frame length
  
  // MP3-encoded buffer
  uint8_t       mp3FrameBuffer[MP3_FRAME_BUFFER_BYTES];         // buffer for MP3-encoded frames
  uint32_t      top;                                            // current position in frame buffer (points to top)
  uint32_t      bottom;                                         // current position at info end in frame buffer

  // ID3 tag
  bool                  hasID3Tag;                              // True if the file has valid ID3 tag
  mp3decoder_tag_data_t ID3Data;                                // Parsed data from ID3 tag

} mp3decoder_context_t;



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

/*
 * @brief Reads ID3 tag from MP3 file and updates file pointer after tag info
 */
static void readID3Tag(void);

/*
* @brief  Recursively decodes one mp3 frame (if available) to WAV format
* 
* @param  *outbuffer      pointer to output buffer (should have space for at least one frame samples)
* @param  buffersize      number of available bytes in output buffer
* @param  *samplesDecoded pointer to variable that will be updated with number of samples decoded (if process is successful)
* 
* @returns  result code (MP3DECODER_ERROR, MP3DECODER_NOERROR, MP3DECODER_FILE_END, MP3DECODER_NO_FILE, MP3DECODER_BUFFER_OVERFLOW)
*/
static mp3decoder_result_t MP3GetDecodedFrameRec(short* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded, uint8_t depth);

/* FILE HANDLING FUNCTIONS */

/**
 * @brief Closes current file 
 */
static void closeFile(void);

/**
 * @brief Opens the given file
 * @param filename  File to be opened
 * @retval True if successfull
 */ 
static bool openFile(const char * filename);

/**
 * @brief Returns current file size
 */ 
static size_t currentFileSize();

/**
 * @brief Sets file cursor position to zero
 */ 
static void fileRewind();

/**
 * @brief Sets cursor position to pos
 * @param pos
 */ 
static void fileSeek(size_t pos);

/**
 * @brief Reads the requested amount of bytes from the file
 * @param buf Buffer 
 * @param count Amount of elements
 * @retval Amount of bytes read
 */ 
static size_t readFile(void * buf, size_t count);

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
  dec.hasID3Tag = false;
  #ifdef MP3_PC_TESTBENCH
  printf("Decoder initialized. Buffer size is %d bytes\n", MP3_FRAME_BUFFER_BYTES);
  #endif
}

bool MP3LoadFile(const char* filename)
{
  bool ret = false;

  if (dec.fileOpened)
  {
    // Close previous file and context if necessary
    closeFile();

    // Reset context pointers and vars 
    dec.fileOpened = false;
    dec.bottom = 0;
    dec.top = 0;
    dec.fileSize = 0;
    dec.bytesRemaining = 0;
    dec.hasID3Tag = false;
  }

  // Open new file, if successfully opened
  if (openFile(filename))
  {
    dec.fileOpened = true;
    dec.fileSize = currentFileSize();
    dec.bytesRemaining = dec.fileSize;

    // read ID3 tag and update pointers
    readID3Tag();
    
    // flush file to buffer
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
    return MP3GetDecodedFrameRec(outBuffer, bufferSize, samplesDecoded, 0);
}

mp3decoder_result_t MP3GetDecodedFrameRec(short* outBuffer, uint16_t bufferSize, uint16_t* samplesDecoded, uint8_t depth)
{
  mp3decoder_result_t ret = MP3DECODER_NO_ERROR;    // Return value of the function
  
  #ifdef MP3_PC_TESTBENCH
  printf("Entered decoding. File has %d bytes to decode\n", dec.fileSize);
  printf("Buffer has %d bytes to decode\n", dec.bottom - dec.top);
  #endif
  
  if (depth < MP3_REC_MAX_DEPTH)
  {
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
          if ((dec.top > 0) && ((dec.bottom - dec.top) > 0) && (dec.bottom - dec.top < MP3_FRAME_BUFFER_BYTES))
          {
              //memcpy(dec.mp3FrameBuffer , dec.mp3FrameBuffer + dec.top, dec.bottom - dec.top);
              memmove(dec.mp3FrameBuffer, dec.mp3FrameBuffer + dec.top, dec.bottom - dec.top);
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
          uint8_t* decPointer = dec.mp3FrameBuffer + dec.top;
          int bytesLeft = dec.bottom - dec.top;

          int res = MP3Decode(dec.helixDecoder, &decPointer, &(bytesLeft), outBuffer, MP3DECODER_MODE_NORMAL); //! autodecrements fileSize with bytes decoded. updated inbuf pointer, updated bytesLeft

          if (res == ERR_MP3_NONE) // if decoding successful
          {
              uint16_t decodedBytes = dec.bottom - dec.top - bytesLeft;
              dec.lastFrameLength = decodedBytes;

              #ifdef MP3_PC_TESTBENCH
              printf("Frame decoded!. MP3 frame size was %d bytes\n", decodedBytes);
              #endif

              // update header pointer and file size
              dec.top += decodedBytes;
              dec.bytesRemaining -= decodedBytes;

              // update last frame decoded info
              MP3GetLastFrameInfo(dec.helixDecoder, &(dec.lastFrameInfo));

              // update samples decoded
              *samplesDecoded = dec.lastFrameInfo.outputSamps;

		  	  uint16_t channelCount = dec.lastFrameInfo.nChans;
			  for (uint16_t i = 0 ; i < *samplesDecoded / channelCount ; i++)
			  {
				if (i > 0)
				{
			  	  static uint16_t maxDiff = 0;
				  int16_t aux1 = outBuffer[channelCount * i];
				  int16_t aux2 = outBuffer[channelCount * (i-1)];
				  uint16_t currDiff = abs(aux1 - aux2);
				  if (currDiff > maxDiff)
				  {
					maxDiff = currDiff;
				  }
				  if (currDiff >= 2000)
				  {
					currDiff++;
				  }
				 }
				}

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
              return MP3GetDecodedFrameRec(outBuffer, bufferSize, samplesDecoded, depth + 1); //! H-quearlo
          }
          else
          {
              if (dec.bytesRemaining <= dec.lastFrameLength)
              {
                  #ifdef MP3_PC_TESTBENCH
                  printf("Dropped frame\n");
                  #endif
                  return MP3DECODER_FILE_END;
              }
              else
              {
                  dec.top++;
                  dec.bytesRemaining--;
                  #ifdef MP3_PC_TESTBENCH
                  printf("Error: %d\n", res);
                  #endif

                  // If invalid header, try with next frame
                  return MP3GetDecodedFrameRec(outBuffer, bufferSize, samplesDecoded, depth + 1); //! H-quearlo
              }
          }
      }
      else
      {
          ret = MP3DECODER_FILE_END;
      }
  }
  else
  {
    ret = MP3DECODER_ERROR;
  }
  return ret;

}

bool MP3GetTagData(mp3decoder_tag_data_t* data)
{
    bool ret = false;
    if (dec.hasID3Tag)
    {
        strcpy(data->album, dec.ID3Data.album);
        strcpy(data->artist, dec.ID3Data.artist);
        strcpy(data->title, dec.ID3Data.title);
        strcpy(data->trackNum, dec.ID3Data.trackNum);
        strcpy(data->year, dec.ID3Data.year);     
        ret = true;
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
    if (MP3_FRAME_BUFFER_BYTES - dec.bottom > 0)
    {
        bytesRead = readFile(dest, (MP3_FRAME_BUFFER_BYTES - dec.bottom));
        // Update bottom pointer
        dec.bottom += bytesRead;
    }

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

void readID3Tag(void)
{

    if (has_ID3_tag(dec.mp3File))
    {
        dec.hasID3Tag = true;

        if (!read_ID3_info(TITLE_ID3, dec.ID3Data.title, ID3_MAX_FIELD_SIZE, dec.mp3File))
            strcpy(dec.ID3Data.title, DEFAULT_ID3_FIELD);

        if (!read_ID3_info(ALBUM_ID3, dec.ID3Data.album, ID3_MAX_FIELD_SIZE, dec.mp3File))
            strcpy(dec.ID3Data.album, DEFAULT_ID3_FIELD);

        if (!read_ID3_info(ARTIST_ID3, dec.ID3Data.artist, ID3_MAX_FIELD_SIZE, dec.mp3File))
            strcpy(dec.ID3Data.artist, DEFAULT_ID3_FIELD);

        if (!read_ID3_info(YEAR_ID3, dec.ID3Data.year, 10, dec.mp3File))
            strcpy(dec.ID3Data.year, DEFAULT_ID3_FIELD);

        if (!read_ID3_info(TRACK_NUM_ID3, dec.ID3Data.trackNum, 10, dec.mp3File))
            strcpy(dec.ID3Data.trackNum, DEFAULT_ID3_FIELD);


        unsigned int tagSize = get_ID3_size(dec.mp3File);

        #ifdef MP3_PC_TESTBENCH
        printf("ID3 Track found.\n");
        printf("ID3 Tag is %d bytes long\n", tagSize);
        #endif    

        fileSeek(tagSize);
        dec.bytesRemaining -= tagSize;

    }
    else
    {
        fileRewind();
    }
}

/* FILE HANDLING FUNCTIONS */

bool openFile(const char * filename)
{
    bool ret = false;
    #ifdef __arm__
    FRESULT fr = f_open(&dec.file, filename, FA_READ);
    if (fr == FR_OK)
    {
    	dec.mp3File = &(dec.file);
    	ret = true;
    }
    #else
    dec.mp3File = fopen(filename, "rb"); 
    ret = (dec.mp3File != NULL);
    #endif
    return ret;
}

void closeFile(void)
{
    #ifdef __arm__
    f_close(dec.mp3File);
    #else
    fclose(dec.mp3File);
    #endif
}

size_t currentFileSize()
{
  size_t result = 0;
  if (dec.fileOpened)
  {
    #ifdef __arm__
    result = f_size(dec.mp3File);
    #else
    fseek(dec.mp3File, 0L, SEEK_END);
    result = ftell(dec.mp3File);
    fileRewind();
    fseek (dec.mp3File, 0, SEEK_SET);
    #endif
  }
  return result;
}

void fileRewind()
{
    #ifdef __arm__
    f_rewind(dec.mp3File);
    #else
    rewind(dec.mp3File);
    #endif
}

void fileSeek(size_t pos)
{
    #ifdef __arm__
    f_lseek(dec.mp3File, pos);
    #else
    fseek(dec.mp3File, pos, SEEK_SET);
    #endif
}

size_t readFile(void * buf, size_t count)
{
    size_t ret = 0, read = 0;

    if (dec.fileOpened)
    {
      #ifdef __arm__
      FRESULT fr;
      fr = f_read(dec.mp3File, ((uint8_t *)buf), count, &read);
	  if (fr == FR_OK)
	  {
		ret = read;
	  }
	  else
	  {
		  ret = 0;
	  }
      #else
      ret = fread(buf, 1, count, dec.mp3File);
      #endif
    }

    return ret;
}


/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/


