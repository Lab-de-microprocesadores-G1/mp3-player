/********************************************************************************
  @file     App.c
  @brief    Application functions
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 *******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/MCAL/dma_sga/dma_sga.h"
#include "drivers/MCAL/dac/dac.h"
#include "drivers/MCAL/pit/pit.h"
#include "MK64F12.h"
#include <stdbool.h>
#include <math.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define FRAME_SIZE	1024
#define SAMPLE_RATE	44100
#define DMA_CHANNEL	1
#define PIT_CHANNEL	DMA_CHANNEL
#define DAC_CHANNEL 0

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static void onMajorLoop(void);
static void configDma(void);


/*******************************************************************************
 * VARIABLES TYPES DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * PRIVATE VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static uint16_t buffers[2][FRAME_SIZE];
static dma_sga_channel_cfg_t dmaConfig;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/* Called once at the beginning of the program */
void appInit (void)
{
	// PIT init
	pitInit(PIT_CHANNEL);
	pitSetInterval(PIT_CHANNEL, PIT_HZ_TO_TICKS(SAMPLE_RATE));

	// Config DMA peripheral
	dmasgaInit();

	// Fill buffer
	for (uint32_t i = 0 ; i < FRAME_SIZE ; i++)
	{
		buffers[0][i] = i;
	}

	// Start transfers
	dmasgaOnMajorLoop(DMA_CHANNEL, onMajorLoop);
	configDma();

    dac_cfg_t dacConfig;
    dacConfig.swTrigger = 0; // disable software trigger
	dacInit(DAC_CHANNEL, dacConfig);

	pitStart(PIT_CHANNEL);
}

/* Called repeatedly in an infinite loop */
void appRun (void)
{

}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void onMajorLoop(void)
{
	static int huevo = 0;
	huevo++;
}

void configDma(void)
{
    DAC_Type * dacPointers[] = DAC_BASE_PTRS;

    // Configure DMA Software TCD fields common to both TCDs
    // Destination address: buffer[1]
    dmaConfig.tcds[0].DADDR = (uint32_t)(&(dacPointers[0]->DAT[0].DATL));

    // Source and destination offsets
    dmaConfig.tcds[0].SOFF = sizeof(uint16_t);
    dmaConfig.tcds[0].DOFF = 0;

    // Source last sddress adjustment
    dmaConfig.tcds[0].SLAST = - FRAME_SIZE * sizeof(uint16_t);

    // Set transfer size to 16bits (DAC DAT size is 12-bit)
    dmaConfig.tcds[0].ATTR = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1);
    // Write all samples on each trigger
    dmaConfig.tcds[0].NBYTES_MLNO = FRAME_SIZE * sizeof(uint16_t);

    // Enable Interrupt on major loop end and Scatter Gather Operation
    dmaConfig.tcds[0].CSR = DMA_CSR_INTMAJOR(1) | DMA_CSR_ESG(1);

    // Minor Loop Beginning Value
    dmaConfig.tcds[0].BITER_ELINKNO = 1;
    // Minor Loop Current Value must be set to the beginning value the first time
    dmaConfig.tcds[0].CITER_ELINKNO = 1;

    // Copy common content from TCD0 to TCD1
    dmaConfig.tcds[1] = dmaConfig.tcds[0];

    // Set source addresses for DMAs' TCD
    dmaConfig.tcds[0].SADDR = (uint32_t)(buffers[0]);
    dmaConfig.tcds[1].SADDR = (uint32_t)(buffers[1]);

    // Set Scatter Gather register of each TCD pointing to each other.
    dmaConfig.tcds[0].DLAST_SGA = (uint32_t) &(dmaConfig.tcds[1]);
    dmaConfig.tcds[1].DLAST_SGA = (uint32_t) &(dmaConfig.tcds[0]);

    // Enable period triggering, mux always enabled
    dmaConfig.pitEn = 1;
    dmaConfig.muxSource = 58;
    // Disable fixed-priority arbitration
    dmaConfig.fpArb = 0;

    dmasgaChannelConfig(DMA_CHANNEL, dmaConfig);
}

/*******************************************************************************
 ******************************************************************************/

