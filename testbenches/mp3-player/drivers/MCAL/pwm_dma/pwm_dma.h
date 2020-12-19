/*******************************************************************************
  @file     pwm_dma.h
  @brief    FTM PWM driver with DMA controller
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef PWM_DMA_H_
#define PWM_DMA_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "../ftm/ftm.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// The PWMDMA update callback is used to update the next frame to be used in the
// DMA transfer.
// @param frameToUpdate   Pointer to the frame to be updated
// @param frameCounter    Current frame 
typedef void (*pwmdma_update_callback_t)(uint16_t * frameToUpdate, uint8_t frameCounter);

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initializes the FTM for PWM with DMA support
 * @param prescaler   Prescaler value for the FTM time base
 * @param mod         Modulo for PWM period ticks count
 */ 
void pwmdmaInit(uint8_t prescaler, uint16_t mod, ftm_instance_t ftmInstance, ftm_channel_t ftmChannel);

/**
 * @brief Registers callback for frame update request from the driver
 * @param callback    pwmdma_update_callback_t 
 */ 
void pwmdmaOnFrameUpdate(pwmdma_update_callback_t callback);

/**
 * @brief Returns whether the pwmdma driver is available to run a transfer.
 */
bool pwmdmaAvailable(void);

/**
 * @brief Starts the PWM 
 * @param firstFrame    Pointer to the first frame
 * @param secondFrame   Pointer to the second frame
 * @param frameSize     Size of the frame
 * @param totalFrames   Total amount of frames
 * @param loop          Whether the transfer should run in loop or not
 * @returns				Whether the transfer process started or not
 */ 
bool pwmdmaStart(uint16_t* firstFrame, uint16_t* secondFrame, size_t frameSize, size_t totalFrames, bool loop);


/*******************************************************************************
 ******************************************************************************/


#endif /* PWM_DMA_H_ */
