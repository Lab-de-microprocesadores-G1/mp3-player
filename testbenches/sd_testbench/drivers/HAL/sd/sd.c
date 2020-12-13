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

/* Card Status Flags */
#define SD_CARD_STATUS_APP_CMD_MASK			(0x20)
#define SD_CARD_sTATUS_READY_FOR_DATA_MASK	(0x100)

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

// SD driver context variables
typedef struct {
	// Registers
	uint16_t		rca;
	uint32_t		ocr;
	uint32_t		cid[4];

	// Flags
	bool			alreadyInitialized;
} sd_context_t;

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
	bool success = true;
	bool isReadyState = false;
	uint16_t attempts = SD_VOLTAGE_RETRIES;
	sdhc_command_t command;

	// Send 80 clocks to the card, to initialize internal operations
	sdhcInitializationClocks();

	// GO_IDLE_STATE: Send CMD0, to reset all MMC and SD cards.
	if (success)
	{
		command.index = SD_GO_IDLE_STATE;
		command.argument = 0;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_NONE;
		success = (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK);
	}

	// SEND_IF_COND: Send CMD8, asks the SD card if works with the given voltage range.
	if (success)
	{
		command.index = SD_SEND_IF_COND;
		command.argument = 0x100 | SD_CHECK_PATTERN;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R7;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			if ((command.response[0] & SD_CHECK_PATTERN_MASK) == SD_CHECK_PATTERN)
			{
				success = true;
			}
			else
			{
				success = false;
			}
		}
		else
		{
			success = false;
		}
	}

	// APP_CMD: Send CMD55, tells the SD card that the next command is an application specific
	// command. For initialization process a default RCA is used, 0x0000.
	if (success)
	{
		while (!isReadyState && attempts--)
		{
			command.index = SD_APP_CMD;
			command.argument = 0;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
			{
				if (command.response[0] & SD_CARD_STATUS_APP_CMD_MASK)
				{
					success = true;
				}
				else
				{
					success = false;
				}
			}
			else
			{
				success = false;
			}

			// SD_SEND_OP_COND: Send ACMD41, sends information about the host to the card to match capabilities
			if (success)
			{
				command.index = SD_SEND_OP_COND;
				command.argument = SD_OCR_28_29_MASK | SD_OCR_32_33_MASK | SD_OCR_33_34_MASK;
				command.commandType = SDHC_COMMAND_TYPE_NORMAL;
				command.responseType = SDHC_RESPONSE_TYPE_R3;
				if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
				{
					if (command.response[0] & SD_OCR_STATUS_MASK)
					{
						// Storing in the internal context, the OCR register of the SD card connected
						context.ocr = command.response[0];
						isReadyState = true;
						success = true;
					}
					else
					{
						success = false;
					}
				}
				else
				{
					success = false;
				}
			}
		}
	}

	if (success & isReadyState)
	{
		// ALL_SEND_CID: Send the CMD2, ask all SD cards to send their CID.
		command.index = SD_ALL_SEND_CID;
		command.argument = 0;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R2;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			// Storing in the internal context, the CID register of the SD card connected
			context.cid[0] = command.response[0];
			context.cid[1] = command.response[1];
			context.cid[2] = command.response[2];
			context.cid[3] = command.response[3];

			// SEND_RELATIVE_ADDR: Send the CMD3, ask the card to publish its relative address
			command.index = SD_SEND_RELATIVE_ADDR;
			command.argument = 0;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R6;
			if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
			{
				// Storing in the internal context, the RCA register of the SD card connected
				context.rca = (command.response[0] & SDHC_R6_RCA_MASK) >> SDHC_R6_RCA_SHIFT;
				success = true;
			}
			else
			{
				success = false;
			}
		}
		else
		{
			success = false;
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

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
