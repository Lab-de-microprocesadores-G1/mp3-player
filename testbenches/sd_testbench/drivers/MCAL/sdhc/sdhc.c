/*******************************************************************************
  @file     sdhc.c
  @brief    Secure Digital Host Controller peripheral driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "hardware.h"
#include "MK64F12.h"
#include "sdhc.h"

#include "drivers/MCAL/gpio/gpio.h"

#include <stdio.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// SDHC declaring pins
#define SDHC_SWITCH_PIN		PORTNUM2PIN(PE, 6)
#define SDHC_CMD_PIN		PORTNUM2PIN(PE, 3)
#define SDHC_DCLK_PIN		PORTNUM2PIN(PE, 2)
#define SDHC_D0_PIN			PORTNUM2PIN(PE, 1)
#define SDHC_D1_PIN			PORTNUM2PIN(PE, 0)
#define SDHC_D2_PIN			PORTNUM2PIN(PE, 5)
#define SDHC_D3_PIN			PORTNUM2PIN(PE, 4)

// SDHC declaring PCR configuration for each pin
#define SDHC_CMD_PCR		PORT_PCR_MUX(4)
#define SDHC_DCLK_PCR		PORT_PCR_MUX(4)
#define SDHC_D0_PCR			PORT_PCR_MUX(4)
#define SDHC_D1_PCR			PORT_PCR_MUX(4)
#define SDHC_D2_PCR			PORT_PCR_MUX(4)
#define SDHC_D3_PCR			PORT_PCR_MUX(4)

// SDHC internal parameters
#define SDHC_MAXIMUM_BLOCK_SIZE		4096
#define SDHC_RESET_TIMEOUT			100000
#define SDHC_CLOCK_FREQUENCY		(96000000U)

// SDHC possible values for the register fields
#define SDHC_RESPONSE_LENGTH_NONE	SDHC_XFERTYP_RSPTYP(0b00)
#define SDHC_RESPONSE_LENGTH_48		SDHC_XFERTYP_RSPTYP(0b10)
#define SDHC_RESPONSE_LENGTH_136	SDHC_XFERTYP_RSPTYP(0b01)
#define SDHC_RESPONSE_LENGTH_48BUSY	SDHC_XFERTYP_RSPTYP(0b11)

#define SDHC_COMMAND_CHECK_CCR		SDHC_XFERTYP_CCCEN(0b1)
#define SDHC_COMMAND_CHECK_INDEX	SDHC_XFERTYP_CICEN(0b1)

// SDHC flag shortcuts
#define SDHC_COMMAND_COMPLETED_FLAG		(SDHC_IRQSTAT_CC_MASK)
#define SDHC_TRANSFER_COMPLETED_FLAG	(SDHC_IRQSTAT_TC_MASK)
#define SDHC_CARD_DETECTED_FLAGS		(SDHC_IRQSTAT_CINS_MASK | SDHC_IRQSTAT_CRM_MASK)
#define SDHC_DATA_FLAG					(SDHC_IRQSTAT_BRR_MASK | SDHC_IRQSTAT_BWR_MASK)
#define SDHC_DATA_TIMEOUT_FLAG			(SDHC_IRQSTAT_DTOE_MASK)
#define SDHC_ERROR_FLAG					(																										 \
											SDHC_IRQSTAT_DMAE_MASK | SDHC_IRQSTAT_AC12E_MASK | SDHC_IRQSTAT_DEBE_MASK |  SDHC_IRQSTAT_DCE_MASK | \
											SDHC_IRQSTAT_CIE_MASK | SDHC_IRQSTAT_CEBE_MASK | SDHC_IRQSTAT_CCE_MASK |    \
											SDHC_IRQSTAT_CTOE_MASK														 \
										)

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// SDHC driver context variables
typedef struct {
	// Peripheral capabilities
	bool			lowVoltageSupported;	// If the peripheral support 3.3V voltage
	bool			suspendResumeSupported;	// If supports suspend/resume functionalities
	bool			dmaSupported;			// If supports using DMA
	bool			highSpeedSupported;		// If supports clocking frequency higher than 25MHz
	bool			admaSupported;			// If supports using Advanced DMA
	uint16_t		maxBlockSize;			// Maximum size of block supported by the peripheral
	uint32_t		readWatermarkLevel;		// Read watermark level
	uint32_t		writeWatermarkLevel;	// Write watermark level

	// Current status
	sdhc_command_t*	currentCommand;			// Current command being transfered
	sdhc_data_t*	currentData;			// Current data being transfered
	size_t			transferedWords;		// Amount of transfered bytes
	sdhc_error_t	currentError;			// Current error status of the driver

	// Callback
	sdhc_callback_t	onCardInserted;			// Callback to be called when card is inserted
	sdhc_callback_t	onCardRemoved;			// Callback to be called when card is removed
	sdhc_callback_t	onTransferCompleted;	// Callback to be called when a transfer is completed
	sdhc_error_callback_t onTransferError;	// Callback to be called when a transfer error occurs

	// Flags
	bool			alreadyInit;			// true: driver was already initialized
	bool			cardStatus;				// true: is inserted, false: is removed
	bool			transferCompleted;		// true: last transfer was completed
	bool			available;				// true: if driver is available or not busy
} sdhc_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

// Interrupt Service Routines, used to handle interruption on specific flags
// fired by the peripheral being controlled by the driver. Handlers receive a
// parameter called 'status' which contains the current value of the interrupt
// status flags, at the moment of being called.
__ISR__ SDHC_IRQHandler(void);
static void SDHC_CommandCompletedHandler	(uint32_t status);
static void SDHC_TransferCompletedHandler	(uint32_t status);
static void SDHC_DataHandler				(uint32_t status);
static void SDHC_CardDetectedHandler		(void);
static void SDHC_TransferErrorHandler		(uint32_t status);

/*
 * @brief Computes the best register values to be configured in the peripheral to get
 * 		  the desired frequency for the SD bus.
 * @param frequency		Target frequency
 * @param sdclks		sdclks register field
 * @param dvs			dvs register field
 */
