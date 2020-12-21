/*******************************************************************************
  @file     audio.h
  @brief    Audio UI and Controller Module
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef _AUDIO_H
#define _AUDIO_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "events/events.h"
#include "arm_math.h"

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

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
 * @brief Initializes the audio module.
 */
void audioInit(void);

/**
 * @brief Cycles the audio module with the next user event.
 * @param event     Next event
 */
void audioRun(event_t event);

/**
 * @brief Filename and path of current song, starts playing the audio.
 * @param path      Directory path for the audio files
 * @param file      Filename of the starting audio
 * @param index     Index of the filename in the directory
 */
void audioSetFolder(const char* path, const char* file, uint8_t index);

void setEqEnabled(bool eqEnabled);

/*******************************************************************************
 ******************************************************************************/

#endif
