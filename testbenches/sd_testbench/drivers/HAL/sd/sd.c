/*******************************************************************************
  @file     sd.c
  @brief    SD card driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/MCAL/sdhc/sdhc.h"
#include "sd.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/* Driver internal constant parameters */
#define SD_VOLTAGE_RETRIES					(1000)	// Retry ACMD41 communication until the card is ready state
#define SD_CHECK_PATTERN					(0xAA)	// Check pattern used for the CMD8 communication
#define SD_CHECK_PATTERN_MASK				(0xFF)	// Check pattern mask
#define SD_MAX_BUFFER_SIZE					512

/* Card Status Flags */
#define SD_CARD_STATUS_APP_CMD_SHIFT		(5)
#define SD_CARD_STATUS_READY_FOR_DATA_SHIFT	(8)
#define SD_CARD_STATUS_CURRENT_STATE_SHIFT	(9)

#define SD_CARD_STATUS_APP_CMD_MASK			(0x1 << SD_CARD_STATUS_APP_CMD_SHIFT)
#define SD_CARD_STATUS_READY_FOR_DATA_MASK	(0x1 << SD_CARD_STATUS_READY_FOR_DATA_SHIFT)
#define SD_CARD_STATUS_CURRENT_STATE_MASK	(0xF << SD_CARD_STATUS_CURRENT_STATE_SHIFT)


/* SD OCR Register Flags */
#define SD_OCR_LOW_VOLTAGE_MASK				(0x1 << 7)
#define SD_OCR_27_28_MASK					(0x1 << 15)
#define SD_OCR_28_29_MASK					(0x1 << 16)
#define SD_OCR_29_30_MASK					(0x1 << 17)
#define SD_OCR_30_31_MASK					(0x1 << 18)
#define SD_OCR_31_32_MASK					(0x1 << 19)
#define SD_OCR_32_33_MASK					(0x1 << 20)
#define SD_OCR_33_34_MASK					(0x1 << 21)
#define SD_OCR_34_35_MASK					(0x1 << 22)
#define SD_OCR_35_36_MASK					(0x1 << 23)
#define SD_OCR_CAPACITY_MASK				(0x1 << 30)
#define SD_OCR_STATUS_MASK					(0x1 << 31)

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// SD current state enumeration
enum {
	SD_CURRENT_STATE_IDLE,
	SD_CURRENT_STATE_READY,
	SD_CURRENT_STATE_IDENT,
	SD_CURRENT_STATE_STBY,
	SD_CURRENT_STATE_TRAN,
	SD_CURRENT_STATE_DATA,
	SD_CURRENT_STATE_RCV,
	SD_CURRENT_STATE_PRG,
	SD_CURRENT_STATE_DIS
};

// SD driver context variables
typedef struct {
	// Registers
	uint16_t		rca;
	uint32_t		ocr;
	uint32_t		cid[4];
	uint32_t 		csd[4];
	uint32_t		scr[2];

	// Buffer for read/write
	uint32_t		buffer[SD_MAX_BUFFER_SIZE];

	// Flags
	bool			alreadyInitialized;
} sd_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*
 * @brief SD wrapper for blocking command transfer
 * @param index			Command index
 * @param argument		Command argument
 * @param commandType	Command type
 * @param responseType	Response type
 * @param response		Pointer to the response
 */
static sdhc_error_t sdCommandTransfer(uint8_t index, uint32_t argument, sdhc_command_type_t commandType, sdhc_response_type_t responseType, uint32_t* response);

/*
 * @brief SD wrapper for blocking application command transfer
 * @param index			Command index
 * @param argument		Command argument
 * @param commandType	Command type
 * @param responseType	Response type
 * @param response		Pointer to the response
 */
static sdhc_error_t sdAppCommandTransfer(uint8_t index, uint32_t argument, sdhc_command_type_t commandType, sdhc_response_type_t responseType, uint32_t* response);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static sd_context_t context;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void sdInit(void)
{
	if (!context.alreadyInitialized)
	{
		// SDHC driver initialization
		sdhc_config_t config = {
			.frequency = 400000,
			.readWatermarkLevel = 64,
			.writeWatermarkLevel = 64
		};
		sdhcInit(config);

		// Set the already initialized flag, prevents multiple initializations
		context.alreadyInitialized = true;
	}
}