static void	getSettingsByFrequency(uint32_t frequency, uint8_t* sdclks, uint8_t* dvs);

/*
 * @brief Computes the resulting frequency with the given register values
 * @param prescaler		Prescaler value for the clock
 * @param divisor		Divisor value for the clock
 */
static uint32_t computeFrequency(uint8_t prescaler, uint8_t divisor);

/*
 * @brief Initializes the context with its default values, and loads the SDHC
 * 		  peripheral capabilities into the internal variables of the structure.
 */
static void contextInit(void);

/*
 * @brief Set the transfer complete flag, raises the event and clears the available status.
 */
static void contextSetTransferComplete(void);

/**
 * @brief Returns the next word available to be read.
 * @returns		The next word received
 */
static uint32_t sdhcReadWord(void);

/*
 * @brief Reads many words received and stores them in the given memory buffer.
 * @param buffer	Pointer to the memory buffer
 * @param count		Amount of words to be read
 */
static void sdhcReadManyWords(uint32_t* buffer, size_t count);

/*
 * @brief Write the next word to be transfered.
 * @param data		Word to be written
 */
static void sdhcWriteWord(uint32_t data);

/*
 * @brief Write many words to be transfered from the memory buffer given
 * @param buffer	Pointer to the memory buffer
 * @param count		Amount of words to be read
 */
