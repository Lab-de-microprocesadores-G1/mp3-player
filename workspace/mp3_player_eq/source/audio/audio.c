/*******************************************************************************
  @file     audio.c
  @brief    Audio UI and Controller Module
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "board/board.h"
#include "audio.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "drivers/HAL/HD44780_LCD/HD44780_LCD.h"
#include "drivers/MCAL/equaliser/equaliser_iir.h"
#include "drivers/MCAL/dac_dma/dac_dma.h"
#include "drivers/MCAL/cfft/cfft.h"
#include "drivers/HAL/timer/timer.h"
#include "drivers/MCAL/gpio/gpio.h"

#include "lib/mp3decoder/mp3decoder.h"
#include "lib/vumeter/vumeter.h"
#include "lib/fatfs/ff.h"
#include "display/display.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define AUDIO_PROCESSING_RETRIES        (10)
#define AUDIO_STRING_BUFFER_SIZE        (256)
#define AUDIO_LCD_FPS_MS                (200)
#define AUDIO_LCD_ROTATION_TIME_MS  	  (350)
#define AUDIO_LCD_LINE_NUMBER       	  (0)
#define AUDIO_FRAME_SIZE 				        (1024)
#define AUDIO_FULL_SCALE 				        (300000)
#define AUDIO_DEFAULT_SAMPLE_RATE       (44100)
#define AUDIO_MAX_FILENAME_LEN          (AUDIO_BUFFER_SIZE)
#define AUDIO_BUFFER_COUNT              (2)
#define AUDIO_BUFFER_SIZE               (1024)
#define AUDIO_FLOAT_MAX                 (1)
#define MAX_VOLUME                      (30)
#define VOLUME_DURATION_MS              (2000)

// #define AUDIO_ENABLE_FFT
// #define AUDIO_ENABLE_EQ
#define AUDIO_DEBUG_MODE

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
  AUDIO_STATE_IDLE,     // No folder, path, filename or directory has been set
  AUDIO_STATE_PLAYING,  // Currently playing a song
  AUDIO_STATE_PAUSED,   // The current song has been paused

  AUDIO_STATE_COUNT
} audio_state_t;

typedef struct {
  // Flags
  bool                      alreadyInit;      // Whether it has been already initialized or not
  bool                      messageChanged;   // When the message to be displayed in the LCD has changed
  const char*               message;          // Current message to be displayed in the LCD

  // Internal variables
  char                      filePath[AUDIO_MAX_FILENAME_LEN]; // File path 
  const char*               currentPath;                      // Path name of the current directory
  const char*               currentFile;                      // Filename of the current file being played
  uint32_t                  currentIndex;                     // Index of the current file in the directory
  audio_state_t             currentState;                     // State of current audio

  // Audio output buffer
  uint16_t                  audioBuffer[AUDIO_BUFFER_COUNT][AUDIO_BUFFER_SIZE];

  // Display data
  struct {
    pixel_t                 displayMatrix[DISPLAY_COL_SIZE][DISPLAY_COL_SIZE];
    float                   colValues[DISPLAY_COL_SIZE];
  } display;
  
  // MP3 data
  struct {
    mp3decoder_tag_data_t     tagData;
    mp3decoder_frame_data_t   frameData;              
    uint32_t                  sampleRate;        
    int16_t                   buffer[MP3_DECODED_BUFFER_SIZE + 2 * AUDIO_BUFFER_SIZE];  
    uint16_t                  samples;       
  } mp3;      
  
 struct {
   float32_t filteredOutput[AUDIO_FRAME_SIZE];
   float32_t input[AUDIO_FRAME_SIZE * 2];
   float32_t output[AUDIO_FRAME_SIZE * 2];
   float32_t magOutput[AUDIO_FRAME_SIZE];
 } fft;

 struct {
   uint16_t input[AUDIO_BUFFER_SIZE];
   uint16_t output[AUDIO_BUFFER_SIZE];
 } eq;

  // Volume and message buffers
  uint8_t volume;
  char    volumeBuffer[AUDIO_STRING_BUFFER_SIZE];
  char    messageBuffer[AUDIO_STRING_BUFFER_SIZE];
} audio_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Cycles the audio module on the idle state.
 * @param event   Next event to be run
 */
static void audioRunIdle(event_t event);

