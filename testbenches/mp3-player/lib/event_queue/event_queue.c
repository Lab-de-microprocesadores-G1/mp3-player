/*******************************************************************************
  @file     event_queue.c
  @brief    [...]
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "event_queue.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/


/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

event_queue_t createEventQueue(void* buffer, size_t queueSize, size_t elementSize)
{
	event_queue_t newQueue = {
			.queue = createQueue(buffer, queueSize, elementSize),
			.generatorsCount = 0
	};
	return newQueue;
}

generator_id_t registerEventGenerator(event_queue_t* queue, event_generator_t generator)
{
	generator_id_t id = OUT_OF_GENERATORS;

#ifdef EVENT_QUEUE_DEVELOPMENT_MODE
	if (queue)
#endif
	{

#ifdef EVENT_QUEUE_DEVELOPMENT_MODE
		if (queue->generatorsCount < MAX_EVENT_GENERATORS)
#endif
		{
			queue->eventGenerators[queue->generatorsCount] = generator;
			queue->enabledGenerators[queue->generatorsCount] = true;
			id = queue->generatorsCount++;
		}
	}

	// Return the succeed status
	return id;
}

bool setEnable(event_queue_t* queue, generator_id_t id, bool enable)
{
	bool succeed = false;

#ifdef EVENT_QUEUE_DEVELOPMENT_MODE
	if (queue && id < queue->generatorsCount)
#endif
	{
		queue->enabledGenerators[id] = enable;
		succeed = true;
	}

	// Return the succeed status
	return succeed;
}

void* getNextEvent(event_queue_t* queue)
{
	void* element = NO_EVENTS;

#ifdef EVENT_QUEUE_DEVELOPMENT_MODE
	if (queue)
#endif
	{
		// Look for all events from generators
		uint8_t	i;
		void* ev;
		for ( i = 0 ; i < queue->generatorsCount ; i++ )
		{
			// If the generator is enabled
			if (queue->enabledGenerators[i])
			{
				ev = queue->eventGenerators[i]();
				if (ev != NO_EVENTS)
				{
					push(&queue->queue, ev);
				}
			}
		}

		// Get a new event element, if has any
		if (!isEmpty(&queue->queue))
		{
			element = pop(&queue->queue);
		}
	}

	// Return the next event
	return element;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/


/******************************************************************************/