static void sdhcWriteManyWords(uint32_t* buffer, size_t count);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static sdhc_context_t	context;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void sdhcInit(sdhc_config_t config)
{
	if (!context.alreadyInit)
	{
		// Disables the memory protection unit
		SYSMPU->CESR = 0;

		// Initialization of GPIO peripheral to detect switch
		gpioMode(SDHC_SWITCH_PIN, INPUT | PULLDOWN);
		gpioIRQ(SDHC_SWITCH_PIN, GPIO_IRQ_MODE_INTERRUPT_BOTH_EDGES, SDHC_CardDetectedHandler);

		// Configuration of the clock gating for both SDHC, PORTE peripherals
		SIM->SCGC3 = (SIM->SCGC3 & ~SIM_SCGC3_SDHC_MASK)  | SIM_SCGC3_SDHC(1);
		SIM->SCGC5 = (SIM->SCGC5 & ~SIM_SCGC5_PORTE_MASK) | SIM_SCGC5_PORTE(1);

		// Setting the corresponding value to each pin PCR register
		PORTE->PCR[PIN2NUM(SDHC_CMD_PIN)] = SDHC_CMD_PCR;
		PORTE->PCR[PIN2NUM(SDHC_DCLK_PIN)] = SDHC_DCLK_PCR;
		PORTE->PCR[PIN2NUM(SDHC_D0_PIN)] = SDHC_D0_PCR;
		PORTE->PCR[PIN2NUM(SDHC_D1_PIN)] = SDHC_D1_PCR;
		PORTE->PCR[PIN2NUM(SDHC_D2_PIN)] = SDHC_D2_PCR;
		PORTE->PCR[PIN2NUM(SDHC_D3_PIN)] = SDHC_D3_PCR;

		// Reset the SDHC peripheral
		sdhcReset(SDHC_RESET_ALL | SDHC_RESET_CMD | SDHC_RESET_DATA);

		// Set the watermark
		context.writeWatermarkLevel = config.writeWatermarkLevel;
		context.readWatermarkLevel = config.readWatermarkLevel;
		SDHC->WML = (SDHC->WML & ~SDHC_WML_WRWML_MASK) | SDHC_WML_WRWML(config.writeWatermarkLevel);
		SDHC->WML = (SDHC->WML & ~SDHC_WML_RDWML_MASK) | SDHC_WML_RDWML(config.readWatermarkLevel);

		// Disable the automatically gating off of the peripheral's clock, hardware and other
		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_PEREN_MASK) | SDHC_SYSCTL_PEREN(1);
		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_HCKEN_MASK) | SDHC_SYSCTL_HCKEN(1);
		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_IPGEN_MASK) | SDHC_SYSCTL_IPGEN(1);

		// Disable the peripheral clocking, sets the divisor and prescaler for the new target frequency
		// and configures the new value for the time-out delay. Finally, enables the clock again.
		SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_DTOCV_MASK)   | SDHC_SYSCTL_DTOCV(0b1110);
		sdhcSetClock(config.frequency);

		// Disable interrupts and clear all flags
		SDHC->IRQSTATEN = 0;
		SDHC->IRQSIGEN = 0;
		SDHC->IRQSTAT = 0xFFFFFFFF;

		// Enable interrupts, signals and NVIC
		SDHC->IRQSTATEN = (
				SDHC_IRQSTATEN_CCSEN_MASK 		| SDHC_IRQSTATEN_TCSEN_MASK 	| SDHC_IRQSTATEN_BGESEN_MASK 	| SDHC_IRQSTATEN_DINTSEN_MASK 	|
				SDHC_IRQSTATEN_BWRSEN_MASK 		| SDHC_IRQSTATEN_BRRSEN_MASK 	| SDHC_IRQSTATEN_CTOESEN_MASK 	| SDHC_IRQSTATEN_CCESEN_MASK 	|
				SDHC_IRQSTATEN_CEBESEN_MASK 	| SDHC_IRQSTATEN_CIESEN_MASK 	| SDHC_IRQSTATEN_DTOESEN_MASK 	| SDHC_IRQSTATEN_DCESEN_MASK 	|
				SDHC_IRQSTATEN_DEBESEN_MASK 	| SDHC_IRQSTATEN_AC12ESEN_MASK
		);
		SDHC->IRQSIGEN = (
				SDHC_IRQSIGEN_CCIEN_MASK 	| SDHC_IRQSIGEN_TCIEN_MASK 		| SDHC_IRQSIGEN_BGEIEN_MASK 	| SDHC_IRQSIGEN_DINTIEN_MASK 	|
				SDHC_IRQSIGEN_BWRIEN_MASK 	| SDHC_IRQSIGEN_BRRIEN_MASK 	| SDHC_IRQSIGEN_CTOEIEN_MASK 	| SDHC_IRQSIGEN_CCEIEN_MASK 	|
				SDHC_IRQSIGEN_CEBEIEN_MASK 	| SDHC_IRQSIGEN_CIEIEN_MASK 	| SDHC_IRQSIGEN_DTOEIEN_MASK 	| SDHC_IRQSIGEN_DCEIEN_MASK 	|
				SDHC_IRQSIGEN_DEBEIEN_MASK 	| SDHC_IRQSIGEN_AC12EIEN_MASK
		);
		NVIC_EnableIRQ(SDHC_IRQn);

		// Initialization successful
		contextInit();
	}
}

