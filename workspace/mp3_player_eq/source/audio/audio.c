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

#define AUDIO_PROCESSING_RETRIES        		(10)
#define AUDIO_STRING_BUFFER_SIZE        		(128)
#define AUDIO_LCD_FPS_MS                		(200)
#define AUDIO_LCD_ROTATION_TIME_MS  	  		(350)
#define AUDIO_LCD_LINE_NUMBER       	  		(0)
#define AUDIO_FRAME_SIZE 				            (4096)
#define AUDIO_FULL_SCALE 				            (50000)
#define AUDIO_DEFAULT_SAMPLE_RATE       		(44100)
#define AUDIO_MAX_FILENAME_LEN          		(128)
#define AUDIO_BUFFER_COUNT              		(2)
#define AUDIO_BUFFER_SIZE               		(4096)
#define AUDIO_FLOAT_MAX                 		(1)
#define AUDIO_MAX_VOLUME                    (100)
#define AUDIO_VOLUME_DURATION_MS            (2000)

// #define AUDIO_ENABLE_FFT
#define AUDIO_ENABLE_EQ
#define AUDIO_DEBUG_MODE

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
  AUDIO_STATE_IDLE,     // No folder, path, filename or directory has been set
  AUDIO_STATE_PLAYING,  // Currently playing a song
  AUDIO_STATE_PAUSED,   // The current song has been paused
  AUDIO_STATE_FINISHED,	// Finished playing a song

  AUDIO_STATE_COUNT
} audio_state_t;

typedef struct {
  // Flags
  bool                      alreadyInit;      // Whether it has been already initialized or not
  bool                      messageChanged;   // When the message to be displayed in the LCD has changed
  const char*               message;          // Current message to be displayed in the LCD

  // Internal variables
  char                      filePath[AUDIO_MAX_FILENAME_LEN]; 		// File path
  char               		currentPath[AUDIO_MAX_FILENAME_LEN];    // Path name of the current directory
  char               		currentFile[AUDIO_MAX_FILENAME_LEN];    // Filename of the current file being played
  uint32_t                  currentIndex;                     		// Index of the current file in the directory
  audio_state_t             currentState;                     		// State of current audio

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
   float32_t input[AUDIO_FRAME_SIZE * 2];
   float32_t output[AUDIO_FRAME_SIZE * 2];
 } fft;

 struct {
   uint16_t input[AUDIO_BUFFER_SIZE];
   uint16_t output[AUDIO_BUFFER_SIZE];
 } eq;

  // Volume and message buffers
  uint8_t volume;
  bool    mute;
  char    volumeBuffer[AUDIO_STRING_BUFFER_SIZE];
  char    messageBuffer[AUDIO_STRING_BUFFER_SIZE];
  tim_id_t  volumeTimer;

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
 * @brief Cycles the audio module on the next state.
 * @param event   Next event to be run
 */
static void audioRunFinished(event_t event);

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

/**
 * @brief Play an audio file
 * @param file    Filename of the audio
 * @param index   Index of the file in the directory
 */
static bool audioPlay(const char* file, uint8_t index);

/**
 * @brief Play the next audio file in the directory.
 */
static bool audioPlayNext(void);

/**
 * @brief Play the previous audio file in the directory.
 */
static bool audioPlayPrevious(void);

/**
 * @brief Shows current song tag or title
 */ 
static void showFileTag(void);

/**
 * @brief Shows player info after volume timer timeout
 */ 
void  onVolumeTimeout(void);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
 
// Mapping the FFT bin to the led matrix columns, according to the equaliser band-pass frequency.                   
//                                        80Hz    150Hz   330Hz   680Hz     1,2kHz    3,9kHz    12kHz     18kHz
static const uint32_t FFT_COLUMN_BIN[] = { 2,      3,      8,      16,       28,       91,       279,      418};

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static audio_context_t  context;
static const pixel_t    clearPixel = {0,0,0};

