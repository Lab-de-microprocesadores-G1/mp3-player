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
#define DAC_FULL_SCALE          4096

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef void  (*dacdma_update_callback_t) (uint16_t * frameToUpdate);

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


/**
 * @brief Sets callback for frame updating
 * @param callback   Function to be called when the buffers must be updated
 */ 
void dacdmaSetCallback( dacdma_update_callback_t callback);

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

/*  
*  dacdmaGetFreq()
* @brief getter for DAC frequency
* @return DAC frequency
*/
uint16_t dacdmaGetFreq(void);

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_DAC_DMA_DAC_DMA_H_ */