void sdhcReset(sdhc_reset_t reset)
{
	uint32_t timeout = SDHC_RESET_TIMEOUT;
	uint32_t mask = 0;
	bool forceExit = false;

	// Selects the reset function
	if (reset == SDHC_RESET_DATA)
	{
		mask |= SDHC_SYSCTL_RSTD_MASK;
	}
	if (reset == SDHC_RESET_CMD)
	{
		mask |= SDHC_SYSCTL_RSTC_MASK;
	}
	if (reset == SDHC_RESET_ALL)
	{
		mask |= SDHC_SYSCTL_RSTA_MASK;
	}

	// Trigger a software reset for the peripheral hardware
	SDHC->SYSCTL |= mask;

	// Wait until the driver reset process has finished
	while (!forceExit && (SDHC->SYSCTL & mask))
	{
		if (timeout)
		{
			timeout--;
		}
		else
		{
			forceExit = true;
		}
	}

	// Correction to reset
	if (reset == SDHC_RESET_CMD)
	{
		SDHC->IRQSIGEN = SDHC->IRQSTATEN;
	}
}

uint16_t sdhcGetMaximumBlockCount(void)
{
	return SDHC_BLKATTR_BLKCNT_MASK >> SDHC_BLKATTR_BLKCNT_SHIFT;
}

uint16_t sdhcGetMaximumBlockSize(void)
{
	return SDHC_MAXIMUM_BLOCK_SIZE;
}

void sdhcSetClock(uint32_t frequency)
{
	uint8_t sdcklfs, dvs;
	getSettingsByFrequency(frequency, &sdcklfs, &dvs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKEN_MASK) | SDHC_SYSCTL_SDCLKEN(0);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKFS_MASK) | SDHC_SYSCTL_SDCLKFS(sdcklfs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_DVS_MASK)     | SDHC_SYSCTL_DVS(dvs);
	SDHC->SYSCTL = (SDHC->SYSCTL & ~SDHC_SYSCTL_SDCLKEN_MASK) | SDHC_SYSCTL_SDCLKEN(1);
}

void sdhcSetBusWidth(sdhc_data_width_t width)
{
	SDHC->PROCTL = (SDHC->PROCTL & ~SDHC_PROCTL_DTW_MASK) | SDHC_PROCTL_DTW(width);
}

sdhc_error_t sdhcGetErrorStatus(void)
{
	return context.currentError;
}

bool sdhcIsAvailable(void)
{
	return context.available;
}

void sdhcInitializationClocks(void)
{
	// Send initialization clocks to the SD card
	uint32_t timeout = SDHC_RESET_TIMEOUT;
	bool forceExit = false;
	SDHC->SYSCTL |= SDHC_SYSCTL_INITA_MASK;
	while (!forceExit && (SDHC->SYSCTL & SDHC_SYSCTL_INITA_MASK))
	{
		if (timeout)
		{
			timeout--;
		}
		else
		{
			forceExit = true;
		}
	}
}

bool sdhcIsCardInserted(void)
{
	return context.cardStatus;
}

bool sdhcIsTransferComplete(void)
{
	bool status = context.transferCompleted;
	if (status)
	{
		context.transferCompleted = false;
	}
	return status;
}