static const float32_t eqCoeffsTestFloat[8*6*3] = 
{ 
  1.00835600624052*0.00216553414810450*327.317081433528/2,  0.0/2,	2*0.00216553414810450*327.317081433528/2,	0.991916794681873*0.00216553414810450*327.317081433528/2,	-1.94951079265195/2,	-0.952093538476318/2,
  1.78583296534210/2, 0.0/2,	-2/2,	0.896408323748891/2,  -0.233486396398810/2,	-0.360402792417911/2,
  2/2,  0.0/2,	1.07595105249082/2,	-0.876465363661421/2,	-1.05840645602835/2,	-0.105214135828325/2,

  1.06594164349188*0.0470832718010200*21.1820803351109/2, 0.0/2,	2*0.0470832718010200*21.1820803351109/2,	0.948091954408744*0.0470832718010200*21.1820803351109/2,	-1.72905848227802/2,	-0.785095389248171/2,
  1.09035534332368/2, 0.0/2,	-2/2,	0.934214685697580/2,	1.63460342408081/2,	-0.728242807825876/2,
  2/2,  0.0/2,	0.0669872295660320/2,	-1.51622458787765/2,	-0.0633190825957426/2,	0.542715597041235/2,
  
  1.02433646264940*0.00387752894614502*202.642196761567/2,  0.0/2, 	-2*0.00387752894614502*202.642196761567/2, 	0.977866239744286*0.00387752894614502*202.642196761567/2,	1.93059706941576/2,	-0.935143860692841/2,
  1.49285480535221/2, 0.0/2, 	2/2, 	0.870065179634465/2,	-1.03407531436612/2,	-0.481207570977051/2,
  2/2,  0.0/2, 	-0.809985310456335/2, 	-1.04657658458247/2,	0.534814648688213/2,	0.370476473603739/2,

  1.01072457568661*0.000807981680029028*591.205111949767/2, 0.0/2, 	-2.00000000000000*0.000807981680029028*591.205111949767/2, 	0.989725411916175*0.000807981680029028*591.205111949767/2,	1.96917484672735/2,	0.970121794265049/2,
  2/2,  0.0/2, 	1.16358783614474/2, 	0.760849536121614/2,	-0.0936004258199819/2,	-0.344079170483367/2,
  2/2,  0.0/2, 	-1.52537222956547/2, 	-0.423053519820128/2,	0.945160260816669/2,	0.0228829936642259/2,

  1.00272386528311*0.000239342330207526*995.772261520071/2, 0.0/2, 	-2*0.000239342330207526*995.772261520071/2, 	0.997305769626192*0.000239342330207526*995.772261520071/2,  1.98365742954945/2,	-0.983942761571608/2,
  2/2,  0.0/2, 	-0.390651211555079/2, 	0.680383010224290/2,  1.37652177558776/2,	-0.605576750784261/2,
  1.86884745346874/2, 0.0/2, 	-2/2, 	0.140575826278916/2,	1.56497537811220/2,	-0.572348978505266/2,

  1.00192198758327*2.72338740331284e-05*7235.17994671590/2, 0.0/2,  -2*2.72338740331284e-05*7235.17994671590/2,	0.998092975368662*2.72338740331284e-05*7235.17994671590/2,  1.99445083720081/2,	-0.994482725253394/2,
  1.48608339064881/2, 0.0/2,	-2/2,	0.869830173270140/2,	1.56442746089939/2,	-0.688811817355399/2,
  1.28612855314640/2, 0.0/2,	-2/2,	0.716084086218801/2,	1.67323452092318/2,	-0.675085653207281/2,

  1.00087164556994*5.63376622144417e-06*46249.2848504937/2, 0.0/2, 	-2*5.63376622144417e-06*46249.2848504937/2, 	0.999131445036195*5.63376622144417e-06*46249.2848504937/2,  1.99748212282793/2,	-0.997488719311369/2,
  1.16513571174038/2, 0.0/2, 	-2/2, 	0.903530810325367/2,	1.81231871956445/2,	-0.841171346261079/2,
  1.12970820602613/2, 0.0/2, 	-2/2, 	0.870748616732947/2,	1.83748886733712/2,	-0.837908570704190/2,

  1.00047610364104*1.70860238912297e-06*167201.812858106/2, 0.0/2,	-2*1.70860238912297e-06*167201.812858106/2,	0.999524819919675*1.70860238912297e-06*167201.812858106/2,	1.99862444736334/2,	-0.998626419705476/2,
  1.08079073011060/2, 0.0/2,	-2.00000000000000/2,	0.939484732537213/2,	1.90046718405089/2,	-0.909458348044327/2,
  1.07084298161993/2, 0.0/2,	-2/2,	0.929293519106444/2,	1.90787535117889/2,	-0.908005564364528/2
};

