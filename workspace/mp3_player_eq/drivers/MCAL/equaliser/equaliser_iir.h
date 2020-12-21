/***************************************************************************//**
  @file     equaliser.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_EQUALISER_IIR_H_
#define MCAL_EQUALISER_IIR_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "arm_math.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define EQ_NUM_OF_FILTERS			8

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initialises equaliser.
 * @param frameSize  Number of data numbers to be filtered when calling eqFilterFrame.
 */
void eqIirInit(uint32_t frameSize);

/**
 * @brief Compute the equaliser filter on the data given.
 * @param inputF32  Pointer to input data to filter.
 * @param outputF32 Pointer to where the filtered data should be saved.
 */
void eqIirFilterFrame(float32_t * inputF32, float32_t * outputF32);

/**
 * @brief Sets all equaliser filter gains.
 * @param gains  Array with the filter gains for each of the equaliser bands.
 */
void eqIirSetFilterGains(float32_t gains[EQ_NUM_OF_FILTERS]);

/**
 * @brief Sets equaliser number filterNum to the gain given.
 * @param gain        Filter gain for filter number filterNum.
 * @param filterNum   Number of filter to apply the gain to.
 */
void eqIirSetFilterGain(float32_t gain, uint8_t filterNum);

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_EQUALISER_IIR_H_ */
