/***************************************************************************//**
  @file     equaliser.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_CFFT_CFFT_H_
#define MCAL_CFFT_CFFT_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "arm_math.h"
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum 
{
  CFFT_16,
  CFFT_32,
  CFFT_64,
  CFFT_128,
  CFFT_256,
  CFFT_512,
  CFFT_1024,
  CFFT_2048,
  CFFT_4096
} cfft_size_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initialises cfft module. Sets the corresponding cfft instance to the size given.
 * @param size  Size of cfft to compute.
 */
void cfftInit(cfft_size_t size);

/**
 * @brief Compute the complex FFT on the data given.
 * @param inputF32      Buffer with input data.
 * @param outputF32     Buffer to store the output data.
 * @param doBitReverse  Determines whether to bit reverse output.
 */
void cfft(float32_t * inputF32, float32_t * outputF32, bool doBitReverse);

/**
 * @brief Compute the complex inverse FFT on the data given.
 * @param inputF32      Buffer with input data.
 * @param outputF32     Buffer to store the output data.
 * @param doBitReverse  Determines whether to bit reverse output.
 */
void icfft(float32_t * inputF32, float32_t * outputF32, bool doBitReverse);

/**
 * @brief Compute the complex FFT on the data given.
 * @param inputF32      Buffer with input data.
 * @param outputF32     Buffer to store the output data.
 */
void cfftGetMag(float32_t * inputF32, float32_t * outputF32);

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_CFFT_CFFT_H_ */
