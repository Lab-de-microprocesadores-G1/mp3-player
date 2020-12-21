/*******************************************************************************
  @file     dac_dma.c
  @brief    DAC DMA Driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdio.h>

#include "dac_dma.h"
#include "MK64F12.h"
#include "hardware.h"

#include "drivers/MCAL/dac/dac.h"
#include "drivers/MCAL/pit/pit.h"
#include "drivers/MCAL/dma_sga/dma_sga.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define DACDMA_DAC_ID       0
#define DACDMA_DMA_ID       0
#define DACDMA_DMA_CHANNEL  DMA_CHANNEL_0
#define DACDMA_PIT_CHANNEL  DACDMA_DMA_CHANNEL
#define DACDMA_TRIG_SOURCE  58                  // trigger always on

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum
{
  DACDMA_NOT_INITIALIZED,
  DACDMA_INITIALIZED,
  DACDMA_SETUP_READY
} dacdma_status_t;

typedef struct
{
  dacdma_status_t     		status;
  uint16_t*           		ppBufferPtr[DMA_SGA_PPBUFFER_COUNT];
  uint8_t			  		currentBuffer : 1;
  uint16_t            		bufferSize;
  uint16_t            		dacFreq;
  dacdma_update_callback_t  updateCallback;
  dma_sga_channel_cfg_t 	dmaConfig;
} dacdma_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onMajorLoop(void);

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
    dmasgaInit();
	  dmasgaOnMajorLoop(DACDMA_DMA_CHANNEL, onMajorLoop);

    // now is initialized
    dacdmaContext.status = DACDMA_INITIALIZED;
  }
}

void dacdmaSetFreq(uint16_t freq)
{
  pitSetInterval(DACDMA_PIT_CHANNEL, (uint16_t)PIT_HZ_TO_TICKS(freq));
  dacdmaContext.dacFreq = freq;
}

void dacdmaSetCallback(dacdma_update_callback_t callback)
{
  dacdmaContext.updateCallback = callback;
}

void dacdmaSetBuffers(uint16_t *buffer1, uint16_t *buffer2, uint16_t bufferSize)
{
  if ( (bufferSize != 0) && (buffer1 != NULL) && (buffer2 != NULL) )
  {
    dacdmaContext.ppBufferPtr[0] = buffer1;
    dacdmaContext.ppBufferPtr[1] = buffer2;
    dacdmaContext.bufferSize = bufferSize;

    // should call "dacdmaSetFreq()" before starting
    dacdmaContext.status = DACDMA_SETUP_READY;
  }
}

void dacdmaStart(void)
{
    if (dacdmaContext.status == DACDMA_SETUP_READY)
    {
        DAC_Type * dacPointers[] = DAC_BASE_PTRS;

        dacdmaContext.currentBuffer = 0;

        // Fill both buffers
        dacdmaContext.updateCallback(dacdmaContext.ppBufferPtr[0]);
        dacdmaContext.updateCallback(dacdmaContext.ppBufferPtr[1]);

        // Configure DMA Software TCD fields common to both TCDs
        // Destination address: DAC DAT
        dacdmaContext.dmaConfig.tcds[0].DADDR = (uint32_t)(&(dacPointers[DACDMA_DAC_ID]->DAT[0].DATL));

        // Source and destination offsets
        dacdmaContext.dmaConfig.tcds[0].SOFF = sizeof(uint16_t);
        dacdmaContext.dmaConfig.tcds[0].DOFF = 0;
        
        // Source last address adjustment
        dacdmaContext.dmaConfig.tcds[0].SLAST = -dacdmaContext.bufferSize * sizeof(uint16_t);
        
        // Set transfer size to 16bits (DAC DAT size is 12-bit)
        dacdmaContext.dmaConfig.tcds[0].ATTR = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1);
        // Write one sample on each trigger
        dacdmaContext.dmaConfig.tcds[0].NBYTES_MLNO = (0x01) * sizeof(uint16_t);
        
        // Enable Interrupt on major loop end and Scatter Gather Operation
        dacdmaContext.dmaConfig.tcds[0].CSR = DMA_CSR_INTMAJOR(1) | DMA_CSR_ESG(1);

        // Minor Loop Beginning Value
        dacdmaContext.dmaConfig.tcds[0].BITER_ELINKNO = dacdmaContext.bufferSize;
        // Minor Loop Current Value must be set to the beginning value the first time
        dacdmaContext.dmaConfig.tcds[0].CITER_ELINKNO = dacdmaContext.bufferSize;

        // Copy common content from TCD0 to TCD1
        dacdmaContext.dmaConfig.tcds[1] = dacdmaContext.dmaConfig.tcds[0];

        // Set source addresses for DMAs' TCD
        dacdmaContext.dmaConfig.tcds[0].SADDR = (uint32_t)(dacdmaContext.ppBufferPtr[0]);
        dacdmaContext.dmaConfig.tcds[1].SADDR = (uint32_t)(dacdmaContext.ppBufferPtr[1]);

        // Set Scatter Gather register of each TCD pointing to each other.
        dacdmaContext.dmaConfig.tcds[0].DLAST_SGA = (uint32_t) &(dacdmaContext.dmaConfig.tcds[1]);
        dacdmaContext.dmaConfig.tcds[1].DLAST_SGA = (uint32_t) &(dacdmaContext.dmaConfig.tcds[0]);

        // Enable period triggering, mux always enabled
        dacdmaContext.dmaConfig.pitEn = 1;
        dacdmaContext.dmaConfig.muxSource = DACDMA_TRIG_SOURCE;
        // Enable fixed-priority arbitration
        dacdmaContext.dmaConfig.fpArb = 0;
        dacdmaContext.dmaConfig.ecp = 1;
        dacdmaContext.dmaConfig.dpa = 0;
        dacdmaContext.dmaConfig.priority = 12;

        dmasgaChannelConfig(DACDMA_DMA_CHANNEL, dacdmaContext.dmaConfig);

        // PIT start
        pitStart(DACDMA_PIT_CHANNEL);
    }
}

void dacdmaStop(void)
{
    // stop PIT to avoid DMA requests triggering
    pitStop(DACDMA_PIT_CHANNEL);

}

void dacdmaResume(void)
{
  pitStart(DACDMA_PIT_CHANNEL);
}

  
uint16_t dacdmaGetFreq(void)
{
	return dacdmaContext.dacFreq;
}
/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void onMajorLoop(void)
{
	// Ping pong buffer switch
    dacdmaContext.currentBuffer = !dacdmaContext.currentBuffer;

    // Ask for frame update
	if (dacdmaContext.updateCallback)
	{
		dacdmaContext.updateCallback(dacdmaContext.ppBufferPtr[!dacdmaContext.currentBuffer]);
	}
}

/******************************************************************************/