/**
 * @brief Cycles the audio module on the playing state.
 * @param event   Next event to be run
 */
static void audioRunPlaying(event_t event);

/**
 * @brief Cycles the audio module on the paused state.
 * @param event   Next event to be run
 */
static void audioRunPaused(event_t event);

/**
 * @brief Cycles the audio module volume controller
 * @param event   Next event to be run
 */
static void audioRunVolumeController(event_t event);

/**
 * @brief Sets the new state of the audio module.
 * @param state   Next state
 */
static void audioSetState(audio_state_t state);

/**
 * @brief Audio processing routine.
 * @param frame   Pointer to the frame to be updated
 */
static void audioProcess(uint16_t* frame);

/**
 * @brief Audio set the current string.
 * @param message New message
 */
static void audioSetDisplayString(const char* message);

/**
 * @brief Callback to be called when updating the LCD.
 */
static void audioLcdUpdate(void);

/**
 * @brief Fills matrix with colValues
 */
static void audioFillMatrix(void);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static audio_context_t  context;
static const pixel_t    clearPixel = {0,0,0};

static const float32_t eqCoeffsTestFloat[6*3] = 
{ 
  1.01072457568661*0.000807981680029028,	0.0, -2.00000000000000*0.000807981680029028,	0.989725411916175,	1.96917484672735,	-0.970121794265049,
  2,  0.0,	1.16358783614474,	0.760849536121614,	-0.0936004258199819,	-0.344079170483367,
  2,  0.0,	-1.52537222956547,	-0.423053519820128,	0.945160260816669,	0.0228829936642259
};
static q15_t eqCoeffsTest[6*3];
static arm_biquad_casd_df1_inst_q15 filterTest;
static q15_t filterStateTest[4*3];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void audioInit(void)
{
  if (!context.alreadyInit)
  {
    // arm_float_to_q15(eqCoeffsTestFloat, eqCoeffsTest, 6*3);
    // arm_biquad_cascade_df1_init_q15(&filterTest, 3, eqCoeffsTest, filterStateTest, 1);
    // eqIirInit();

    // Raise the already initialized flag
    context.alreadyInit = true;
    context.currentState = AUDIO_STATE_IDLE;
    context.volume = MAX_VOLUME / 2;
    
    arm_float_to_q15(eqCoeffsTestFloat, eqCoeffsTest, 6*3);
    arm_biquad_cascade_df1_init_q15(&filterTest, 3, eqCoeffsTest, filterStateTest, 1);
    
    // Initialization of the timer
    timerStart(timerGetId(), TIMER_MS2TICKS(AUDIO_LCD_FPS_MS), TIM_MODE_PERIODIC, audioLcdUpdate);

    // FFT initialization
	  cfftInit(CFFT_1024);

    // Equaliser initialisation
    eqInit(AUDIO_FRAME_SIZE);
    
    // MP3 Decoder init
    MP3DecoderInit();

    // DAC DMA init
    dacdmaInit();
    dacdmaSetBuffers(context.audioBuffer[0], context.audioBuffer[1], AUDIO_BUFFER_SIZE);
    dacdmaSetFreq(AUDIO_DEFAULT_SAMPLE_RATE);

#ifdef AUDIO_DEBUG_MODE
    gpioMode(PIN_PROCESSING, OUTPUT);
#endif
  }
}

void audioRun(event_t event)
{
  // Run the volume controller for the audio module
  if (event.id == EVENTS_VOLUME_DECREASE || event.id == EVENTS_VOLUME_INCREASE || event.id == EVENTS_VOLUME_TOGGLE)
  {
    audioRunVolumeController(event);
  }
  // Run the audio module controller, when not modifying the volume system
  else
  {
    switch (context.currentState)
    {
      case AUDIO_STATE_IDLE:
        audioRunIdle(event);
        break;
      
      case AUDIO_STATE_PLAYING:
        audioRunPlaying(event);
        break;
      
      case AUDIO_STATE_PAUSED:
        audioRunPaused(event);
        break;

      default:
        break;
    }
  }
}