static q15_t eqCoeffsTest[8*6*3];
static arm_biquad_casd_df1_inst_q15 filterTest;
static q15_t filterStateTest[8*4*3];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void audioInit(void)
{
  if (!context.alreadyInit)
  {
#ifdef AUDIO_ENABLE_EQ

    // eqInit(AUDIO_FRAME_SIZE);
    // arm_float_to_q15(eqCoeffsTestFloat, eqCoeffsTest, 8*6*3);
    // arm_biquad_cascade_df1_init_q15(&filterTest, 8*3, eqCoeffsTest, filterStateTest, 1);
    eqIirInit();
#endif

    // Raise the already initialized flag
    context.alreadyInit = true;
    context.currentState = AUDIO_STATE_IDLE;
    context.volume = AUDIO_MAX_VOLUME / 2;
    context.mute = false;

    // Request timer for volume control
    context.volumeTimer = timerGetId();
    
    // Initialization of the timer
    timerStart(timerGetId(), TIMER_MS2TICKS(AUDIO_LCD_FPS_MS), TIM_MODE_PERIODIC, audioLcdUpdate);

    // FFT initialization
    cfftInit(CFFT_1024);
    
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

      case AUDIO_STATE_FINISHED:
        audioRunFinished(event);
        break;

      default:
        break;
    }
  }
}

