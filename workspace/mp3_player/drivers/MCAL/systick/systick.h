/*******************************************************************************
  @file     systick.h
  @brief    Systick simple timer driver
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef _SYSTICK_H_
#define _SYSTICK_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define SYSTICK_ISR_FREQUENCY_HZ   	1000U
#define CPU_FREQUENCY_HZ           	100000000UL
#define SYSTICK_TICK_MS			   	(1000U / SYSTICK_ISR_FREQUENCY_HZ)
#define SYSTICK_MS2TICKS(x)			((x) / SYSTICK_TICK_MS)

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
 * @brief Initialise SysTic driver
 * @param funcallback Function to be call every SysTick
 * @return Initialization and registration succeed
 */
bool systickInit (void (*funcallback)(void));


/*******************************************************************************
 ******************************************************************************/

#endif // _SYSTICK_H_
