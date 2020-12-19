/***************************************************************************//**
  @file     dac_dma.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_DAC_DMA_DAC_DMA_H_
#define MCAL_DAC_DMA_DAC_DMA_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DAC_DMA_PPBUFFER_COUNT  2
#define DAC_DMA_PPBUFFER_SIZE   2048

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/


/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
*  dacdmaInit()
* @brief initializes dac-dma
*/
void dacdmaInit(void);

/*
*  dacdmaSetBuffers()
* @brief  sets ping pong buffers
* @param  buffer1, buffer2 pointers to buffers
* @param  bufferSize size of buffers
*/
void dacdmaSetBuffers(uint16_t *buffer1, uint16_t *buffer2, uint16_t bufferSize);

/*  
*  dacdmaSetFreq()
* @brief sets dac frequency
*/
void dacdmaSetFreq(uint16_t freq);

/*  
*  dacdmaStop()
* @brief stops DAC
*/
void dacdmaStop(void);

/*  
*  dacdmaStart()
* @brief starts DAC
*/
void dacdmaStart(void);

uint16_t* dacdmaGetBuffer(void);

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_DAC_DMA_DAC_DMA_H_ */
