/*******************************************************************************
  @file     pwm_dma.c
  @brief    PWM DMA
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <string.h>

#include "pwm_dma.h"
#include "MK64F12.h"
#include "hardware.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DMA_CHANNEL   0

#if !defined(FTM_DRIVER_LEGACY_MODE)
	#error	Please, turn the FlexTimer driver to the legacy mode.
#endif


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// DMA TCD Structure
typedef struct {
  uint32_t SADDR;                               /**< TCD Source Address, array offset: 0x1000, array step: 0x20 */
  uint16_t SOFF;                                /**< TCD Signed Source Address Offset, array offset: 0x1004, array step: 0x20 */
  uint16_t ATTR;                                /**< TCD Transfer Attributes, array offset: 0x1006, array step: 0x20 */
  union {                                       /* offset: 0x1008, array step: 0x20 */
    uint32_t NBYTES_MLNO;                       /**< TCD Minor Byte Count (Minor Loop Disabled), array offset: 0x1008, array step: 0x20 */
    uint32_t NBYTES_MLOFFNO;                    /**< TCD Signed Minor Loop Offset (Minor Loop Enabled and Offset Disabled), array offset: 0x1008, array step: 0x20 */
    uint32_t NBYTES_MLOFFYES;                   /**< TCD Signed Minor Loop Offset (Minor Loop and Offset Enabled), array offset: 0x1008, array step: 0x20 */
  };
  uint32_t SLAST;                               /**< TCD Last Source Address Adjustment, array offset: 0x100C, array step: 0x20 */
  uint32_t DADDR;                               /**< TCD Destination Address, array offset: 0x1010, array step: 0x20 */
  uint16_t DOFF;                                /**< TCD Signed Destination Address Offset, array offset: 0x1014, array step: 0x20 */
  union {                                       /* offset: 0x1016, array step: 0x20 */
    uint16_t CITER_ELINKNO;                     /**< TCD Current Minor Loop Link, Major Loop Count (Channel Linking Disabled), array offset: 0x1016, array step: 0x20 */
    uint16_t CITER_ELINKYES;                    /**< TCD Current Minor Loop Link, Major Loop Count (Channel Linking Enabled), array offset: 0x1016, array step: 0x20 */
  };
  uint32_t DLAST_SGA;                           /**< TCD Last Destination Address Adjustment/Scatter Gather Address, array offset: 0x1018, array step: 0x20 */
  uint16_t CSR;                                 /**< TCD Control and Status, array offset: 0x101C, array step: 0x20 */
  union {                                       /* offset: 0x101E, array step: 0x20 */
    uint16_t BITER_ELINKNO;                     /**< TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Disabled), array offset: 0x101E, array step: 0x20 */
    uint16_t BITER_ELINKYES;                    /**< TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Enabled), array offset: 0x101E, array step: 0x20 */
  };
} pwmdma_TCD_t;

// PWM-DMA Context data structura
typedef struct {
  /* Status and control fields of the context */
  bool                      alreadyInitialized;
  bool						isRunning;
  pwmdma_update_callback_t  updateCallback;

  /* Data Frames ping pong buffers and index */
  uint16_t*                 frames[2];
  uint8_t                   currentFrame : 1;

  /* Data frames information */
  size_t                    frameSize;
  size_t                    totalFrames;
  bool                      loop;  
  size_t                    framesCopied;

  /* FTM peripheral instance and channel */
  ftm_instance_t            ftmInstance;
  ftm_channel_t             ftmChannel;

  /* Software TCD structs for Scatter and Gather */
  pwmdma_TCD_t              tcds[2] __attribute__ ((aligned(32)));
  
} pwmdma_context_t;


/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint8_t pwmdmaFtm2DmaChannel(ftm_instance_t ftmInstance, ftm_channel_t ftmChannel);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static pwmdma_context_t context;

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void pwmdmaInit(uint8_t prescaler, uint16_t mod, ftm_instance_t ftmInstance, ftm_channel_t ftmChannel)
{
  if (!context.alreadyInitialized)
  {
    // Update the already initialized flag
    context.alreadyInitialized = true;
    context.isRunning = false;
    
    // Save context variables.
    context.ftmInstance = ftmInstance;
    context.ftmChannel = ftmChannel;

    // Initialize the FlexTimer module, the channel as PWM and enable the DMA to trigger the DMA request
    // when the transfer is required to update the next value of the CnV.
    // ¡Using the FlexTimer in legacy mode!
    ftmInit(ftmInstance, prescaler, 0xFFFF);
    ftmPwmInit(ftmInstance, ftmChannel, FTM_PWM_HIGH_PULSES, FTM_PWM_EDGE_ALIGNED, 1, mod);
    ftmChannelEnableDMA(ftmInstance, ftmChannel);

    // Clock Gating for eDMA and DMAMux
    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;
	SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;

    // Enable NVIC for DMA channel 0
    NVIC_EnableIRQ(DMA0_IRQn);
  
    // Enable DMAMUX for DMA_CHANNEL and select source
    DMAMUX->CHCFG[DMA_CHANNEL] = DMAMUX_CHCFG_ENBL(1) | DMAMUX_CHCFG_TRIG(0) | DMAMUX_CHCFG_SOURCE(pwmdmaFtm2DmaChannel(context.ftmInstance, context.ftmChannel));
  }
}