void audioSetFolder(const char* path, const char* file, uint8_t index)
{
  strcpy(context.currentPath, path);
  audioSetState(AUDIO_STATE_PLAYING);
  audioPlay(file, index);
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static bool audioPlay(const char* file, uint8_t index)
{
  bool success = false;

  // Save the current file and file index in the context
  strcpy(context.currentFile, file);
  context.currentIndex = index;

  // Load MP3 File
  sprintf(context.filePath, "%s/%s", context.currentPath, file);
  if (MP3LoadFile(context.filePath))
  {
	// Variable initialization
    context.mp3.samples = 0;

    // Read ID3 tag if present
    if (!MP3GetTagData(&(context.mp3.tagData)) || !strlen((char*) context.mp3.tagData.title))
    {
      // If not, title will be filename 
      strcpy((char*) context.mp3.tagData.title, file);
    }

    // Get sample rate 
    if (MP3GetNextFrameData(&context.mp3.frameData))
    {
      context.mp3.sampleRate = context.mp3.frameData.sampleRate; 
      dacdmaSetFreq(context.mp3.sampleRate);
    }

    // Start sound reproduction
    showFileTag();
    dacdmaStart();
    success = true;
  }
  
  return success;
}

static bool audioPlayNext(void)
{
  bool success = false;
  FILINFO file;
  FRESULT fr;
  DIR dir;
  if (context.currentPath)
  {
    fr = f_opendir(&dir, context.currentPath);
    if (fr == FR_OK) 
    {
      for (uint32_t i = 0 ; (i <= context.currentIndex) && (fr == FR_OK) ; i++)
      {  
        fr = f_readdir(&dir, &file);
      }
      if (fr == FR_OK)
      {
        if (file.fname[0])
        {
          if (strcmp(file.fname, context.currentFile))
          {
            dacdmaStop();
            success = audioPlay(file.fname, context.currentIndex + 1);
          }
        }
      }
    }
    f_closedir(&dir);
  }
  return success;
}

static bool audioPlayPrevious(void)
{
  bool success = false;
  FRESULT fr;
  FILINFO file;
  DIR dir;
  if (context.currentPath)
  {
	  fr = f_opendir(&dir, context.currentPath);
    if (fr == FR_OK) 
    {
      fr = f_rewinddir(&dir);
      for (uint32_t i = 0 ; (i < context.currentIndex) && (fr == FR_OK) ; i++)
      {  
        fr = f_readdir(&dir, &file);
      }
      if (fr == FR_OK)
      {
        if (context.currentIndex)
        {
          if (file.fname[0])
          {
            if (strcmp(file.fname, context.currentFile))
            {
              dacdmaStop();
              success = audioPlay(file.fname, context.currentIndex - 1);
            }
          }
        }
      }
    }
    f_closedir(&dir);
  }

  return success;
}

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
      audioSetState(AUDIO_STATE_PAUSED);
      dacdmaStop();
      showFileTag();
      break;

    case EVENTS_PREVIOUS:
      audioPlayPrevious();
      break;

    case EVENTS_NEXT:
      audioPlayNext();
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
  switch (event.id)
  {
    case EVENTS_PLAY_PAUSE:
      audioSetState(AUDIO_STATE_PLAYING);
      dacdmaResume();
      showFileTag();
      break;

    case EVENTS_PREVIOUS:
      audioPlayPrevious();
      break;

    case EVENTS_NEXT:
      audioPlayNext();
      break;

    default:
      break;
  }
}

static void audioRunFinished(event_t event)
{
  switch (event.id)
  {
    case EVENTS_PLAY_PAUSE:
      audioPlay(context.currentFile, context.currentIndex);
      break;

    case EVENTS_PREVIOUS:
      audioPlayPrevious();
      break;

    case EVENTS_NEXT:
      audioPlayNext();
      break;

    default:
      break;
  }
}

static void audioRunVolumeController(event_t event)
{
  switch (event.id)
  {
    case EVENTS_VOLUME_INCREASE:
      if (context.volume < AUDIO_MAX_VOLUME)
      {
        context.volume++;
        sprintf(context.volumeBuffer, "Volumen %d", context.volume);
      }
      if (context.volume == AUDIO_MAX_VOLUME)
      {
        sprintf(context.volumeBuffer, "Volumen MAX");
      }
      break;
      
    case EVENTS_VOLUME_DECREASE:
      if (context.volume)
      {
        context.volume--;
      }
      sprintf(context.volumeBuffer, "Volumen %d", context.volume);
      break;
    
    case EVENTS_VOLUME_TOGGLE:
      context.mute = !context.mute;
      if (context.mute)
      {
        sprintf(context.volumeBuffer, "Mute");
      }
      else
      {
        sprintf(context.volumeBuffer, "Unmute"); 
      }
      break;

    default:
      break;


  }

  // Show volume status on display
  audioSetDisplayString(context.volumeBuffer);

  // Start (or restart) volume timer
  timerStart(context.volumeTimer, TIMER_MS2TICKS(AUDIO_VOLUME_DURATION_MS), TIM_MODE_SINGLESHOT, onVolumeTimeout);

}

void  onVolumeTimeout(void)
{
  // Return LCD control to player
  audioSetDisplayString(context.messageBuffer);
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
  vumeterMultiple((pixel_t*)context.display.displayMatrix, context.display.colValues, DISPLAY_COL_SIZE, AUDIO_FULL_SCALE, BAR_MODE + LINEAR_MODE);
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

  while ((context.mp3.samples < channelCount * AUDIO_BUFFER_SIZE) && attempts && (mp3Res == MP3DECODER_NO_ERROR))
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
      // Raise file end flag
      audioSetState(AUDIO_STATE_FINISHED);
      dacdmaStop();
    }
    else
    {
      // Verify the amount of retries
      attempts--;
    }
  }