bool sdCardInit(void)
{
	uint16_t attempts = SD_VOLTAGE_RETRIES;
	bool isReadyState = false;
	uint32_t response[4];
	bool success;

	// Send 80 clocks to the card, to initialize internal operations
	sdhcReset(SDHC_RESET_CMD);
	sdhcInitializationClocks();

	// GO_IDLE_STATE: Send CMD0, to reset all MMC and SD cards.
	success = (sdCommandTransfer(SD_GO_IDLE_STATE, 0, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_NONE, response) == SDHC_ERROR_OK);

	// SEND_IF_COND: Send CMD8, asks the SD card if works with the given voltage range.
	if (success)
	{
		success = false;
		if (sdCommandTransfer(SD_SEND_IF_COND, 0x100 | SD_CHECK_PATTERN, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R7, response) == SDHC_ERROR_OK)
		{
			if ((response[0] & SD_CHECK_PATTERN_MASK) == SD_CHECK_PATTERN)
			{
				success = true;
			}
		}
	}

	// SD_SEND_OP_COND: Send ACMD41, sends information about the host to the card to match capabilities
	if (success)
	{
		while (!isReadyState && attempts--)
		{
			success = false;
			if (sdAppCommandTransfer(SD_SEND_OP_COND, SD_OCR_28_29_MASK | SD_OCR_32_33_MASK | SD_OCR_33_34_MASK, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R3, response) == SDHC_ERROR_OK)
			{
				if (response[0] & SD_OCR_STATUS_MASK)
				{
					// Storing in the internal context, the OCR register of the SD card connected
					context.ocr = response[0];
					isReadyState = true;
					success = true;
				}
			}
		}
	}

	// ALL_SEND_CID: Send the CMD2, ask all SD cards to send their CID.
	if (success)
	{
		success = false;
		if (sdCommandTransfer(SD_ALL_SEND_CID, 0, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R2, response) == SDHC_ERROR_OK)
		{
			// Storing in the internal context, the CID register of the SD card connected
			context.cid[0] = response[0];
			context.cid[1] = response[1];
			context.cid[2] = response[2];
			context.cid[3] = response[3];

			success = true;
		}
	}

	// SEND_RELATIVE_ADDR: Send the CMD3, ask the card to publish its relative address
	if (success)
	{
		success = false;
		if (sdCommandTransfer(SD_SEND_RELATIVE_ADDR, 0, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R6, response) == SDHC_ERROR_OK)
		{
			// Storing in the internal context, the RCA register of the SD card connected
			context.rca = (response[0] & SDHC_R6_RCA_MASK) >> SDHC_R6_RCA_SHIFT;
			success = true;
		}
	}

	// SEND_CSD: Send the CMD9, ask the card to publish its specific data
	if (success)
	{
		success = false;

		if (sdCommandTransfer(SD_SEND_CSD, rca << SDHC_R6_RCA_SHIFT, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R2, response) == SDHC_ERROR_OK)
		{
			// Storing in the internal context, the CSD register of the SD card connected
			context.csd[0] = response[0];
			context.csd[1] = response[1];
			context.csd[2] = response[2];
			context.csd[3] = response[3];
			success = true;
		}
	}

	// SELECT_CARD: Send the CMD7, tell to switch from Stand-By to Transfer State
	if (success)
	{
		success = false;

		if (sdCommandTransfer(SD_SELECT_CARD, rca << SDHC_R6_RCA_SHIFT, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R1b, response) == SDHC_ERROR_OK)
		{
			// Check card is in Transfer State
			if (((response[0] & SD_CARD_STATUS_CURRENT_STATE_MASK) >> SD_CARD_STATUS_CURRENT_STATE_SHIFT) == SD_CURRENT_STATE_TRAN)
			{
				success = true;
			}
		}
	}

	// APP_CMD: Send CMD55, tells the SD card that the next command is an application specific
	// command.
	// SEND_SCR: Send the ACMD51, ask the card to publish its specific data
	if (success)
	{
		success = false;

		// Switch peripheral CLK frequency to 25MHz
		sdhcSetClock(SDHC_FREQUENCY_TYP);

		if (sdCommandTransfer(SD_APP_CMD, rca << SDHC_R6_RCA_SHIFT, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R1, response) == SDHC_ERROR_OK)
		{
			sdhc_command_t command;
			sdhc_data_t	data;

			command.index = SD_SEND_SCR;
			command.argument = rca << SDHC_R6_RCA_SHIFT;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			data.blockCount = 1;
			data.blockSize = 8;
			data.readBuffer = context.buffer;
			data.writeBuffer = NULL;
			if (sdhcTransfer(&command, &data) == SDHC_ERROR_OK)
			{
				// Read SCR value
				scr[0] = buffer[0];
				scr[1] = buffer[1];

				success = true;
			}
		}

		// If 4 bit bus operation is valid, switch bus width

		// Switch SDHC peripheral bus width

		// Set block size 512

		// Read status
	}



	}

	return success;
}

bool sdIsCardInserted(void)
{
	return sdhcIsCardInserted();
}

void sdOnCardInserted(sd_callback_t callback)
{
	sdhcOnCardInserted(callback);
}

void sdOnCardRemoved(sd_callback_t callback)
{
	sdhcOnCardRemoved(callback);
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static sdhc_error_t sdCommandTransfer(uint8_t index, uint32_t argument, sdhc_command_type_t commandType, sdhc_response_type_t responseType, uint32_t* response)
{
	// Build the command structure
	sdhc_command_t command = {
		.index = index,
		.argument = argument,
		.commandType = commandType,
		.responseType = responseType
	};

	// Blocking transfer
	sdhc_error_t error = sdhcTransfer(&command, NULL);

	// Copy the response in the given pointer
	*(response + 0) = command.response[0];
	*(response + 1) = command.response[1];
	*(response + 2) = command.response[2];
	*(response + 3) = command.response[3];

	// Return the error
	return error;
}

static sdhc_error_t sdAppCommandTransfer(uint8_t index, uint32_t argument, sdhc_command_type_t commandType, sdhc_response_type_t responseType, uint32_t* response)
{
	sdhc_error_t error = sdCommandTransfer(SD_APP_CMD, 0, SDHC_COMMAND_TYPE_NORMAL, SDHC_RESPONSE_TYPE_R1, response);
	if (error == SDHC_ERROR_OK)
	{
		if (response[0] & SD_CARD_STATUS_APP_CMD_MASK)
		{
			error = sdCommandTransfer(index, argument, commandType, responseType, response);
		}
	}
	return error;
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