void pwmdmaOnFrameUpdate(pwmdma_update_callback_t callback)
{
  context.updateCallback = callback;
}

bool pwmdmaAvailable(void)
{
	return !context.isRunning;
}

bool pwmdmaStart(uint16_t* firstFrame, uint16_t* secondFrame, size_t frameSize, size_t totalFrames, bool loop)
{
	bool success = false;
	if (!context.isRunning)
	{
	  // Save the configuration of the transfers
	  context.frames[0] = firstFrame;
	  context.frames[1] = secondFrame;
	  context.frameSize = frameSize;
	  context.totalFrames = totalFrames;
	  context.loop = loop;
	  context.framesCopied = 0;
	  context.currentFrame = 0;
	  context.isRunning = true;

	  // Ask the user to update the content of the first two frames
	  // used for transfering data with DMA controller
	  context.updateCallback(firstFrame, 0);
	  context.updateCallback(secondFrame, 1);

	  // Configure DMA Software TCD fields common to both TCDs
	  // Destination address: FTM CnV for duty change
	  context.tcds[0].DADDR = (uint32_t)(ftmChannelCounter(context.ftmInstance, context.ftmChannel));

	  // Source and destination offsets
	  context.tcds[0].SOFF = sizeof(uint16_t);
	  context.tcds[0].DOFF = 0;

	  // Source last sddress adjustment
	  context.tcds[0].SLAST = -frameSize * sizeof(uint16_t);

	  // Set transfer size to 16bits (CnV size)
	  context.tcds[0].ATTR = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1);
	  context.tcds[0].NBYTES_MLNO = (0x01) * (0x02);

	  // Enable Interrupt on major loop end and Scatter Gather Operation
	  context.tcds[0].CSR = DMA_CSR_INTMAJOR(1) | DMA_CSR_ESG(1);

	  // Minor Loop Beginning Value
	  context.tcds[0].BITER_ELINKNO = frameSize;
	  // Minor Loop Current Value must be set to the beginning value the first time
	  context.tcds[0].CITER_ELINKNO = frameSize;

	  // Copy common content from TCD0 to TCD1
	  context.tcds[1] = context.tcds[0];

	  // Set source addresses for DMAs' TCD
	  context.tcds[0].SADDR = (uint32_t)(firstFrame);
	  context.tcds[1].SADDR = (uint32_t)(secondFrame);

	  // Set Scatter Gather register of each TCD pointing to each other.
	  context.tcds[0].DLAST_SGA = (uint32_t) &(context.tcds[1]);
	  context.tcds[1].DLAST_SGA = (uint32_t) &(context.tcds[0]);

	  // Enable the DMA channel for requests
	  DMA0->ERQ |= (0x0001 << DMA_CHANNEL);

	  // Copy first software TCDn to actual DMA TCD
	  memcpy(&(DMA0->TCD[DMA_CHANNEL]), &(context.tcds[0]), sizeof(pwmdma_TCD_t));

	  // Starts the ftm driver
	  ftmRestart(context.ftmInstance);
	  ftmPwmSetEnable(context.ftmInstance, context.ftmChannel, true);
	  success = true;
	}

	return success;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
__ISR__ DMA0_IRQHandler(void)
{
  uint16_t status = DMA0->INT;

  if (status & (DMA_INT_INT0_MASK << DMA_CHANNEL))
  {
    // Clear flag
    DMA0->INT = (DMA_INT_INT0_MASK << DMA_CHANNEL);

    /* Completed major loop */
    context.currentFrame = !context.currentFrame;   // Ping pong buffer switch  

    if (context.loop)
    {
      context.framesCopied = ( context.framesCopied + 1 ) % context.totalFrames;
      if (context.updateCallback)
      {
        context.updateCallback(context.frames[!context.currentFrame], context.framesCopied + 1); // Reload buffer
      }
    }
    else
    {
      context.framesCopied = context.framesCopied + 1;
      if (context.framesCopied < (context.totalFrames - 1))
      {
        if (context.updateCallback)
        {
          context.updateCallback(context.frames[!context.currentFrame], context.framesCopied + 1); // Reload buffer
        }
      }
      else if (context.framesCopied == (context.totalFrames - 1) )
      {
		// Disable Scatter and Gather operation to prevent one extra request
    	context.tcds[!context.currentFrame].CSR = ( context.tcds[!context.currentFrame].CSR & ~DMA_CSR_ESG_MASK ) | DMA_CSR_ESG(0);
        context.tcds[!context.currentFrame].DLAST_SGA = 0;
      }
      else if (context.framesCopied == context.totalFrames)
      {
        ftmPwmSetEnable(context.ftmInstance, context.ftmChannel, false);
        ftmStop(context.ftmInstance);
        context.isRunning = false;
      }
    } 
  }
}

uint8_t pwmdmaFtm2DmaChannel(ftm_instance_t ftmInstance, ftm_channel_t ftmChannel)
{
  uint8_t ret = 20;
  switch (ftmInstance)
  {
    case FTM_INSTANCE_0:  ret += ftmChannel; break;
    case FTM_INSTANCE_1:  ret += 8 + ftmChannel; break;
    case FTM_INSTANCE_2:  ret += 10 + ftmChannel; break;
    case FTM_INSTANCE_3:  ret += 12 + ftmChannel; break;
    default: break; 
  }
  
  return ret;
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/



/******************************************************************************/
