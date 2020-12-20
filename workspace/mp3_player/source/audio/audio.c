/*******************************************************************************
  @file     audio.c
  @brief    Audio UI and Controller Module
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "audio.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// #include "drivers/MCAL/equaliser/equaliser.h"
#include "drivers/MCAL/cfft/cfft.h"

#include "lib/vumeter/vumeter.h"
#include "lib/fatfs/ff.h"
#include "display/display.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define FRAME_SIZE 					1024
#define FULL_SCALE 				  	300

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
  bool  alreadyInit;                // Whether it has been already initialized or not

  // Internal variables
  const char*     currentPath;      // Path name of the current direc
  const char*     currentFile;      // Filename of the current file being played
  uint32_t        currentIndex;     // Index of the current file in the directory
  audio_state_t   currentState;     // State of current audio
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
 * @brief Fills matrix with colValues
 */
static void fillMatrix(void);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static audio_context_t  context;
static pixel_t          displayMatrix[DISPLAY_SIZE][DISPLAY_SIZE];
static float            colValues[DISPLAY_SIZE];
static pixel_t          clearPixel = {0,0,0};

// Variables for fft
static float32_t filteredOutput[FRAME_SIZE];
static float32_t cfftInput[FRAME_SIZE*2];
static float32_t cfftOutput[FRAME_SIZE*2];
static float32_t cfftMagOutput[FRAME_SIZE];

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void audioInit(void)
{
  if (!context.alreadyInit)
  {
    // Raise the already initialized flag
    context.alreadyInit = true;
    context.currentState = AUDIO_STATE_IDLE;

    //fft initialisation
	  cfftInit(CFFT_1024);
  }
}

void audioRun(event_t event)
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

void audioSetFolder(const char* path, const char* file, uint8_t index)
{
  context.currentPath = path;
  context.currentFile = file;
  context.currentIndex = index;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void audioRunIdle(event_t event)
{
}

static void audioRunPlaying(event_t event)
{
}

static void audioRunPaused(event_t event)
{
}

static void fillMatrix(void)
{
  for(int i = 0; i < DISPLAY_SIZE; i++)
  {
    for(int j = 0; j < DISPLAY_SIZE; j++)
    {
      displayMatrix[i][j] = clearPixel;
    }
  }
  vumeterMultiple(displayMatrix, colValues, DISPLAY_SIZE, FULL_SCALE, BAR_MODE + LINEAR_MODE);
}

/*******************************************************************************
 *******************************************************************************
						INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
