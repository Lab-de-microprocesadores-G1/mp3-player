/***************************************************************************//**
  @file     dac.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_DAC_DAC_H_
#define MCAL_DAC_DAC_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum {
	DAC_0,
	DAC_COUNT
} dac_id_t;

typedef struct {
	uint8_t swTrigger : 1;	// 1 to use software trigger
} dac_cfg_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
 * @brief Initializes the DAC peripheral
 * @param id 		DAC to be used
 * @param config	Peripheral configurations
 */
void dacInit(dac_id_t id, dac_cfg_t config);

/*
 * @brief Writes a value to the output.
 * @param id		DAC to be used
 * @param value		number 0-4095 will determine Vout
 */
void dacWrite(dac_id_t id, uint16_t value);

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_DAC_DAC_H_ */
