/*******************************************************************************
  @file     events.h
  @brief    Event abstraction layer for the application
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef EVENTS_EVENTS_H_
#define EVENTS_EVENTS_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
	EVENTS_NONE,			// No events where generated
	EVENTS_PREVIOUS,		// Previous button was pressed
	EVENTS_PLAY_PAUSE,		// Play and pause button was pressed
	EVENTS_NEXT,			// Next button was pressed
	EVENTS_LEFT,			// Rotative encoder was rotated to the left
	EVENTS_RIGHT,			// Rotative encoder was rotated to the right
	EVENTS_ENTER,			// Rotative encoder button was pressed
	EVENTS_EXIT,			// Double press detected on the rotative encoder button
	EVENTS_VOLUME_INCREASE,	// Volume encoder was rotated to the increase direction
	EVENTS_VOLUME_DECREASE,	// Volume encoder was rotated to the decrease direction
	EVENTS_VOLUME_TOGGLE,	// Volume encoder was pressed, toggle between mute/unmute

	EVENTS_COUNT
} event_id_t;

typedef struct {
	event_id_t	id;
} event_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
 * @brief Initialization of the events abstraction layer.
 */
void eventsInit(void);

/*
 * @brief Returns the next event.
 */
event_t eventsGetNextEvent(void);

/*******************************************************************************
 ******************************************************************************/


#endif /* EVENTS_EVENTS_H_ */
