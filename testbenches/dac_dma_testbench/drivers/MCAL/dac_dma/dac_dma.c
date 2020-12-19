/***************************************************************************//**
  @file     dac_dma.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "dac_dma.h"
#include "MK64F12.h"
#include "hardware.h"

#include "../dac/dac.h"
#include "../pit/pit.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define DACDMA_DAC_ID       0
#define DACDMA_DMA_ID       0
#define DACDMA_DMA_CHANNEL  1
#define DACDMA_PIT_CHANNEL  DACDMA_DMA_CHANNEL
#define DACDMA_TRIG_SOURCE  58                  // trigger always on

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
} dacdma_TCD_t;

typedef enum
{
  DACDMA_NOT_INITIALIZED,
  DACDMA_INITIALIZED,
  DACDMA_SETUP_READY
} dacdma_status_t;

typedef struct 
{
  uint16_t*       ppBuffer[DAC_DMA_PPBUFFER_COUNT];
  uint8_t         currentDMABuffer : 1;
  uint16_t        ppBufferSize;
  dacdma_TCD_t    tcds[DAC_DMA_PPBUFFER_COUNT] __attribute__ ((aligned(32)));
  dacdma_status_t status;
}dacdma_context_t;

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
static dacdma_context_t dacdmaContext;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void dacdmaInit(void)
{
  if (dacdmaContext.status == DACDMA_NOT_INITIALIZED)
  {
    // DAC init
    dac_cfg_t dacConfig;
    dacConfig.swTrigger = 0; // disable software trigger

    dacInit(DACDMA_DAC_ID, dacConfig);

    // PIT init
    pitInit(DACDMA_PIT_CHANNEL);

    // DMA init
    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;     // DMA clock gating
    SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;  

    // enable periodic triggering, with source always on
    DMAMUX_Type* dmaMuxPtrs = DMAMUX_BASE_PTRS;
    dmaMuxPtrs[DACDMA_DMA_ID].CHCFG[DACDMA_DMA_CHANNEL] |= DMAMUX_CHCFG_ENBL(1) | DMAMUX_CHCFG_TRIG(1) | DMAMUX_CHCFG_SOURCE(DACDMA_TRIG_SOURCE);

    NVIC_EnableIRQ(DMA0_IRQn);            // Enable NVIC for DMA channel 0

    // context init
    dacdmaContext.status = DACDMA_INITIALIZED;
  }

  
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
