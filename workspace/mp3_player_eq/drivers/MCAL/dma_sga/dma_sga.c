/***************************************************************************//**
  @file     dma_sga.c
  @brief    Handler for DMA with Scatter and Gather operation
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <string.h>
#include "dma_sga.h"
#include "MK64F12.h"
#include "hardware.h"


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/


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

static dma_sga_callback_t majorLoopCallbacks[DMA_CHANNEL_COUNT];

static void DMA_IRQDispatcher(uint8_t channel);

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void dmasgaInit(void)
{
  // Clock Gating for eDMA and DMAMux
  SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;
  SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;
}

void dmasgaChannelConfig(dma_channel_t channel, dma_sga_channel_cfg_t config)
{
  // Enable NVIC for DMA
  uint8_t dmaIrqs[1][16] = DMA_CHN_IRQS;
  NVIC_EnableIRQ(dmaIrqs[0][channel]);

  // Enable DMAMUX for DMA_CHANNEL and select source
  DMAMUX->CHCFG[channel] = DMAMUX_CHCFG_ENBL(1) | DMAMUX_CHCFG_TRIG(config.pitEn) | DMAMUX_CHCFG_SOURCE(config.muxSource);

  // Enable the DMA channel for requests
  DMA0->ERQ |= (0x0001 << channel);

  // Config fixed-priority or round robin channel arbitration
  DMA0->CR = DMA_CR_ERCA(!config.fpArb);
  // If fixed-point arbitration, config priority
  if (config.fpArb)
  {
    __IO uint8_t * dchpri;
    switch (channel)
    {
      case 0:
        dchpri = &(DMA0->DCHPRI0);
        break;
      case 1:
        dchpri = &(DMA0->DCHPRI1);
        break;
      default:
        dchpri = &(DMA0->DCHPRI2);
        break;
    }
    // Enable preemption by higher priority channel, preempt ability for lower priority channel, set arbitration priority
    *dchpri = DMA_DCHPRI0_CHPRI(config.priority) | DMA_DCHPRI0_ECP(config.ecp) | DMA_DCHPRI0_DPA(config.dpa);
  }

  // Copy first software TCDn to actual DMA TCD 
  memcpy(&(DMA0->TCD[channel]), &(config.tcds[0]), sizeof(dma_tcd_t));
}

void dmasgaOnMajorLoop(dma_channel_t channel, dma_sga_callback_t callback)
{
  majorLoopCallbacks[channel] = callback;
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

__ISR__ DMA1_IRQHandler(void)
{
  DMA_IRQDispatcher(DMA_CHANNEL_1);
}

__ISR__ DMA0_IRQHandler(void)
{
  DMA_IRQDispatcher(DMA_CHANNEL_0);
}

void DMA_IRQDispatcher(uint8_t channel)
{
  uint16_t status = DMA0->INT;

  // Notify subscribers of corresponding event
  if (status & (DMA_INT_INT0_MASK << channel))
  {
	// Clear flag
	DMA0->INT = (DMA_INT_INT0_MASK << channel);
	if (majorLoopCallbacks[channel])
	{
	  majorLoopCallbacks[channel]();
	}
  }
}

/******************************************************************************/
