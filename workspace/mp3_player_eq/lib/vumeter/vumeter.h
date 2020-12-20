/*******************************************************************************
  @file     vumeter.h
  @brief    Vumeter controller
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MATRIX_WRAPPER_H_
#define MATRIX_WRAPPER_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
	// Graphic Modes
	BAR_MODE	= 0b00000001,
	CENTRE_MODE	= 0b00000010,

	// Scale Modes
	LINEAR_MODE = 0b01000000,
	LOGARITHMIC_MODE = 0b10000000
} vumeter_modes_t;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} pixel_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * SERVICES
 ******************************************************************************/

/**
 * @brief Run the vumeter for each column of the given matrix, with the given values.
 * @param input			Input matrix
 * @param colValues		Array with the values
 * @param colQty		Amount of columns
 * @param fullScale		Full scale range
 * @param mode			Vumeter mode
 */
void vumeterMultiple(pixel_t* input, float* colValues, uint8_t colQty, double fullScale, vumeter_modes_t mode);

/**
 * @brief Run the vumeter for a single column.
 * @param col
 * @param value
 * @param colQty
 * @param fullScale
 * @param vumeterMode
 */
void vumeterSingle(pixel_t* col, float value, uint8_t colQty, double fullScale, vumeter_modes_t vumeterMode);

/*******************************************************************************
 * EVENT GENERATORS INTERFACE
 ******************************************************************************/

/*******************************************************************************
 ******************************************************************************/

#endif

