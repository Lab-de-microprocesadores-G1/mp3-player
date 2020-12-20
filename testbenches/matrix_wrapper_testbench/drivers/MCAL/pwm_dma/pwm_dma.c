/*******************************************************************************
  @file     pwm_dma.c
  @brief    PWM DMA
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <string.h>

#include "drivers/MCAL/dma_sga/dma_sga.h"
#include "pwm_dma.h"
#include "MK64F12.h"
#include "hardware.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DMA_CHANNEL   0

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// PWM-DMA Context data structura
typedef struct {

  /* Status and control fields of the context */
  bool                      alreadyInitialized;
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
  // dma_tcd_t	                tcds[2] __attribute__ ((aligned(32)));
  dma_sga_channel_cfg_t		dmaConfig;
  
} pwmdma_context_t;


/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint8_t pwmdmaFtm2DmaChannel(ftm_instance_t ftmInstance, ftm_channel_t ftmChannel);

static void onMajorLoop(void);

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
  FTM_Type * ftmInstances[] = FTM_BASE_PTRS;
  if (!context.alreadyInitialized)
  {
    // Update the already initialized flag
    context.alreadyInitialized = true;
    
    // Save context variables.
    context.ftmInstance = ftmInstance;
    context.ftmChannel = ftmChannel;

    // Initialize the ftm driver
    ftmInit(ftmInstance, prescaler, 0xFFFF);
    
    // Configure FTM for PWM
    ftmPwmInit(ftmInstance, ftmChannel, FTM_PWM_HIGH_PULSES, FTM_PWM_EDGE_ALIGNED, 1, mod);
    
    // Enable FTM to trigger DMA requests
	ftmInstances[context.ftmInstance]->CONTROLS[context.ftmChannel].CnSC |= FTM_CnSC_DMA(1) | FTM_CnSC_CHIE(1);
  
	// Legacy mode!!
	ftmInstances[context.ftmInstance]->MODE = (ftmInstances[context.ftmInstance]->MODE & ~FTM_MODE_FTMEN_MASK) | FTM_MODE_FTMEN(0);

	// Enable DMA operation with Scatter and Gather
	dmasgaInit();
	dmasgaOnMajorLoop(DMA_CHANNEL, onMajorLoop);

  }
}

void pwmdmaOnFrameUpdate(pwmdma_update_callback_t callback)
{
  context.updateCallback = callback;
}

void pwmdmaStart(uint16_t* firstFrame, uint16_t* secondFrame, size_t frameSize, size_t totalFrames, bool loop)
{
  FTM_Type * ftmInstances[] = FTM_BASE_PTRS;

  // Save the configuration of the transfers
  context.frames[0] = firstFrame;
  context.frames[1] = secondFrame;
  context.frameSize = frameSize;
  context.totalFrames = totalFrames;
  context.loop = loop;
  context.framesCopied = 0;
  context.currentFrame = 0;

  // Ask the user to update the content of the first two frames 
  // used for transfering data with DMA controller
  context.updateCallback(firstFrame, 0);
  context.updateCallback(secondFrame, 1);

  // Configure DMA Software TCD fields common to both TCDs
  // Destination address: FTM CnV for duty change
  context.dmaConfig.tcds[0].DADDR = (uint32_t)(&(ftmInstances[context.ftmInstance]->CONTROLS[context.ftmChannel].CnV));

  // Source and destination offsets
  context.dmaConfig.tcds[0].SOFF = sizeof(uint16_t);
  context.dmaConfig.tcds[0].DOFF = 0;
  
  // Source last sddress adjustment
  context.dmaConfig.tcds[0].SLAST = -frameSize * sizeof(uint16_t);
  
  // Set transfer size to 16bits (CnV size)
  context.dmaConfig.tcds[0].ATTR = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1);
  context.dmaConfig.tcds[0].NBYTES_MLNO = (0x01) * (0x02);
  
  // Enable Interrupt on major loop end and Scatter Gather Operation
  context.dmaConfig.tcds[0].CSR = DMA_CSR_INTMAJOR(1) | DMA_CSR_ESG(1);

  // Minor Loop Beginning Value
  context.dmaConfig.tcds[0].BITER_ELINKNO = frameSize;
  // Minor Loop Current Value must be set to the beginning value the first time
  context.dmaConfig.tcds[0].CITER_ELINKNO = frameSize;

  // Copy common content from TCD0 to TCD1
  context.dmaConfig.tcds[1] = context.dmaConfig.tcds[0];

  // Set source addresses for DMAs' TCD
  context.dmaConfig.tcds[0].SADDR = (uint32_t)(firstFrame);
  context.dmaConfig.tcds[1].SADDR = (uint32_t)(secondFrame);

  // Set Scatter Gather register of each TCD pointing to each other.
  context.dmaConfig.tcds[0].DLAST_SGA = (uint32_t) &(context.dmaConfig.tcds[1]);
  context.dmaConfig.tcds[1].DLAST_SGA = (uint32_t) &(context.dmaConfig.tcds[0]);

  // Disable period triggering, mux selects FTM
  context.dmaConfig.pitEn = 0;
  context.dmaConfig.muxSource = pwmdmaFtm2DmaChannel(context.ftmInstance, context.ftmChannel);
  // Disable fixed-priority arbitration
  context.dmaConfig.fpArb = 0;

  dmasgaChannelConfig(DMA_CHANNEL, context.dmaConfig);

  // Starts the ftm driver
  ftmStart(context.ftmInstance);
  ftmPwmSetEnable(context.ftmInstance, context.ftmChannel, true);
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void onMajorLoop(void)
{
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
    	context.dmaConfig.tcds[!context.currentFrame].CSR = ( context.dmaConfig.tcds[!context.currentFrame].CSR & ~DMA_CSR_ESG_MASK ) | DMA_CSR_ESG(0);
        context.dmaConfig.tcds[!context.currentFrame].DLAST_SGA = 0;
      }
      else if (context.framesCopied == context.totalFrames)
      {
         ftmPwmSetEnable(context.ftmInstance, context.ftmChannel, false);
    	 ftmStop(context.ftmInstance);
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