bool sdhcStartTransfer(sdhc_command_t* command, sdhc_data_t* data)
{
	bool 		successful = false;
	uint32_t	flags = 0;

	if (sdhcIsAvailable())
	{
		if (!(SDHC->PRSSTAT & SDHC_PRSSTAT_CDIHB_MASK) && !(SDHC->PRSSTAT & SDHC_PRSSTAT_CIHB_MASK))
		{
			context.currentCommand = command;
			context.currentData = data;
			context.transferedWords = 0;
			context.available = false;
			context.transferCompleted = false;
			context.currentError = SDHC_ERROR_OK;

			if (command)
			{
				// Set the response length expected, and whether index and CCC check is required
				switch (command->responseType)
				{
					case SDHC_RESPONSE_TYPE_NONE:
						flags |= SDHC_RESPONSE_LENGTH_NONE;
						break;
					case SDHC_RESPONSE_TYPE_R1:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R1b:
						flags |= (SDHC_RESPONSE_LENGTH_48BUSY | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R2:
						flags |= (SDHC_RESPONSE_LENGTH_136 | SDHC_COMMAND_CHECK_CCR);
						break;
					case SDHC_RESPONSE_TYPE_R3:
						flags |= (SDHC_RESPONSE_LENGTH_48);
						break;
					case SDHC_RESPONSE_TYPE_R4:
						flags |= (SDHC_RESPONSE_LENGTH_48);
						break;
					case SDHC_RESPONSE_TYPE_R5:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R5b:
						flags |= (SDHC_RESPONSE_LENGTH_48BUSY | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R6:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					case SDHC_RESPONSE_TYPE_R7:
						flags |= (SDHC_RESPONSE_LENGTH_48 | SDHC_COMMAND_CHECK_CCR | SDHC_COMMAND_CHECK_INDEX);
						break;
					default:
						break;
				}

				// Set the command type, index and argument
				flags |= SDHC_XFERTYP_CMDINX(command->index);
				flags |= SDHC_XFERTYP_CMDTYP(command->commandType);
				SDHC->CMDARG = command->argument;
			}

			if (data)
			{
				// Alignment of words, the memory buffer passed by the user must be 4 byte aligned
				if (data->blockSize % sizeof(uint32_t) != 0U)
				{
					data->blockSize += sizeof(uint32_t) - (data->blockSize % sizeof(uint32_t));
				}

				// Set the block size and block count
				SDHC->BLKATTR = SDHC_BLKATTR_BLKCNT(data->blockCount & 0xFFFF) | SDHC_BLKATTR_BLKSIZE(data->blockSize & 0xFFF);

				// Set the data present flag
				flags |= SDHC_XFERTYP_DPSEL_MASK;
				flags |= SDHC_XFERTYP_DTDSEL(data->readBuffer ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_MSBSEL(data->blockCount > 1 ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_AC12EN(data->blockCount > 1 ? 0b1 : 0b0);
				flags |= SDHC_XFERTYP_BCEN(data->blockCount > 1 ? 0b1 : 0b0);
			}

			// Starts the transfer process
			SDHC->XFERTYP = flags;
			successful = true;
		}
	}
	return successful;
}

sdhc_error_t sdhcTransfer(sdhc_command_t* command, sdhc_data_t* data)
{
	sdhc_error_t error = SDHC_ERROR_OK;
	bool forceExit = false;

	if (sdhcStartTransfer(command, data))
	{
		while (!forceExit & !sdhcIsTransferComplete())
		{
			if (sdhcGetErrorStatus() != SDHC_ERROR_OK)
			{
				error = sdhcGetErrorStatus();
				forceExit = true;
			}
		}
	}
	else
	{
		error = SDHC_ERROR_CMD_BUSY;
	}

	return error;
}

void sdhcOnCardInserted(sdhc_callback_t callback)
{
	context.onCardInserted = callback;
}

void sdhcOnCardRemoved(sdhc_callback_t callback)
{
	context.onCardRemoved = callback;
}

void sdhcOnTransferCompleted(sdhc_callback_t callback)
{
	context.onTransferCompleted = callback;
}

void sdhcOnTransferError(sdhc_error_callback_t callback)
{
	context.onTransferError = callback;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void	getSettingsByFrequency(uint32_t frequency, uint8_t* sdclks, uint8_t* dvs)
{
	uint32_t currentFrequency = 0;
	uint32_t currentError = 0;
	uint32_t bestFrequency = 0;
	uint32_t bestError = 0;
	uint16_t prescaler;
	uint16_t divisor;
	for (prescaler = 0x0002 ; prescaler <= 0x0100 ; prescaler = prescaler << 1)
	{
		for (divisor = 1 ; divisor <= 16 ; divisor++)
		{
			currentFrequency = computeFrequency(prescaler, divisor);
			currentError = frequency > currentFrequency ? frequency - currentFrequency : currentFrequency - frequency;
			if ((bestFrequency == 0) || (bestError > currentError))
			{
				bestFrequency = currentFrequency;
				bestError = currentError;
				*sdclks = prescaler >> 1;
				*dvs = divisor - 1;
			}
		}
	}
}

static uint32_t computeFrequency(uint8_t prescaler, uint8_t divisor)
{
	return SDHC_CLOCK_FREQUENCY / (prescaler * divisor);
}

static void contextInit(void)
{
	// Load the peripheral capabilities
	context.lowVoltageSupported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_VS33_MASK ) ? true : false;
	context.suspendResumeSupported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_SRS_MASK ) ? true : false;
	context.dmaSupported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_DMAS_MASK ) ? true : false;
	context.admaSupported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_ADMAS_MASK ) ? true : false;
	context.highSpeedSupported = ( SDHC->HTCAPBLT & SDHC_HTCAPBLT_HSS_MASK ) ? true : false;
	context.cardStatus = gpioRead(SDHC_SWITCH_PIN) == HIGH ? true : false;
	context.transferCompleted = false;
	context.available = true;
	context.alreadyInit = true;
	context.currentError = SDHC_ERROR_OK;
	switch ((SDHC->HTCAPBLT & SDHC_HTCAPBLT_MBL_MASK) >> SDHC_HTCAPBLT_MBL_SHIFT)
	{
		case 0b00:
			context.maxBlockSize = 512;
			break;
		case 0b01:
			context.maxBlockSize = 1024;
			break;
		case 0b10:
			context.maxBlockSize = 2048;
			break;
		case 0b11:
			context.maxBlockSize = 4096;
			break;
	}
}

static void contextSetTransferComplete(void)
{
	context.available = true;

	if (context.onTransferCompleted)
	{
		context.onTransferCompleted();
	}
	else
	{
		context.transferCompleted = true;
	}
}

static uint32_t sdhcReadWord(void)
{
	return SDHC->DATPORT;
}

static void sdhcReadManyWords(uint32_t* buffer, size_t count)
{
	for (size_t index = 0 ; index < count ; index++)
	{
		buffer[index] = sdhcReadWord();
	}
}

static void sdhcWriteWord(uint32_t data)
{
	SDHC->DATPORT = data;
}

static void sdhcWriteManyWords(uint32_t* buffer, size_t count)
{
	for (size_t index = 0 ; index < count ; index++)
	{
		sdhcWriteWord(buffer[index]);
	}
}

static void SDHC_CommandCompletedHandler(uint32_t status)
{
	// The command transfer has been completed, fetch the response if there is one
	if (context.currentCommand->responseType != SDHC_RESPONSE_TYPE_NONE)
	{
		context.currentCommand->response[0] = SDHC->CMDRSP[0];
		context.currentCommand->response[1] = SDHC->CMDRSP[1];
		context.currentCommand->response[2] = SDHC->CMDRSP[2];
		context.currentCommand->response[3] = SDHC->CMDRSP[3] & 0xFFFFFF;
	}

	if (context.currentData == NULL)
	{
		// Notify or raise the transfer completed flag
		contextSetTransferComplete();
	}
}

static void SDHC_TransferCompletedHandler(uint32_t status)
{
	// Notify or raise the transfer completed flag
	contextSetTransferComplete();
}

static void SDHC_DataHandler(uint32_t status)
{
	uint32_t readWords;
	uint32_t totalWords;
	uint32_t watermark;
	bool	 isRead;

	// Determine what kind of operation is being done,
	// and the watermark used for the algorithm run
	isRead = (status & SDHC_IRQSTAT_BRR_MASK) ? true : false;
	watermark = isRead ? context.readWatermarkLevel : context.writeWatermarkLevel;

	// Calculate the total amount of words to be transfered
	totalWords = (context.currentData->blockCount * context.currentData->blockSize) / sizeof(uint32_t);

	// Compute the amount of words to be read/write in the current cycle
	if (watermark >= totalWords)
	{
		readWords = totalWords;
	}
	else if ((watermark < totalWords) && ((totalWords - context.transferedWords) >= watermark))
	{
		readWords = watermark;
	}
	else
	{
		readWords = totalWords - context.transferedWords;
	}

	// Execute the read/write operation
	if (isRead)
	{
		sdhcReadManyWords(context.currentData->readBuffer + context.transferedWords, readWords);
	}
	else
	{
		sdhcWriteManyWords(context.currentData->writeBuffer + context.transferedWords, readWords);
	}
	context.transferedWords += readWords;
}

static void SDHC_CardDetectedHandler(void)
{
	if (gpioRead(SDHC_SWITCH_PIN) == HIGH)
	{
		if (!context.cardStatus)
		{
			context.cardStatus = true;
			if (context.onCardInserted)
			{
				context.onCardInserted();
			}
		}
	}
	else
	{
		if (context.cardStatus)
		{
			context.cardStatus = false;
			if (context.onCardRemoved)
			{
				context.onCardRemoved();
			}
		}
	}
}

static void SDHC_TransferErrorHandler(uint32_t status)
{
	sdhc_error_t error = SDHC_ERROR_OK;

	context.available = true;

	if (status & SDHC_IRQSTAT_DMAE_MASK)
	{
		error |= SDHC_ERROR_DMA;
	}
	if (status & SDHC_IRQSTAT_AC12E_MASK)
	{
		error |= SDHC_ERROR_AUTO_CMD12;
	}
	if (status & SDHC_IRQSTAT_DEBE_MASK)
	{
		error |= SDHC_ERROR_DATA_END;
	}
	if (status & SDHC_IRQSTAT_DCE_MASK)
	{
		error |= SDHC_ERROR_DATA_CRC;
	}
	if (status & SDHC_IRQSTAT_DTOE_MASK)
	{
		error |= SDHC_ERROR_DATA_TIMEOUT;
	}
	if (status & SDHC_IRQSTAT_CIE_MASK)
	{
		error |= SDHC_ERROR_CMD_INDEX;
	}
	if (status & SDHC_IRQSTAT_CEBE_MASK)
	{
		error |= SDHC_ERROR_CMD_END;
	}
	if (status & SDHC_IRQSTAT_CCE_MASK)
	{
		error |= SDHC_ERROR_CMD_CRC;
	}
	if (status & SDHC_IRQSTAT_CTOE_MASK)
	{
		error |= SDHC_ERROR_CMD_TIMEOUT;
	}

	context.currentError = error;
	if (context.onTransferError)
	{
		context.onTransferError(error);
	}
}

/*******************************************************************************
 *******************************************************************************
					    INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

__ISR__ SDHC_IRQHandler(void)
{
	// Get the current status of all interrupt status flags
	uint32_t status = SDHC->IRQSTAT;

	// Dispatches each flag detected
	if (status & SDHC_ERROR_FLAG)
	{
		SDHC_TransferErrorHandler(status & SDHC_ERROR_FLAG);
	}
	else if ((status & SDHC_DATA_TIMEOUT_FLAG) && !(status & SDHC_TRANSFER_COMPLETED_FLAG))
	{
		SDHC_TransferErrorHandler(status & SDHC_ERROR_FLAG);
	}
	else
	{
		if (status & SDHC_DATA_FLAG)
		{
			SDHC_DataHandler(status & SDHC_DATA_FLAG);
		}
		if (status & SDHC_COMMAND_COMPLETED_FLAG)
		{
			SDHC_CommandCompletedHandler(status & SDHC_COMMAND_COMPLETED_FLAG);
		}
		if (status & SDHC_TRANSFER_COMPLETED_FLAG)
		{
			SDHC_TransferCompletedHandler(status & SDHC_TRANSFER_COMPLETED_FLAG);
		}
	}

	// Clear all flags raised when entered the service routine
	SDHC->IRQSTAT = status;
}

/******************************************************************************/