void audioSetFolder(const char* path, const char* file, uint8_t index)
{
  // Should check the current audio state to stop the audio
  // if it is playing! Should clear something else?

  // Load MP3 File
  sprintf(context.filePath, "%s/%s", path, file);
  if (MP3LoadFile(context.filePath))
  {
    context.currentPath = path;
    context.currentFile = file;
    context.currentIndex = index;
    context.currentState = AUDIO_STATE_PLAYING;
    context.mp3.samples = 0;

    // Read ID3 tag if present
    if (!MP3GetTagData(&(context.mp3.tagData)))
    {
      // If not, title will be filename 
      strcpy(context.mp3.tagData.title, file);
    }

    // Get sample rate 
    if (MP3GetNextFrameData(&context.mp3.frameData))
    {
      context.mp3.sampleRate = context.mp3.frameData.sampleRate; 
      dacdmaSetFreq(context.mp3.sampleRate);
    }

    // Start sound reproduction
    dacdmaStart();
  }
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void audioRunIdle(event_t event)
{
  switch (event.id)
  {

    default:
      break;
  }
}

static void audioRunPlaying(event_t event)
{
  switch (event.id)
  {
    case EVENTS_PLAY_PAUSE:
      if (context.currentState == AUDIO_STATE_PLAYING)
      {
        context.currentState = AUDIO_STATE_PAUSED;
        dacdmaStop();
        context.messageBuffer[0] = HD4478_CUSTOM_PAUSE;
        sprintf(context.messageBuffer + 1, " - %s - %s - %s", context.mp3.tagData.title, context.mp3.tagData.artist, context.mp3.tagData.album);
      }
      else if (context.currentState == AUDIO_STATE_PAUSED)
      {
        context.currentState = AUDIO_STATE_PLAYING;
        dacdmaResume();
        context.messageBuffer[0] = HD4478_CUSTOM_PLAY;
        sprintf(context.messageBuffer + 1, " - %s - %s - %s", context.mp3.tagData.title, context.mp3.tagData.artist, context.mp3.tagData.album);
      }
      audioSetDisplayString(context.messageBuffer);
      break;

    case EVENTS_PREVIOUS:
      //TODO! 
      break;

    case EVENTS_NEXT:
      //TODO! 
      break;

    case EVENTS_FRAME_FINISHED:
      audioProcess(event.data.frame);
      break;

    default:
      break;
  }
}

static void audioRunPaused(event_t event)
{
  audioRunPlaying(event);
}

static void audioRunVolumeController(event_t event)
{
  switch (event.id)
  {
    case EVENTS_VOLUME_INCREASE:
      if (context.volume < MAX_VOLUME)
      {
        context.volume++;
        sprintf(context.volumeBuffer, "Volumen %d", context.volume);
      }
      if (context.volume == MAX_VOLUME)
      {
        sprintf(context.volumeBuffer, "Volumen MAX");
      }
      audioSetDisplayString(context.volumeBuffer);
      break;
      
    case EVENTS_VOLUME_DECREASE:
      if (context.volume)
      {
        context.volume--;
      }
      sprintf(context.volumeBuffer, "Volumen %d", context.volume);
      audioSetDisplayString(context.volumeBuffer);
      break;
    
    default:
      break;
  }
}

static void audioSetState(audio_state_t state)
{
  context.currentState = state;
}

static void	audioLcdUpdate(void)
{
  if (HD44780LcdInitReady())
  {
    if (context.messageChanged)
    {
      context.messageChanged = false;
      if (context.currentState == AUDIO_STATE_PAUSED)
      {
        HD44780WriteNewLine(AUDIO_LCD_LINE_NUMBER, (uint8_t*)context.message, strlen(context.message));
      }
      else
      {
        HD44780WriteRotatingString(AUDIO_LCD_LINE_NUMBER, (uint8_t*)context.message, strlen(context.message), AUDIO_LCD_ROTATION_TIME_MS);
      }
    }
  }
}

static void audioSetDisplayString(const char* message)
{
  context.message = message;
  context.messageChanged = true;
}

static void audioFillMatrix(void)
{
  for(int i = 0; i < DISPLAY_COL_SIZE; i++)
  {
    for(int j = 0; j < DISPLAY_COL_SIZE; j++)
    {
      context.display.displayMatrix[i][j] = clearPixel;
    }
  }
  vumeterMultiple((ws2812_pixel_t*)context.display.displayMatrix, context.display.colValues, DISPLAY_COL_SIZE, AUDIO_FULL_SCALE, BAR_MODE + LINEAR_MODE);
  displayFlip((ws2812_pixel_t*)context.display.displayMatrix);
}

void audioProcess(uint16_t* frame)
{
  uint16_t attempts = AUDIO_PROCESSING_RETRIES;
  uint16_t sampleCount;
  uint16_t channelCount = 1;
  mp3decoder_result_t mp3Res = MP3DECODER_NO_ERROR;
  mp3decoder_frame_data_t frameData;

#ifdef AUDIO_DEBUG_MODE
    gpioWrite(PIN_PROCESSING, HIGH);
#endif
  
  // Get number of channels in next mp3 frame
  if (MP3GetNextFrameData(&frameData))
  {
    channelCount = frameData.channelCount;
  } 
  
  // Check if decoding samples is necessary
  while ((context.mp3.samples < channelCount * AUDIO_BUFFER_SIZE) && attempts)
  {
    // Decode next frame (STEREO output)
    mp3Res = MP3GetDecodedFrame(context.mp3.buffer + context.mp3.samples, MP3_DECODED_BUFFER_SIZE, &sampleCount);
    
    if (mp3Res == MP3DECODER_NO_ERROR)
    {
      // Update sample count
      context.mp3.samples += sampleCount;
    }
    else if (mp3Res == MP3DECODER_FILE_END)
    {
      // If file ended and not enough samples, fill with zeros to process last samples
      if (context.mp3.samples < AUDIO_BUFFER_SIZE * channelCount)
      {
        memset(context.mp3.buffer, 0, AUDIO_BUFFER_SIZE * channelCount - context.mp3.samples);
        context.mp3.samples = AUDIO_BUFFER_SIZE*channelCount;
      }

      // Stop song and trigger next file open
    }
    else
    {
      // Verify the amount of retries
      attempts--;
    }
  }

  // Data conditioning for next stage
  #ifdef AUDIO_ENABLE_EQ
  for (uint16_t i = 0; i < AUDIO_BUFFER_SIZE; i++)
  {
    context.eq.input[i] = (uint16_t)context.decodedMP3Buffer[channelCount * i];
    context.eq.output[i] = 0;
    // context.eq.output[i] = (float32_t)context.mp3.buffer[channelCount * i];
  }  
  // Equalising
  // eqFilterFrame(context.eq.input, context.eq.output);
  // arm_biquad_cascade_df1_q15(&filterTest, context.eq.input, context.eq.output, 1024);
  eqIirFilterFrame(context.eq.input, context.eq.output);
  #endif

  #ifdef AUDIO_ENABLE_FFT
  // Computing FFT
  for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; i++)
	{
		context.fft.input[i*2] = (float32_t)context.eq.output[i];
		context.fft.input[i*2+1] = 0;
		context.fft.output[i*2] = 0;
		context.fft.output[i*2+1] = 0;
    context.fft.magOutput[i] = 0;
	}

  cfft(context.fft.input, context.fft.output, true);
	cfftGetMag(context.fft.output, context.fft.magOutput);

	for (uint32_t i = 0; i < DISPLAY_COL_SIZE; i++)
	{
		arm_mean_f32(context.fft.magOutput + AUDIO_BUFFER_SIZE / 2 + i * AUDIO_BUFFER_SIZE / DISPLAY_COL_SIZE / 2, AUDIO_BUFFER_SIZE / DISPLAY_COL_SIZE / 2, context.display.colValues + i);
	}

  audioFillMatrix();
  #endif

  // Write samples to output buffer
  for (uint16_t i = 0 ; i < AUDIO_BUFFER_SIZE ; i++)
  {
    // DAC output is unsigned, mono and 12 bit long

    uint16_t aux = (uint16_t)((context.eq.output[i]) * context.volume / 16 + (DAC_FULL_SCALE / 2));
    // frame[i] = aux;
    frame[i] = (uint16_t)(context.mp3.buffer[channelCount * i] / 16 + (DAC_FULL_SCALE / 2));
  }

  // Update MP3 decoding buffer
  context.mp3.samples -= AUDIO_BUFFER_SIZE * channelCount;
  memmove(context.mp3.buffer, context.mp3.buffer + AUDIO_BUFFER_SIZE * channelCount, context.mp3.samples * sizeof(int16_t));

#ifdef AUDIO_DEBUG_MODE
  gpioWrite(PIN_PROCESSING, LOW);
#endif
}


/******************************************************************************/
