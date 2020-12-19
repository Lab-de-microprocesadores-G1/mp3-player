/***************************************************************************//**
  @file     dma.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "dma.h"
#include "MK64F12.h"
#include "hardware.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

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
} dma_TCD_t;

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

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void dmaInit(void)
{

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
__ISR__ DMA0_IRQHandler(void)
{
  uint16_t status = DMA0->INT;

  if (status & (DMA_INT_INT0_MASK << DACDMA_DMA_CHANNEL))
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
      }
    } 
  }
}

/******************************************************************************/
