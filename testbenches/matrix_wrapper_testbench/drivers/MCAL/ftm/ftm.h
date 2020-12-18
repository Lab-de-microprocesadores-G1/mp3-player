/*******************************************************************************
  @file     ftm.h

  @brief    FTM Driver
			When using the FTM driver you should always follow these general steps,
				(1°) Timer general setup using ftmInit(...)
				(2°) Channel specific setup, for instance ftmOutputCompareInit(...) 
					 and, maybe, register a callback on channel match
				(3°) Never forget starting the ftm at the end with ftmStart(...)

  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_FTM_FTM_H_
#define MCAL_FTM_FTM_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// FlexTimer Channels
typedef enum {
	FTM_CHANNEL_0,
	FTM_CHANNEL_1,
	FTM_CHANNEL_2,
	FTM_CHANNEL_3,
	FTM_CHANNEL_4,
	FTM_CHANNEL_5,
	FTM_CHANNEL_6,
	FTM_CHANNEL_7,
	FTM_CHANNEL_COUNT
} ftm_channel_t;

// FlexTimer Instances
typedef enum {
	FTM_INSTANCE_0,
	FTM_INSTANCE_1,
	FTM_INSTANCE_2,
	FTM_INSTANCE_3,
	FTM_INSTANCE_COUNT
} ftm_instance_t;

// Input Capture Modes
typedef enum {
	FTM_IC_RISING_EDGE,
	FTM_IC_FALLING_EDGE,
	FTM_IC_BOTH_EDGES
} ftm_ic_mode_t;

// Output Compare Modes
typedef enum {
	FTM_OC_TOGGLE,
	FTM_OC_CLEAR,
	FTM_OC_SET
} ftm_oc_mode_t;

// PWM Modes
typedef enum {
	FTM_PWM_HIGH_PULSES,
	FTM_PWM_LOW_PULSES
} ftm_pwm_mode_t;

// PWM Alignments
typedef enum {
	FTM_PWM_EDGE_ALIGNED,
	FTM_PWM_CENTER_ALIGNED
} ftm_pwm_alignment_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*****************************
*                            *
* FTM TIMER GENERAL SERVICES *
*                            *
*****************************/

/*
 * @brief Configures the FlexTimer module or instance selected, the timer remains stopped.
 * @param instance 		FTM Instance
 * @param prescaler		Frequency divider
 * @param module		Value to reset the count
 */
void ftmInit(ftm_instance_t instance, uint8_t prescaler, uint16_t module);

/*
 * @brief Starts running the FlexTimer module.
 * @param instance		FTM Instance
 */
void ftmStart(ftm_instance_t instance);

/*
 * @brief Stops running the FlexTimer module.
 * @param instance		FTM Instance
 */
void ftmStop(ftm_instance_t instance);

/*
 * @brief Returns the current value of the FlexTimer counter.
 * @param instance		FTM Instance
 */
uint16_t ftmGetCount(ftm_instance_t instance);

/*
 * @brief Registers action to be done on instance overflow. Registering a callback through
 * 		  this service enables the overflow interruption of the selected FlexTimer module.
 * @param instance		FTM Instance
 * @param callback		Callback to be called on overflow event
 */
void ftmOverflowSubscribe(ftm_instance_t instance, void (*callback)(void));

/*****************************
*                            *
*  CHANNEL GENERAL SERVICES  *
*                            *
*****************************/

/*
 * @brief Sets the value of the channel's count register. Particularly useful when running
 * 	      channel as output compare mode to set the value where the match event is triggered.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param count			Sets the value of the channel count where matching process occurs
 */
void ftmChannelSetCount(ftm_instance_t instance, ftm_channel_t channel, uint16_t count);

/*
 * @brief Returns current value of the channel count register.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 */
uint16_t ftmChannelGetCount(ftm_instance_t instance, ftm_channel_t channel);

/*
 * @brief Registers action to be done on channel event. This event is either the match event
 * 		  when running as output (Output Compare, PWM), or the edge detection event when running
 * 		  as input (Input Capture). Registering a callback through this service automatically enables
 * 		  channel interruption on the specified instance and channel.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param callback		Callback to be called on channel event
 */
void ftmChannelSubscribe(ftm_instance_t instance, ftm_channel_t channel, void (*callback)(uint16_t));

/*************************************
*                                    *
*   INPUT CAPTURE CHANNEL SERVICES   *
*                                    *
*************************************/

/*
 * @brief Configures FlexTimer's channel as Input Capture. After this, if FlexTimer instance
 * 		  is already running, you only have to register a callback for the channel event.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param mode			FTM Input Capture Mode
 */
void ftmInputCaptureInit(ftm_instance_t instance, ftm_channel_t channel, ftm_ic_mode_t mode);

/****************************************
*                                       *
*    OUTPUT COMPARE CHANNEL SERVICES    *
*                                       *
****************************************/

/*
 * @brief Configures FlexTimer's channel as Output Compare. Even if the FlexTimer instance is
 *        running, the Output Compare does not start working until you call ftmOutputCompareStart()
 *        which enables the matching process between the counter registers of the instance and the channel.
 *        This allows you to control when the output compare is working.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param mode			FTM Output Compare mode
 * @param outInit		FTM Output initial value when starting the operation
 */
void ftmOutputCompareInit(ftm_instance_t instance, ftm_channel_t channel, ftm_oc_mode_t mode, bool outInit);

/**
 * @brief Starts running the Output Compare, enables the match process.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param count			FTM ticks to wait until matching process
 */
void ftmOutputCompareStart(ftm_instance_t instance, ftm_channel_t channel, uint16_t count);

/**
 * @brief Stops running the Output Compare mode, disables the match process.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 */
void ftmOutputCompareStop(ftm_instance_t instance, ftm_channel_t channel);

/*****************************
*                            *
*    PWM CHANNEL SERVICES    *
*                            *
*****************************/

/*
 * @brief Configures FlexTimer instance and channel as PWM, starts running whenever the FlexTimer
 *        instance starts.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param mode			PWM mode
 * @param alignment		PWM Alignment
 * @param duty			Duty cycle ticks count
 * @param period		Period ticks count
 */
void ftmPwmInit(ftm_instance_t instance, ftm_channel_t channel, ftm_pwm_mode_t mode, ftm_pwm_alignment_t alignment, uint16_t duty, uint16_t period);

/*
 * @brief Updates the current duty value of the channel configured as PWM, synchronization process is triggered
 * 		  and handled automatically.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param duty			Duty cycle ticks count
 */
void ftmPwmSetDuty(ftm_instance_t instance, ftm_channel_t channel, uint16_t duty);

/**
 * @brief Sets the enable status of the channel output, even if the PWM is still working
 * 		  the output is not driven by its state when disabled. Can be useful for turning off
 * 		  the PWM without changing it current duty value.
 * @param instance		FTM Instance
 * @param channel		FTM Channel
 * @param running		True if enabled, false if disabled (output is masked)
 */
void ftmPwmSetEnable(ftm_instance_t instance, ftm_channel_t channel, bool running);


/*******************************************************************************
 ******************************************************************************/

#endif
