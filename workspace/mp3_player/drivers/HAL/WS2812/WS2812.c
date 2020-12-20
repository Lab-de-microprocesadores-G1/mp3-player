/***************************************************************************//**
  @file     WS2812.c
  @brief    Smart led WS2812 controller
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "WS2812.h"

#include "drivers/MCAL/pwm_dma/pwm_dma.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define WS2812_LED_RGB_SIZE   3                                               // Size of led in rgb values
#define WS2812_FRAME_LED_SIZE 8                                               // Size of frame in leds
#define WS2812_LED_SIZE       (WS2812_LED_RGB_SIZE * 8)                       // Size of led in bits
#define WS2812_FRAME_SIZE     (WS2812_FRAME_LED_SIZE * WS2812_LED_SIZE)       // Size of frame in bits
#define WS2812_FTM_INSTANCE   FTM_INSTANCE_0
#define WS2812_FTM_CHANNEL    FTM_CHANNEL_0
#define WS2812_FTM_PRESCALE   0                                               // Configuration of the FTM driver
#define WS2812_FTM_MODULO     62                                              // for 800kbps rate needed in WS2812 leds
#define WS2812_HIGH_DUTY      42                                              // Duty value for '1' binit
#define WS2812_LOW_DUTY       20                                              // Duty value for '0' binit

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// Declaring the context of WS2812 data structure
typedef struct {
  /* Display buffer */
  ws2812_pixel_t*    buffer;
  size_t      bufferSize;

  /* Controller variables */
  uint16_t 	  firstFrame[WS2812_FRAME_SIZE];
  uint16_t    secondFrame[WS2812_FRAME_SIZE];
  
  /* Internal controller flags */
  bool alreadyInitialized;
} ws2812_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Updates the next frame to be transfered via DMA
 * @param frameToUpdate   Frame received to update
 * @param frameCounter    Current frame
 */
static void pwmUpdateCallback(uint16_t *frameToUpdate, uint8_t frameCounter);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static ws2812_context_t context;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void WS2812Init(void)
{
  if (!context.alreadyInitialized)
  {
    // Initialize PWM-DMA driver
    pwmdmaInit(WS2812_FTM_PRESCALE, WS2812_FTM_MODULO, WS2812_FTM_INSTANCE, WS2812_FTM_CHANNEL);
    
    // Register callback for buffer update
    pwmdmaOnFrameUpdate(pwmUpdateCallback);
    
    // Set initialization flag
    context.alreadyInitialized = true;
  }
}

void WS2812SetDisplayBuffer(ws2812_pixel_t* buffer, size_t size)
{
  context.buffer = buffer;
  context.bufferSize = size;
}

void WS2812Update(void)
{
  pwmdmaStart(context.firstFrame, context.secondFrame, WS2812_FRAME_SIZE, context.bufferSize / WS2812_FRAME_LED_SIZE, false);
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void pwmUpdateCallback(uint16_t *frameToUpdate, uint8_t frameCounter)
{
  uint8_t pixelComponents[3];

  for (uint8_t i = 0 ; i < WS2812_FRAME_LED_SIZE ; i++)
  {
    // Decoding current pixel in the frame to be updated
    ws2812_pixel_t pixel = context.buffer[ i + frameCounter * WS2812_FRAME_LED_SIZE ];

    // Using GRB order
    pixelComponents[0] = pixel.g;
    pixelComponents[1] = pixel.r;
    pixelComponents[2] = pixel.b;

    // Decoding binits to duty value required and updating the next frame
    for (uint8_t j = 0 ; j < WS2812_LED_RGB_SIZE ; j++)
    {
      for (uint8_t k = 0 ; k < 8 ; k++)
      {
    	uint16_t aux = (pixelComponents[j] & 0x80) ? WS2812_HIGH_DUTY : WS2812_LOW_DUTY;
        *(frameToUpdate + i * WS2812_LED_SIZE + j * 8 + k ) = aux;
        pixelComponents[j] <<= 1;
      }
    }
  }
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
