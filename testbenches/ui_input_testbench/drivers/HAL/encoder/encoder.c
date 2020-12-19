/*******************************************************************************
  @file     encoder.c
  @brief    Rotative encoder driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "encoder.h"
#include "../../../board/board.h"
#include "../../MCAL/gpio/gpio.h"
#include "MK64F12.h"
#include <stdio.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define ENCODER0_A 	PIN_LEFT_ENCA
#define ENCODER0_B 	PIN_LEFT_ENCB

#define ENCODER1_A	PIN_RIGHT_ENCA
#define ENCODER1_B	PIN_RIGHT_ENCB

//* To add a new encoder, set it's pins in board.h like this:
//* ENCODER1_A_PIN PORTNUM2PIN(port,pinNumber)
//* ENCODER1_B_PIN PORTNUM2PIN(port,pinNumber)

//* And then here like:
//* #define ENCODER1_A ENCODER1_A_PIN
//* #define ENCODER1_B ENCODER1_B_PIN

#ifdef ISR_DEVELOPMENT_MODE
#define ENCODER_ISR_DEV ENCODER_ISR_DEV_PIN
#endif

#define ENCODER_ISR_HANDLER(i,s) \
      void encoderISRHandler_ ## i ## _ ## s(void) { \
          FSMCycle(i,s); \
      }
#define ENCODER_ISR_HANDLER_NAME(i,s) encoderISRHandler_ ## i ## _ ## s
#define ENCODER_ISR_HANDLER_PROTOTYPE(i,s) static void encoderISRHandler_ ## i ## _ ## s(void);


/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef void (*encoder_act_routine_t)(encoder_id_t);

typedef enum uint8_t {
  A_POS_EDGE,
  A_NEG_EDGE,
  B_POS_EDGE,
  B_NEG_EDGE,
  DEFAULT_EV
} encoder_fsm_event_t;

typedef struct state_diagram_edge encoder_state_t;
struct state_diagram_edge {
  encoder_fsm_event_t event;
  encoder_state_t* nextState;
  encoder_act_routine_t actionRoutine;
};

typedef void (*encoder_isr_t)(void);

typedef struct {
  pin_t pinNumberA;
  pin_t pinNumberB;
  encoder_state_t* currentState;
  bool enabled;
  encoder_callback_t clockwiseCallback;
  encoder_callback_t counterClockwiseCallback;
  encoder_isr_t isrA;
  encoder_isr_t isrB;
} encoder_t;

typedef enum {
  A,
  B
} encoder_signal_t;



/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
static void FSMCycle(encoder_id_t id, encoder_signal_t signal);
static encoder_fsm_event_t getEvent(encoder_id_t id, encoder_signal_t signal);
static void initSingleEncoder(encoder_id_t id);
ENCODER_ISR_HANDLER_PROTOTYPE(0, A)
ENCODER_ISR_HANDLER_PROTOTYPE(0, B)
ENCODER_ISR_HANDLER_PROTOTYPE(1, A)
ENCODER_ISR_HANDLER_PROTOTYPE(1, B)

//* To add a new encoder:
//* ENCODER_ISR_HANDLER_PROTOTYPE(1,A)
//* ENCODER_ISR_HANDLER_PROTOTYPE(1,B)

// Action routines for FSM.
static void noActRoutine(encoder_id_t id);
static void rotateClockwise(encoder_id_t id);
static void rotateCounterClockwise(encoder_id_t id);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
extern encoder_state_t IDLE[];
extern encoder_state_t RC1[];
extern encoder_state_t RC2[];
extern encoder_state_t RC3[];
extern encoder_state_t RCC1[];
extern encoder_state_t RCC2[];
extern encoder_state_t RCC3[];


static encoder_t encodersList[ENCODER_COUNT] = {
  {ENCODER0_A, ENCODER0_B, IDLE, false, NULL, NULL, ENCODER_ISR_HANDLER_NAME(0,A), ENCODER_ISR_HANDLER_NAME(0,B)},
  {ENCODER1_A, ENCODER1_B, IDLE, false, NULL, NULL, ENCODER_ISR_HANDLER_NAME(1,A), ENCODER_ISR_HANDLER_NAME(1,B)}
  //* To add a new encoder:
  //* {ENCODER1_A, ENCODER1_B, IDLE, false, NULL, NULL, ENCODER_ISR_HANDLER_NAME(1,A), ENCODER_ISR_HANDLER_NAME(1,B)}
};

// Defining FSM.
encoder_state_t IDLE[] = {
  {B_NEG_EDGE, RC1, noActRoutine},
  {A_NEG_EDGE, RCC1, noActRoutine},
  {DEFAULT_EV, IDLE, noActRoutine}
};
encoder_state_t RC1[] = {
  {B_POS_EDGE, IDLE, noActRoutine},
  {A_NEG_EDGE, RC2, noActRoutine},
  {DEFAULT_EV, RC1, noActRoutine}
};
encoder_state_t RC2[] = {
  {A_POS_EDGE, RC1, noActRoutine},
  {B_POS_EDGE, RC3, noActRoutine},
  {DEFAULT_EV, RC2, noActRoutine}
};
encoder_state_t RC3[] = {
  {B_NEG_EDGE, RC2, noActRoutine},
  {A_POS_EDGE, IDLE, rotateClockwise},
  {DEFAULT_EV, RC3, noActRoutine}
};
encoder_state_t RCC1[] = {
  {A_POS_EDGE, IDLE, noActRoutine},
  {B_NEG_EDGE, RCC2, noActRoutine},
  {DEFAULT_EV, RCC1, noActRoutine}
};
encoder_state_t RCC2[] = {
  {B_POS_EDGE, RCC1, noActRoutine},
  {A_POS_EDGE, RCC3, noActRoutine},
  {DEFAULT_EV, RCC2, noActRoutine}
};
encoder_state_t RCC3[] = {
  {A_NEG_EDGE, RCC2, noActRoutine},
  {B_POS_EDGE, IDLE, rotateCounterClockwise},
  {DEFAULT_EV, RCC3, noActRoutine}
};


/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void encoderInit(void)
{
  for (uint8_t id = 0; id < ENCODER_COUNT; id++)
  {
    initSingleEncoder(id);
  }

  #ifdef ISR_DEVELOPMENT_MODE
  gpioWrite(ENCODER_ISR_DEV, LOW);
  gpioMode(ENCODER_ISR_DEV, OUTPUT);
  #endif
}

void registerCallbacks(encoder_id_t id, encoder_callback_t clockwiseCallback, encoder_callback_t counterClockwiseCallback)
{
  #ifdef TIMER_DEVELOPMENT_MODE
  if(id >= ENCODER_COUNT || id < 0)
    return; // exception (NMI)
  #endif

  registerClockwiseCallback(id, clockwiseCallback);
  registerCounterClockwiseCallback(id, counterClockwiseCallback);
  enableEncoder(id);

}

void registerClockwiseCallback(encoder_id_t id, encoder_callback_t clockwiseCallback)
{
  #ifdef TIMER_DEVELOPMENT_MODE
  if(id >= ENCODER_COUNT || id < 0)
    return; // exception (NMI)
  #endif
  
  encodersList[id].clockwiseCallback = clockwiseCallback;
  enableEncoder(id);

}

void registerCounterClockwiseCallback(encoder_id_t id, encoder_callback_t counterClockwiseCallback)
{
  #ifdef TIMER_DEVELOPMENT_MODE
  if(id >= ENCODER_COUNT || id < 0)
    return; // exception (NMI)
  #endif

  encodersList[id].counterClockwiseCallback = counterClockwiseCallback;
  enableEncoder(id);
}

void disableEncoder(encoder_id_t id)
{
  #ifdef TIMER_DEVELOPMENT_MODE
  if(id >= ENCODER_COUNT || id < 0)
    return; // exception (NMI)
  #endif

  encodersList[id].enabled = false;
}

void enableEncoder(encoder_id_t id)
{
  #ifdef TIMER_DEVELOPMENT_MODE
  if(id >= ENCODER_COUNT || id < 0)
    return; // exception (NMI)
  #endif
  
  encodersList[id].enabled = true;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
ENCODER_ISR_HANDLER(0, A)
ENCODER_ISR_HANDLER(0, B)
ENCODER_ISR_HANDLER(1, A)
ENCODER_ISR_HANDLER(1, B)

//* To add a new encoder:
//* ENCODER_ISR_HANDLER(1,A)
//* ENCODER_ISR_HANDLER(1,B)

void initSingleEncoder(encoder_id_t id)
{
  gpioMode(encodersList[id].pinNumberA, INPUT);
  gpioMode(encodersList[id].pinNumberB, INPUT);
  gpioIRQ(encodersList[id].pinNumberA, GPIO_IRQ_MODE_INTERRUPT_BOTH_EDGES, encodersList[id].isrA);
  gpioIRQ(encodersList[id].pinNumberB, GPIO_IRQ_MODE_INTERRUPT_BOTH_EDGES, encodersList[id].isrB);
}

void FSMCycle(encoder_id_t id, encoder_signal_t signal)
{
  #ifdef ISR_DEVELOPMENT_MODE
  gpioWrite(ENCODER_ISR_DEV, HIGH);
  #endif

  if (encodersList[id].enabled)
  {
    encoder_fsm_event_t ev = getEvent(id, signal);

    encoder_state_t* stateIterator = encodersList[id].currentState;
    while (stateIterator->event != ev && stateIterator->event != DEFAULT_EV)
    {
      stateIterator++;
    }

    (*stateIterator->actionRoutine)(id);
    encodersList[id].currentState = stateIterator->nextState;
  }
  
  #ifdef ISR_DEVELOPMENT_MODE
  gpioWrite(ENCODER_ISR_DEV, LOW);
  #endif
}

encoder_fsm_event_t getEvent(encoder_id_t id, encoder_signal_t signal)
{
  encoder_fsm_event_t ev;

  // Getting whether the event triggered is a positive edge or negative edge, and determining which encoder signal triggered it.
  if (signal == A)
  {
    // Signal A triggered the event.
    if (gpioRead(encodersList[id].pinNumberA))
    {
      // A is now HIGH, therefore it was a positive edge.
      ev = A_POS_EDGE;
    }
    else
    {
      // A is now LOW, therefore it was a negative edge.
      ev = A_NEG_EDGE;
    }
    
  }
  else
  {
    // Signal B triggered the event.
    if (gpioRead(encodersList[id].pinNumberB))
    {
      // B is now HIGH, therefore it was a positive edge.
      ev = B_POS_EDGE;
    }
    else
    {
      // A is now LOW, therefore it was a negative edge.
      ev = B_NEG_EDGE;
    }
  }

  return ev;
}

void noActRoutine(encoder_id_t id)
{
  // Nothing to do here.
}

void rotateClockwise(encoder_id_t id)
{
  encodersList[id].clockwiseCallback();
}

void rotateCounterClockwise(encoder_id_t id)
{
  encodersList[id].counterClockwiseCallback();
}

/******************************************************************************/
