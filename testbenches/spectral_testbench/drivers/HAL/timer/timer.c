/***************************************************************************//**
  @file     timer.c
  @brief    Timer driver. Advance implementation
  @author   N. Magliola, G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "timer.h"
#include "../../MCAL/systick/SysTick.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#if TIMER_TICK_MS != (1000U/SYSTICK_ISR_FREQUENCY_HZ)
#error Las frecuencias no coinciden!!
#endif // TIMER_TICK_MS != (1000U/SYSTICK_ISR_FREQUENCY_HZ)

#define TIMER_DEVELOPMENT_MODE    1

#define TIMER_ID_INTERNAL   0

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef struct {
	ttick_t             period;
	ttick_t             cnt;
    tim_callback_t      callback;
    uint8_t             mode        : 1;
    uint8_t             running     : 1;
    uint8_t             expired     : 1;
    uint8_t             unused      : 5;
} timer_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Periodic service
 */
static void timer_isr(void);


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static timer_t timers[TIMERS_MAX_CANT];
static tim_id_t timers_cant = TIMER_ID_INTERNAL+1;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void timerInit(void)
{
    static bool yaInit = false;
    if (yaInit)
        return;
    
    systickInit(timer_isr); // init peripheral
    
    yaInit = true;
}


tim_id_t timerGetId(void)
{
#ifdef TIMER_DEVELOPMENT_MODE
    if (timers_cant >= TIMERS_MAX_CANT)
    {
        return TIMER_INVALID_ID;
    }
    else
#endif // TIMER_DEVELOPMENT_MODE
    {
        return timers_cant++;
    }
}


void timerStart(tim_id_t id, ttick_t ticks, uint8_t mode, tim_callback_t callback)
{
#ifdef TIMER_DEVELOPMENT_MODE
    if ((id < timers_cant) && (mode < CANT_TIM_MODES))
#endif // TIMER_DEVELOPMENT_MODE
    {
        // disable timer
        timers[id].running = 0;

        // configure timer
        timers[id].period = ticks;
        timers[id].cnt = ticks;
        timers[id].callback = callback;
        timers[id].mode = mode;
        timers[id].expired = 0;

        // enable timer
        timers[id].running = 1;
    }
}

void timerResume(tim_id_t id)
{
#ifdef TIMER_DEVELOPMENT_MODE
    if (id < timers_cant)
#endif // TIMER_DEVELOPMENT_MODE
    {
        // Si esta pausado el timer
        if (!timers[id].running)
        {
            // Reanudo el timer
            timers[id].cnt = timers[id].period;
            timers[id].running = 1;
        }
    }
}

void timerPause(tim_id_t id)
{
#ifdef TIMER_DEVELOPMENT_MODE
    if (id < timers_cant)
#endif // TIMER_DEVELOPMENT_MODE
    {
        // Apago el timer
        timers[id].running = 0;

        // y bajo el flag
        timers[id].expired = 0;
    }
}

void timerRestart(tim_id_t id)
{
#ifdef TIMER_DEVELOPMENT_MODE
    if (id < timers_cant)
#endif // TIMER_DEVELOPMENT_MODE
    {
        // disable timer
        timers[id].running = 0;

        // configure timer
        timers[id].cnt = timers[id].period;
        timers[id].expired = 0;

        // enable timer
        timers[id].running = 1;
    }
}

bool timerRunning(tim_id_t id)
{
    return timers[id].running;
}

bool timerExpired(tim_id_t id)
{
    // Verifico si expiró el timer
    bool hasExpired = timers[id].expired;

    // y bajo el flag
    timers[id].expired = 0;

    return hasExpired;
}

void timerDelay(ttick_t ticks)
{
    timerStart(TIMER_ID_INTERNAL, ticks, TIM_MODE_SINGLESHOT, NULL);
    while (!timerExpired(TIMER_ID_INTERNAL));
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void timer_isr(void)
{
    // decremento los timers activos
    uint8_t timerIndex;
    for (timerIndex = 0 ; timerIndex < timers_cant ; timerIndex++)
    {
        timer_t* currentTimer = &timers[timerIndex];
        if (currentTimer->running)
        {
            currentTimer->cnt--;
            // si hubo timeout!
            if (currentTimer->cnt == 0)
            {
            	// Important: first update state so that if timerStart()
				// is called in the callback, this block doesn't deletes
            	// the configuration
                // 1) update state
                if (currentTimer->mode == TIM_MODE_SINGLESHOT)
                {
                    currentTimer->expired = 1;
                    currentTimer->running = 0;
                }
                else
                {
                    currentTimer->cnt = currentTimer->period;
                    currentTimer->expired = 1;
                }

            	// 2) execute action: callback or set flag
                if (currentTimer->callback)
                {
                    currentTimer->callback();
                }

            }
        }
    }
}

/******************************************************************************/
