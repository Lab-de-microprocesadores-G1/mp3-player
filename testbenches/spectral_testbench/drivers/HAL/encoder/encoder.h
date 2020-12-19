/***************************************************************************//**
  @file     encoder.h
  @brief    Rotative encoder driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef HAL_ENCODER_ENCODER_H_
#define HAL_ENCODER_ENCODER_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdbool.h>
#include "../../MCAL/gpio/gpio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum {
	ENCODER_0,
	ENCODER_1,
	ENCODER_COUNT
} encoder_id_t;

typedef void (*encoder_callback_t)(void);

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initialise encoders driver and corresponding peripherals.
 */
void encoderInit(void);

/**
 * @brief Register both callbacks for encoder.
 * @param id of the encoder to use.
 * @param clockwiseCallback Function to call when encoder rotates one position in clockwise direction.
 * @param counterClockwiseCallback Function to call when encoder rotates one position in counter-clockwise direction.
 */
void encoderRegisterCallbacks(encoder_id_t id, encoder_callback_t clockwiseCallback, encoder_callback_t counterClockwiseCallback);

/**
 * @brief Register callback for encoder rotating clockwise.
 * @param id of the encoder to use.
 * @param clockwiseCallback Function to call when encoder rotates one position in clockwise direction.
 */
void encoderRegisterClockwiseCallback(encoder_id_t id, encoder_callback_t clockwiseCallback);

/**
 * @brief Register callback for encoder rotating clockwise.
 * @param id of the encoder to use.
 * @param clockwiseCallback Function to call when encoder rotates one position in clockwise direction.
 */
void encoderRegisterCounterClockwiseCallback(encoder_id_t id, encoder_callback_t counterClockwiseCallback);

/**
 * @brief Disable encoder.
 * @param id of the encoder to use.
 */
void disableEncoder(encoder_id_t id);

/**
 * @brief Enable encoder.
 * @param id of the encoder to use.
 */
void enableEncoder(encoder_id_t id);

/*******************************************************************************
 ******************************************************************************/

#endif
