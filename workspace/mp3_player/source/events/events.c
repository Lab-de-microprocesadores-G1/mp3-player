/*******************************************************************************
  @file     events.c
  @brief    Event abstraction layer for the application
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "lib/event_queue/event_queue.h"
#include "lib/queue/queue.h"
#include "drivers/HAL/keypad/keypad.h"
#include "events.h"

#include <stdbool.h>

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

/*
 * @brief Callback to be called when the keypad raises an event.
 * @param event		Event being catched
 */
static void onKeyPadEvent(keypad_events_t event);

/*
 * @brief Event generator which provides access to the internal events of the hardware queue.
 */
static void* hardwareQueueEventGenerator(void);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static bool 			alreadyInit = false;							// Internal flag to detect initialization
static event_t 			hardwareQueueBuffer[QUEUE_STANDARD_MAX_SIZE];	// Hardware queue buffer for asynchronous events
static event_t 			eventQueueBuffer[QUEUE_STANDARD_MAX_SIZE];		// Event queue buffer used as memory buffer
static queue_t 			hardwareQueue;									// Hardware queue handler
static event_queue_t 	eventQueue;										// Event queue handler

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void eventsInit(void)
{
	if (!alreadyInit)
	{
		// Raise the internal flag, to avoid running the initialization of the
		// driver more than once. Skips the initialization routine;
		alreadyInit = true;

		// Initialization of the keypad driver
		keypadInit();
		keypadSubscribe(onKeyPadEvent);

		// Initialization of the hardware queue handler
		hardwareQueue = createQueue(&hardwareQueueBuffer, QUEUE_STANDARD_MAX_SIZE, sizeof(event_t));

		// Initialization of the event queue handler
		eventQueue = createEventQueue(&eventQueueBuffer, QUEUE_STANDARD_MAX_SIZE, sizeof(event_t));

		// Registers the event generators for the internal event queue
		registerEventGenerator(&eventQueue, hardwareQueueEventGenerator);
	}
}

event_t eventsGetNextEvent(void)
{
	event_t resultEvent = { .id = EVENTS_NONE };
	void* event = getNextEvent(&eventQueue);
	if (event)
	{
		resultEvent = *(event_t*)event;
	}
	return resultEvent;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void onKeyPadEvent(keypad_events_t event)
{
	event_t newEvent;

	// Mapping the event received from the keypad interface
	// into the available events of this abstraction layer
	if (event.source == KEYPAD_ENCODER_LEFT)
	{
		if (event.id == KEYPAD_PRESSED)
		{
			newEvent.id = EVENTS_ENTER;
		}
		else if (event.id == KEYPAD_DOUBLE_PRESSED)
		{
			newEvent.id = EVENTS_EXIT;
		}
		else if (event.id == KEYPAD_ROTATION_CLKW)
		{
			newEvent.id = EVENTS_LEFT;
		}
		else if (event.id == KEYPAD_ROTATION_COUNTER_CLKW)
		{
			newEvent.id = EVENTS_RIGHT;
		}
	}
	else if (event.source == KEYPAD_ENCODER_RIGHT)
	{
		if (event.id == KEYPAD_PRESSED)
		{
			newEvent.id = EVENTS_VOLUME_TOGGLE;
		}
		else if (event.id == KEYPAD_ROTATION_CLKW)
		{
			newEvent.id = EVENTS_VOLUME_DECREASE;
		}
		else if (event.id == KEYPAD_ROTATION_COUNTER_CLKW)
		{
			newEvent.id = EVENTS_VOLUME_INCREASE;
		}
	}
	else if (event.source == KEYPAD_BUTTON_PREVIOUS)
	{
		if (event.id == KEYPAD_PRESSED)
		{
			newEvent.id = EVENTS_PREVIOUS;
		}
	}
	else if (event.source == KEYPAD_BUTTON_PLAY_PAUSE)
	{
		if (event.id == KEYPAD_PRESSED)
		{
			newEvent.id = EVENTS_PLAY_PAUSE;
		}
	}
	else if (event.source == KEYPAD_BUTTON_NEXT)
	{
		if (event.id == KEYPAD_PRESSED)
		{
			newEvent.id = EVENTS_NEXT;
		}
	}

	// Push the new event into the hardware queue
	push(&hardwareQueue, (void*)(&newEvent));
}

static void* hardwareQueueEventGenerator(void)
{
	return pop(&hardwareQueue);
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