#ifdef AUDIO_DEBUG_MODE
  gpioWrite(PIN_PROCESSING, LOW);
#endif

  // Data conditioning for next stage
  #ifdef AUDIO_ENABLE_EQ
  for (uint16_t i = 0; i < AUDIO_BUFFER_SIZE; i++)
  {
    context.eq.input[i] = (uint16_t)context.mp3.buffer[channelCount * i];
    context.eq.output[i] = 0;
  }  
  // Equalising
  // eqFilterFrame(context.eq.input, context.eq.output);
  // arm_biquad_cascade_df1_q15(&filterTest, context.eq.input, context.eq.output, 4096);
  eqIirFilterFrame(context.eq.input, context.eq.output);
  #endif

  #ifdef AUDIO_ENABLE_FFT
  // Computing FFT
  for (uint32_t i = 0; i < AUDIO_BUFFER_SIZE; i++)
  {
		context.fft.input[i*2] = (float32_t)context.mp3.buffer[i];
		context.fft.input[i*2+1] = 0;
		context.fft.output[i*2] = 0;
		context.fft.output[i*2+1] = 0;
	}

  cfft(context.fft.input, context.fft.output, true);
  cfftGetMag(context.fft.output, context.fft.input);
  for (uint32_t i = 0 ; i < DISPLAY_COL_SIZE ; i++)
  {
    context.display.colValues[i] = context.fft.input[AUDIO_BUFFER_SIZE / 2 + FFT_COLUMN_BIN[i]];
  }

  audioFillMatrix();
  #endif

  double volume = (context.mute ? 0 : context.volume) / (double)AUDIO_MAX_VOLUME;
  // Write samples to output buffer
  for (uint16_t i = 0 ; i < AUDIO_BUFFER_SIZE ; i++)
  {
    // DAC output is unsigned, mono and 12 bit long
#ifdef AUDIO_ENABLE_EQ
    uint16_t aux = (uint16_t)((context.eq.output[i] / 16.0 + 0.5) * volume + (DAC_FULL_SCALE / 2));
    frame[i] = aux;
#else
    frame[i] = (int16_t)(context.mp3.buffer[channelCount * i] / 16.0 + 0.5) * volume + (DAC_FULL_SCALE / 2);
#endif
  }

  // Update MP3 decoding buffer
  context.mp3.samples -= AUDIO_BUFFER_SIZE * channelCount;
  memmove(context.mp3.buffer, context.mp3.buffer + AUDIO_BUFFER_SIZE * channelCount, context.mp3.samples * sizeof(int16_t));
}

void showFileTag(void)
{
  context.messageBuffer[0] = (context.currentState == AUDIO_STATE_PLAYING) ? HD4478_CUSTOM_PLAY : HD4478_CUSTOM_PAUSE;
  context.messageBuffer[1] = '\0';
  if ((context.mp3.tagData.title) && strlen((const char*)context.mp3.tagData.title))
  {
    sprintf(context.messageBuffer + strlen(context.messageBuffer), " - %s", context.mp3.tagData.title);
  }
  if ((context.mp3.tagData.artist) && strlen((const char*)context.mp3.tagData.artist))
  {
    sprintf(context.messageBuffer + strlen(context.messageBuffer), " - %s", context.mp3.tagData.artist);
  }
  if ((context.mp3.tagData.album) && strlen((const char*)context.mp3.tagData.album))
  {
    sprintf(context.messageBuffer + strlen(context.messageBuffer), " - %s", context.mp3.tagData.album);
  }
  audioSetDisplayString(context.messageBuffer);
}

/******************************************************************************/
