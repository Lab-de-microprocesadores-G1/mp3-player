/***************************************************************************//**
  @file     pit.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_PIT_PIT_H_
#define MCAL_PIT_PIT_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define PIT_CLOCK_HZ (50e6) 

#define PIT_HZ_TO_TICKS(x) ((uint16_t)( PIT_CLOCK_HZ / (double) (x) ) )
#define PIT_uS_TO_TICKS(x) ((uint16_t)( PIT_CLOCK_HZ * (double) (x) / (double) 1e6 ))

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum
{
  PIT_CHANNEL_0,
  PIT_CHANNEL_1,
  PIT_CHANNEL_2,
  PIT_CHANNEL_3,
  PIT_CHANNEL_COUNT
}pit_channel_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/
/*
* pitInit()
* @brief  PIT channel initialization
* @param  channel PIT channel to initialize
*
*/
void pitInit(pit_channel_t channel);

/*
* pitSetInterval()
* @brief  sets PIT channel counter (period)
* @param  channel   PIT channel to initialize
* @param  ticks     value to load in counter (in ticks)   
*
*/
void pitSetInterval(pit_channel_t channel, uint32_t ticks);

/*
* pitStart()
* @brief  PIT channel start
* @param  channel PIT channel to start
*
*/
void pitStart(pit_channel_t channel);

/*
* pitStop()
* @brief  PIT channel stop
* @param  channel PIT channel to stop
*
*/
void pitStop(pit_channel_t channel);



/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_PIT_PIT_H_ */
