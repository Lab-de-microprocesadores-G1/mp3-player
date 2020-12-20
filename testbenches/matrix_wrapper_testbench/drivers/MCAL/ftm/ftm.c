/*******************************************************************************
  @file     ftm.c
  @brief    FTM Driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "hardware.h"
#include "ftm.h"
#include "../gpio/gpio.h"
#include "../../../CMSIS/MK64F12.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#if !defined(FTM_DRIVER_LEGACY_MODE) && !defined(FTM_DRIVER_ADVANCED_MODE)
	#error	Need to define the operation mode of the driver.
#endif

#define CHANNEL_MASK(x)		(0x00000001 << (x))

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// Callback types of the FTM driver
typedef void 	(*ov_callback)	(void);
typedef void	(*ch_callback)	(uint16_t);

// Clock source options for the FTM instance
enum {
	FTM_CLOCK_DISABLED,
	FTM_CLOCK_SYSTEM,
	FTM_CLOCK_INTERNAL,
	FTM_CLOCK_EXTERNAL	
};

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

// FTM instances interruption service routines
static void 	FTM_IRQDispatch(ftm_instance_t instance);
__ISR__ FTM0_IRQHandler(void);
__ISR__ FTM1_IRQHandler(void);
__ISR__ FTM2_IRQHandler(void);
__ISR__ FTM3_IRQHandler(void);

// Configure PORT MUX 
static void setFtmChannelMux(ftm_instance_t instance, ftm_channel_t channel);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

// Callbacks registered for both overflow and channel of every FTM instance
static ov_callback		ftmOverflowCallbacks[FTM_INSTANCE_COUNT];
static ch_callback		ftmChannelCallbacks[FTM_INSTANCE_COUNT][FTM_CHANNEL_COUNT];

// Pointers to FTM Instances
static FTM_Type*	ftmInstances[] = FTM_BASE_PTRS;

// FTMIRQn for NVIC Enabling
static const uint8_t 	ftmIrqs[] = FTM_IRQS;

// The one and only one look up table by Nico "El Rafa" Trozzo
static const pin_t		ftmChannelPins[FTM_INSTANCE_COUNT][FTM_CHANNEL_COUNT] = {
	//	Channel 0			Channel 1			Channel 2			Channel 3		Channel 4			Channel 5			Channel 6			Channel 7
	{ PORTNUM2PIN(PC,1),  PORTNUM2PIN(PA,4),  PORTNUM2PIN(PC,3), PORTNUM2PIN(PC,4), PORTNUM2PIN(PD,4), PORTNUM2PIN(PD,5), PORTNUM2PIN(PD,6)	, PORTNUM2PIN(PD,7)  },	// FTM0
	{ PORTNUM2PIN(PA,12), PORTNUM2PIN(PA,13), 0				   , 0				  , 0				 , 0			   	, 0				   	, 0				     }, // FTM1
	{ PORTNUM2PIN(PB,18), PORTNUM2PIN(PB,19), 0				   , 0				  , 0				 , 0			   	, 0				   	, 0				     }, // FTM2
	{ PORTNUM2PIN(PD,0),  PORTNUM2PIN(PD,1),  PORTNUM2PIN(PD,2), PORTNUM2PIN(PD,3), PORTNUM2PIN(PC,8), PORTNUM2PIN(PC,9), PORTNUM2PIN(PC,10), PORTNUM2PIN(PC,11) }  // FTM3
};

// FTM Channel Pin MUX Alternatives
static const uint8_t	ftmChannelAlts[FTM_INSTANCE_COUNT][FTM_CHANNEL_COUNT] = {
	// Ch0 Ch1 Ch2 Ch3 Ch4 Ch5 Ch6 Ch7
	{  4,  3,  4,  4,  4,  4,  4,  4  }, // FTM0
	{  3,  3,  0,  0,  0,  0,  0,  0  }, // FTM1
	{  3,  3,  0,  0,  0,  0,  0,  0  }, // FTM2
	{  4,  4,  4,  4,  3,  3,  3,  3  }  // FTM3
};

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void ftmInit(ftm_instance_t instance, uint8_t prescaler, uint16_t module)
{
	// Clock gating enable
	switch(instance)
	{
		case FTM_INSTANCE_0:
			SIM->SCGC6 |= SIM_SCGC6_FTM0_MASK;
			break;
			
		case FTM_INSTANCE_1:
			SIM->SCGC6 |= SIM_SCGC6_FTM1_MASK;
			break;

		case FTM_INSTANCE_2:
			SIM->SCGC3 |= SIM_SCGC3_FTM2_MASK;
			SIM->SCGC6 |= SIM_SCGC6_FTM2_MASK;
			break;

		case FTM_INSTANCE_3:
			SIM->SCGC3 |= SIM_SCGC3_FTM3_MASK;
			break;

		default:
			break;
	}

	// Port Clock Gating
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

	// Enable Interrupts on NVIC
	NVIC_EnableIRQ(ftmIrqs[instance]);

	// Enable CNTIN and MOD configuration
	ftmInstances[instance]->PWMLOAD = FTM_PWMLOAD_LDOK(1);

	// Free-running counter and prescaler
	ftmInstances[instance]->SC = FTM_SC_PS(prescaler);
	ftmInstances[instance]->CNTIN = 0;
	ftmInstances[instance]->CNT = 0;
	ftmInstances[instance]->MOD = module - 1;

	// Enable advanced mode
#ifdef FTM_DRIVER_LEGACY_MODE
	ftmInstances[instance]->MODE = (ftmInstances[instance]->MODE & ~FTM_MODE_FTMEN_MASK) | FTM_MODE_FTMEN(0);
#endif
#ifdef FTM_DRIVER_ADVANCED_MODE
	ftmInstances[instance]->MODE = (ftmInstances[instance]->MODE & ~FTM_MODE_FTMEN_MASK) | FTM_MODE_FTMEN(1);
#endif
}

void ftmStart(ftm_instance_t instance)
{
	ftmInstances[instance]->SC = (ftmInstances[instance]->SC & ~FTM_SC_CLKS_MASK) | FTM_SC_CLKS(FTM_CLOCK_SYSTEM);
}

void ftmRestart(ftm_instance_t instance)
{
	ftmStop(instance);
	ftmInstances[instance]->CNT = 0;
	ftmStart(instance);
}

void ftmStop(ftm_instance_t instance)
{
	ftmInstances[instance]->SC = (ftmInstances[instance]->SC & ~FTM_SC_CLKS_MASK) | FTM_SC_CLKS(0);
}

uint16_t ftmGetCount(ftm_instance_t instance)
{
	return ftmInstances[instance]->CNT;
}

void ftmOverflowSubscribe(ftm_instance_t instance, void (*callback)(void))
{
	if (callback)
	{
		// Enables the timer overflow interrupt for the FTM instance
		ftmInstances[instance]->SC |= FTM_SC_TOIE(1);

		// Registers the callback to be called when timer overflow
		ftmOverflowCallbacks[instance] = callback;
	}
}

void ftmChannelSetCount(ftm_instance_t instance, ftm_channel_t channel, uint16_t count)
{
	ftmInstances[instance]->CONTROLS[channel].CnV = count;
}

uint16_t ftmChannelGetCount(ftm_instance_t instance, ftm_channel_t channel)
{
	return ftmInstances[instance]->CONTROLS[channel].CnV;
}

volatile uint32_t* ftmChannelCounter(ftm_instance_t instance, ftm_channel_t channel)
{
	return &(ftmInstances[instance]->CONTROLS[channel].CnV);
}

void ftmChannelSubscribe(ftm_instance_t instance, ftm_channel_t channel, void (*callback)(uint16_t))
{
	if (callback)
	{
		// Enables the channel interrupt
		ftmInstances[instance]->CONTROLS[channel].CnSC |= FTM_CnSC_CHIE(1);

		// Registers the callback to be called when channel match occurs
		ftmChannelCallbacks[instance][channel] = callback;
	}
}

void ftmChannelEnableDMA(ftm_instance_t instance, ftm_channel_t channel)
{
	ftmInstances[instance]->CONTROLS[channel].CnSC = (ftmInstances[instance]->CONTROLS[channel].CnSC & ~FTM_CnSC_DMA_MASK) | FTM_CnSC_DMA(1);
	ftmInstances[instance]->CONTROLS[channel].CnSC = (ftmInstances[instance]->CONTROLS[channel].CnSC & ~FTM_CnSC_CHIE_MASK) | FTM_CnSC_CHIE(1);
}

void ftmInputCaptureInit(ftm_instance_t instance, ftm_channel_t channel, ftm_ic_mode_t mode)
{
	// Channel set to input capture on given edge/s
	ftmInstances[instance]->CONTROLS[channel].CnSC = FTM_CnSC_ELSA(mode == FTM_IC_RISING_EDGE ? 0 : 1) | FTM_CnSC_ELSB(mode == FTM_IC_FALLING_EDGE ? 0 : 1);
	
	// Pin MUX alternative
	setFtmChannelMux(instance, channel);
}

void ftmOutputCompareInit(ftm_instance_t instance, ftm_channel_t channel, ftm_oc_mode_t mode, bool outInit)
{
	// Configuration of the channel as output compare
	ftmInstances[instance]->CONTROLS[channel].CnSC = FTM_CnSC_MSB(0) | FTM_CnSC_MSA(1) | FTM_CnSC_ELSB(mode == FTM_OC_TOGGLE ? 0 : 1) | FTM_CnSC_ELSA(mode == FTM_OC_CLEAR ? 0 : 1);

	// Pin MUX alternative
	setFtmChannelMux(instance, channel);

	// Sets the initial value of the output channel
#ifdef FTM_DRIVER_ADVANCED_MODE
	uint32_t outputMask = CHANNEL_MASK(channel);
	ftmInstances[instance]->OUTINIT = outInit ? (ftmInstances[instance]->OUTINIT | outputMask) : (ftmInstances[instance]->OUTINIT & (~outputMask));
#endif
}

void ftmOutputCompareStart(ftm_instance_t instance, ftm_channel_t channel, uint16_t count)
{
	// Forces the output channel to its initial value registered during the initialization process
#ifdef FTM_DRIVER_ADVANCED_MODE
	ftmInstances[instance]->MODE = (ftmInstances[instance]->MODE & ~FTM_MODE_INIT_MASK) | FTM_MODE_INIT(1);
#endif

	// Enables the matching process on the selected channel and updates the current count
	ftmInstances[instance]->CONTROLS[channel].CnV = ftmInstances[instance]->CNT + count;
#ifdef FTM_DRIVER_ADVANCED_MODE
	ftmInstances[instance]->PWMLOAD |= CHANNEL_MASK(channel);
	ftmInstances[instance]->OUTMASK &= (~CHANNEL_MASK(channel));
#endif
}

void ftmOutputCompareStop(ftm_instance_t instance, ftm_channel_t channel)
{
#ifdef FTM_DRIVER_ADVANCED_MODE
	// Disables the matching process on the PWMLOAD register
	ftmInstances[instance]->PWMLOAD &= (~CHANNEL_MASK(channel));
	ftmInstances[instance]->OUTMASK |= CHANNEL_MASK(channel);
#endif
}

void ftmPwmInit(ftm_instance_t instance, ftm_channel_t channel, ftm_pwm_mode_t mode, ftm_pwm_alignment_t alignment, uint16_t duty, uint16_t period)
{
	// Configure up or up/down counter 
	ftmInstances[instance]->SC = (ftmInstances[instance]->SC & ~FTM_SC_CPWMS_MASK) | FTM_SC_CPWMS(alignment == FTM_PWM_CENTER_ALIGNED ? 1 : 0);

	// Configure channel to PWM on the given mode and alignment
	ftmInstances[instance]->CONTROLS[channel].CnSC = FTM_CnSC_MSB(1) | FTM_CnSC_ELSB(1) | FTM_CnSC_ELSA(mode == FTM_PWM_LOW_PULSES ? 1 : 0);
	
	// Enable changes on MOD, CNTIN and CnV
#ifdef FTM_DRIVER_ADVANCED_MODE
	ftmInstances[instance]->PWMLOAD |= FTM_PWMLOAD_LDOK(1) | CHANNEL_MASK(channel);
#endif

	// Configure PWM period and duty
#ifdef FTM_DRIVER_ADVANCED_MODE
	ftmInstances[instance]->CNTIN = 0;
#endif
	ftmInstances[instance]->MOD = period - 1;
	ftmInstances[instance]->CONTROLS[channel].CnV = duty;
	
	// Pin MUX alternative
	setFtmChannelMux(instance, channel);

#ifdef FTM_DRIVER_ADVANCED_MODE
	// Enable Synchronization
	ftmInstances[instance]->COMBINE |= (FTM_COMBINE_SYNCEN0_MASK << (8 * (channel / 2)));

	// Advanced synchronization, and enable Software Trigger to change CnV!
	ftmInstances[instance]->SYNCONF |= FTM_SYNCONF_SYNCMODE_MASK | FTM_SYNCONF_SWWRBUF_MASK;

	// Sync when CNT == MOD - 1
	ftmInstances[instance]->SYNC |= FTM_SYNC_CNTMAX_MASK;
#endif
}

void ftmPwmSetDuty(ftm_instance_t instance, ftm_channel_t channel, uint16_t duty)
{
#ifdef FTM_DRIVER_ADVANCED_MODE
	// Software Trigger
	ftmInstances[instance]->SYNC |= FTM_SYNC_SWSYNC_MASK;
#endif

	// Change Duty
	ftmInstances[instance]->CONTROLS[channel].CnV = duty;
}

void ftmPwmSetEnable(ftm_instance_t instance, ftm_channel_t channel, bool running)
{
#ifdef FTM_DRIVER_ADVANCED_MODE
	// Software Trigger
	ftmInstances[instance]->SYNC |= FTM_SYNC_SWSYNC_MASK;

	// Change OutMask
	if (running)
	{
		ftmInstances[instance]->OUTMASK &= (~CHANNEL_MASK(channel));
	}
	else
	{
		ftmInstances[instance]->OUTMASK |= CHANNEL_MASK(channel);
	}
#endif
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
static void FTM_IRQDispatch(ftm_instance_t instance)
{
	// Verify if the interruption occurred because of the overflow
	// or because of a matching process in any of the timer channels
	if (ftmInstances[instance]->SC & FTM_SC_TOF_MASK)
	{
		// Clear the interruption flag
		ftmInstances[instance]->SC &= (~FTM_SC_TOF_MASK);
		
		// Calls the callback registered (if any)
		ov_callback overflowCallback = ftmOverflowCallbacks[instance];
		if (overflowCallback)
		{
			overflowCallback();
		}
	}
	else
	{
		// Save current status of the interruption flags for all channels
		uint32_t status = ftmInstances[instance]->STATUS;

		// Clear flags in the FlexTimer status register
		ftmInstances[instance]->STATUS = 0x00000000;

		for (ftm_channel_t channel = 0; channel < FTM_CHANNEL_COUNT; channel++)
		{
			// Fetch channel interruption callback
			ch_callback channelCallback = ftmChannelCallbacks[instance][channel];

			// If interruption callback registered, verify if flag was active...
			// laziness will prevent shifting and masking status when no callback registered
			if (channelCallback && (status & CHANNEL_MASK(channel)))
			{
				channelCallback(ftmInstances[instance]->CONTROLS[channel].CnV);
			}
		}
	}
}

__ISR__ FTM0_IRQHandler(void)
{
	FTM_IRQDispatch(FTM_INSTANCE_0);
}

__ISR__ FTM1_IRQHandler(void)
{
	FTM_IRQDispatch(FTM_INSTANCE_1);
}

__ISR__ FTM2_IRQHandler(void)
{
	FTM_IRQDispatch(FTM_INSTANCE_2);
}

__ISR__ FTM3_IRQHandler(void)
{
	FTM_IRQDispatch(FTM_INSTANCE_3);
}

static void setFtmChannelMux(ftm_instance_t instance, ftm_channel_t channel)
{
	PORT_Type* 	ports[] = PORT_BASE_PTRS;
	pin_t 		pin = ftmChannelPins[instance][channel];
	uint8_t 	alt = ftmChannelAlts[instance][channel];
	ports[PIN2PORT(pin)]->PCR[PIN2NUM(pin)] |= PORT_PCR_MUX(alt);
}

/******************************************************************************/
