/***************************************************************************//**
  @file     event_queue.h
  @brief    [...]
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef EVENT_QUEUE_H_
#define EVENT_QUEUE_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../queue/queue.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define NO_EVENTS						NULL
#define	MAX_EVENT_GENERATORS			15
#define	OUT_OF_GENERATORS				MAX_EVENT_GENERATORS
#define EVENT_QUEUE_STANDARD_MAX_SIZE	QUEUE_STANDARD_MAX_SIZE

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// Events are injected in the Event Queue by Event Generators
// registered in it, so every time the queue updates the events, calls
// all its generators.
// When registering a new event generator, the event queue will return its id.
typedef	void* (*event_generator_t)(void);

typedef	uint8_t	generator_id_t;

// The implementation of the Event Queue uses one element to distinguish
// between a full and an empty queue, so the queueSize should be always
// one more than the actual maximum size desired.
typedef struct event_queue{
	queue_t				queue;									 // Queue to save events
	event_generator_t	eventGenerators[MAX_EVENT_GENERATORS];	 // Registered event generators
	bool				enabledGenerators[MAX_EVENT_GENERATORS]; // Generators can be ignored
	uint8_t				generatorsCount;						 // Amount of registered event generators
} event_queue_t;


/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Creates an Event Queue instance from the buffer and size specified by user
 * @param buffer		Pointer to the array reserved in memory
 * @param queueSize		Amount of elements in the array (fixed)
 * @param elementSize	Size in bytes of the element
 */
event_queue_t createEventQueue(void* buffer, size_t queueSize, size_t elementSize);

/**
 * @brief Registers an event generator. Returns the generator id if
 * 		  succeed or OUT_OF_GENERATORS on error.
 * @param queue			Pointer to the Event Queue instance
 * @param generator 	Callback to the generator
 */
generator_id_t registerEventGenerator(event_queue_t* queue, event_generator_t generator);

/**
 * @brief Enables/Disables generator. Returns true if the generator existed,
 * 		  false if not.
 * @param queue			Pointer to the Event Queue instance
 * @param id			generator id
 * @param enable		enable/disable status
 */
bool setEnable(event_queue_t* queue, generator_id_t id, bool enable);

/**
 * @brief Returns next event from the queue, returns NO_EVENTS if there are
 * 		  no events.
 * @param queue			Pointer to the Event Queue instance
 */
void* getNextEvent(event_queue_t* queue);


/*******************************************************************************
 ******************************************************************************/

#endif
