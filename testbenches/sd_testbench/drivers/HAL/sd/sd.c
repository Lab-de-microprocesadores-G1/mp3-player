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

/* Helpful macros */
#define SD_MASK(base_mask, shift)			((base_mask) << shift)

/* Driver internal constant parameters */
#define SD_VOLTAGE_RETRIES					(1000)	// Retry ACMD41 communication until the card is ready state
#define SD_CHECK_PATTERN					(0xAA)	// Check pattern used for the CMD8 communication
#define SD_CHECK_PATTERN_MASK				(0xFF)	// Check pattern mask
#define SD_BLOCK_LENGTH						512
#define SD_MAX_BUFFER_SIZE					SD_BLOCK_LENGTH

/* Card Status Flags */
#define SD_CARD_STATUS_BLOCK_LEN_ERROR_SHIFT	(29)
#define SD_CARD_STATUS_CURRENT_STATE_SHIFT		(9)
#define SD_CARD_STATUS_READY_FOR_DATA_SHIFT		(8)
#define SD_CARD_STATUS_APP_CMD_SHIFT			(5)

#define SD_CARD_STATUS_BLOCK_LEN_ERROR_MASK		SD_MASK(0x1, SD_CARD_STATUS_BLOCK_LEN_ERROR_SHIFT)
#define SD_CARD_STATUS_CURRENT_STATE_MASK		SD_MASK(0xF, SD_CARD_STATUS_CURRENT_STATE_SHIFT)
#define SD_CARD_STATUS_READY_FOR_DATA_MASK		SD_MASK(0x1, SD_CARD_STATUS_READY_FOR_DATA_SHIFT)
#define SD_CARD_STATUS_APP_CMD_MASK				SD_MASK(0x1, SD_CARD_STATUS_APP_CMD_SHIFT)

/* SD OCR Register Flags */
#define SD_OCR_LOW_VOLTAGE_SHIFT			(7)
#define SD_OCR_27_28_SHIFT					(15)
#define SD_OCR_28_29_SHIFT					(16)
#define SD_OCR_29_30_SHIFT					(17)
#define SD_OCR_30_31_SHIFT					(18)
#define SD_OCR_31_32_SHIFT					(19)
#define SD_OCR_32_33_SHIFT					(20)
#define SD_OCR_33_34_SHIFT					(21)
#define SD_OCR_34_35_SHIFT					(22)
#define SD_OCR_35_36_SHIFT					(23)
#define SD_OCR_CAPACITY_SHIFT				(30)
#define SD_OCR_STATUS_SHIFT					(31)

#define SD_OCR_LOW_VOLTAGE_MASK				SD_MASK(0x1, SD_OCR_LOW_VOLTAGE_SHIFT)
#define SD_OCR_27_28_MASK					SD_MASK(0x1, SD_OCR_27_28_SHIFT)
#define SD_OCR_28_29_MASK					SD_MASK(0x1, SD_OCR_28_29_SHIFT)
#define SD_OCR_29_30_MASK					SD_MASK(0x1, SD_OCR_29_30_SHIFT)
#define SD_OCR_30_31_MASK					SD_MASK(0x1, SD_OCR_30_31_SHIFT)
#define SD_OCR_31_32_MASK					SD_MASK(0x1, SD_OCR_31_32_SHIFT)
#define SD_OCR_32_33_MASK					SD_MASK(0x1, SD_OCR_32_33_SHIFT)
#define SD_OCR_33_34_MASK					SD_MASK(0x1, SD_OCR_33_34_SHIFT)
#define SD_OCR_34_35_MASK					SD_MASK(0x1, SD_OCR_34_35_SHIFT)
#define SD_OCR_35_36_MASK					SD_MASK(0x1, SD_OCR_35_36_SHIFT)
#define SD_OCR_CAPACITY_MASK				SD_MASK(0x1, SD_OCR_CAPACITY_SHIFT)
#define SD_OCR_STATUS_MASK					SD_MASK(0x1, SD_OCR_STATUS_SHIFT)

/* SD CSD Register Flags */
#define SD_CSD_CSD_STRUCTURE_SHIFT			(126)
#define SD_CSD_READ_BL_LEN_SHIFT			(80)
#define SD_CSD_C_SIZE_SHIFT					(62)
#define SD_CSD_C_SIZE_MULT_SHIFT			(47)
#define SD_CSD_WRITE_BL_LEN_SHIFT			(22)
#define SD_CSD_FILE_FORMAT_GRP_SHIFT		(15)
#define SD_CSD_FILE_FORMAT_SHIFT			(10)

#define SD_CSD_CSD_STRUCTURE_BITS			(2)
#define SD_CSD_READ_BL_LEN_BITS				(4)
#define SD_CSD_C_SIZE_BITS					(12)
#define SD_CSD_C_SIZE_MULT_BITS				(3)
#define SD_CSD_WRITE_BL_LEN_BITS			(4)
#define SD_CSD_FILE_FORMAT_GRP_BITS			(1)
#define SD_CSD_FILE_FORMAT_BITS				(2)

#define SD_CSD_OFFSET						(8)

/* SD Set Bus Width Parameters */
#define SD_SET_BUS_WIDTH_1BIT				(0b00)
#define SD_SET_BUS_WIDTH_4BIT				(0b10)

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// SD standard commands
enum {
	SD_CMD_0,
	SD_CMD_1,
	SD_CMD_2,
	SD_CMD_3,
	SD_CMD_4,
	SD_CMD_5,
	SD_CMD_6,
	SD_CMD_7,
	SD_CMD_8,
	SD_CMD_9,
	SD_CMD_10,
	SD_CMD_11,
	SD_CMD_12,
	SD_CMD_13,
	SD_CMD_14,
	SD_CMD_15,
	SD_CMD_16,
	SD_CMD_17,
	SD_CMD_18,
	SD_CMD_19,
	SD_CMD_20,
	SD_CMD_21,
	SD_CMD_22,
	SD_CMD_23,
	SD_CMD_24,
	SD_CMD_25,
	SD_CMD_26,
	SD_CMD_27,
	SD_CMD_28,
	SD_CMD_29,
	SD_CMD_30,
	SD_CMD_31,
	SD_CMD_32,
	SD_CMD_33,
	SD_CMD_34,
	SD_CMD_35,
	SD_CMD_36,
	SD_CMD_37,
	SD_CMD_38,
	SD_CMD_39,
	SD_CMD_40,
	SD_CMD_41,
	SD_CMD_42,
	SD_CMD_43,
	SD_CMD_44,
	SD_CMD_45,
	SD_CMD_46,
	SD_CMD_47,
	SD_CMD_48,
	SD_CMD_49,
	SD_CMD_50,
	SD_CMD_51,
	SD_CMD_52,
	SD_CMD_53,
	SD_CMD_54,
	SD_CMD_55,
	SD_CMD_56,
	SD_CMD_57,
	SD_CMD_58,
	SD_CMD_59,
	SD_CMD_60,
	SD_CMD_61,
	SD_CMD_62,
	SD_CMD_63
};

// SD application commands
enum {
	SD_ACMD_6	=	6,
	SD_ACMD_13	=	13,
	SD_ACMD_22	=	22,
	SD_ACMD_23	=	23,
	SD_ACMD_41	=	41,
	SD_ACMD_42	=	42,
	SD_ACMD_51	=	51
};

// SD standard and application commands using more descriptive
// names, according to the physical layer specification
enum {
	// Standard commands
	SD_GO_IDLE_STATE		=	SD_CMD_0,
	SD_ALL_SEND_CID			=	SD_CMD_2,
	SD_SEND_RELATIVE_ADDR	=	SD_CMD_3,
	SD_SELECT_CARD			=	SD_CMD_7,
	SD_SEND_IF_COND			=	SD_CMD_8,
	SD_SEND_CSD				= 	SD_CMD_9,
	SD_SEND_STATUS			=	SD_CMD_13,
	SD_SET_BLOCKLEN			= 	SD_CMD_16,
	SD_READ_SINGLE_BLOCK	=	SD_CMD_17,
	SD_READ_MULTIPLE_BLOCK	=	SD_CMD_18,
	SD_WRITE_SINGLE_BLOCK	=	SD_CMD_24,
	SD_WRITE_MULTIPLE_BLOCK	=	SD_CMD_25,
	SD_ERASE_WR_BLK_START	=	SD_CMD_32,
	SD_ERASE_WR_BLK_END		=	SD_CMD_33,
	SD_ERASE				=	SD_CMD_38,
	SD_APP_CMD				=	SD_CMD_55,

	// Application commands
	SD_SET_BUS_WIDTH		= 	SD_ACMD_6,
	SD_SD_STATUS			= 	SD_ACMD_13,
	SD_SEND_OP_COND			=	SD_ACMD_41,
	SD_SEND_SCR				=	SD_ACMD_51
};

// SD current state enumeration
typedef enum {
	SD_CURRENT_STATE_IDLE,
	SD_CURRENT_STATE_READY,
	SD_CURRENT_STATE_IDENT,
	SD_CURRENT_STATE_STBY,
	SD_CURRENT_STATE_TRAN,
	SD_CURRENT_STATE_DATA,
	SD_CURRENT_STATE_RCV,
	SD_CURRENT_STATE_PRG,
	SD_CURRENT_STATE_DIS
} sd_card_state_t;

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

	// Card Status Flags
	struct {
		bool 			blockLenError;
		bool 			readyForData;
		bool 			applicationCommand;
		sd_card_state_t currentState;
	} cardStatus;

	// SD Status Flags
	struct {
		uint8_t			busWidth;
		uint8_t			securedMode;
		uint8_t			sdCardType;
		uint8_t			protectedSize;
		uint8_t			speedClass;
		uint8_t			performanceMove;
		uint8_t			auSize;
		uint8_t			eraseSize;
		uint8_t			eraseTimeout;
		uint8_t			eraseOffset;
		uint8_t 		uhsSpeedGrade;
		uint8_t   		uhsAuSize;
	} sdStatus;
} sd_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*
 * @brief Sets the current card status register on the SD context, and updates its
 * 		  internal flags.
 * @param cardStatus	Current card status register value
 */
static void sdContextSetCardStatus(uint32_t cardStatus);

/*
 * @brief Set the current SD status, which is decoded and updated into the internal flags of the SD context.
 * @param sdStatus		Current sd status register value
 */
static void sdContextSetSdStatus(uint32_t* sdStatus);

/*
 * @brief Sends a command to the sd card to receive its card status and updates the
 * 		  internal flags.
 */
static bool sdReadCardStatus(void);

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
			.frequency = SDHC_FREQUENCY_DEFAULT,
			.readWatermarkLevel = 128,
			.writeWatermarkLevel = 128
		};
		sdhcInit(config);

		// Set the already initialized flag, prevents multiple initializations
		context.alreadyInitialized = true;
	}
}

bool sdCardInit(void)
{
	uint16_t attempts = SD_VOLTAGE_RETRIES;
	sdhc_command_t command;
	sdhc_data_t data;
	bool success;

	// Send 80 clocks to the card, to initialize internal operations
	sdhcReset(SDHC_RESET_CMD);
	sdhcInitializationClocks();

	// GO_IDLE_STATE: Send CMD0, to reset all MMC and SD cards.
	success = false;
	command.index = SD_GO_IDLE_STATE;
	command.argument = 0;
	command.commandType = SDHC_COMMAND_TYPE_NORMAL;
	command.responseType = SDHC_RESPONSE_TYPE_NONE;
	if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
	{
		success = true;
	}

	// SEND_IF_COND: Send CMD8, asks the SD card if works with the given voltage range.
	if (success)
	{
		success = false;
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
		}
	}

	// APP_CMD: Send CMD55, tells the SD card that the next command is an application specific command.
	// SD_SEND_OP_COND: Send ACMD41, sends information about the host to the card to match capabilities
	if (success)
	{
		success = false;
		while (!success && attempts--)
		{
			command.index = SD_APP_CMD;
			command.argument = 0;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
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
						success = true;
					}
				}
			}
		}
	}

	// ALL_SEND_CID: Send the CMD2, ask all SD cards to send their CID.
	if (success)
	{
		success = false;
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
			success = true;
		}
	}

	// SEND_RELATIVE_ADDR: Send the CMD3, ask the card to publish its relative address
	if (success)
	{
		success = false;
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
	}

	// SEND_CSD: Send the CMD9, ask the card to publish its specific data
	if (success)
	{
		success = false;
		command.index = SD_SEND_CSD;
		command.argument = context.rca << SDHC_R6_RCA_SHIFT;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R2;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			// Storing in the internal context, the CSD register of the SD card connected
			context.csd[0] = command.response[0];
			context.csd[1] = command.response[1];
			context.csd[2] = command.response[2];
			context.csd[3] = command.response[3];
			success = true;
		}
	}

	// SELECT_CARD: Send the CMD7, tell to switch from Stand-By to Transfer State
	if (success)
	{
		success = false;
		command.index = SD_SELECT_CARD;
		command.argument = context.rca << SDHC_R6_RCA_SHIFT;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1b;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			if (((command.response[0] & SD_CARD_STATUS_CURRENT_STATE_MASK) >> SD_CARD_STATUS_CURRENT_STATE_SHIFT) == SD_CURRENT_STATE_STBY)
			{
				// Switch peripheral CLK frequency to 25MHz
				sdhcSetClock(SDHC_FREQUENCY_TYP);
				success = true;
			}
		}
	}

	// APP_CMD: Send CMD55, tells the SD card that the next command is an application specific command.
	// SEND_SCR: Send the ACMD51, ask the card to publish its specific data
	if (success)
	{
		success = false;
		command.index = SD_APP_CMD;
		command.argument = context.rca << SDHC_R6_RCA_SHIFT;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			command.index = SD_SEND_SCR;
			command.argument = context.rca << SDHC_R6_RCA_SHIFT;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			data.blockCount = 1;
			data.blockSize = 8;
			data.readBuffer = context.buffer;
			data.writeBuffer = NULL;
			if (sdhcTransfer(&command, &data) == SDHC_ERROR_OK)
			{
				// Read SCR value
				context.scr[0] = context.buffer[0];
				context.scr[1] = context.buffer[1];
				success = true;
			}
		}
	}

	// SET_BUS_WIDTH: Send the ACMD6, sets the bus width used in the DAT line
	if (success)
	{
		success = false;
		command.index = SD_APP_CMD;
		command.argument = context.rca << SDHC_R6_RCA_SHIFT;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			command.index = SD_SET_BUS_WIDTH;
			command.argument = SD_SET_BUS_WIDTH_4BIT;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
			{
				// Switch SDHC peripheral bus width
				sdhcSetBusWidth(SDHC_DATA_WIDTH_4BIT);
				success = true;
			}
		}
	}

	// SET_BLOCKLEN: Send the CMD16, sets the block length
	if (success)
	{
		success = false;
		command.index = SD_SET_BLOCKLEN;
		command.argument = SD_BLOCK_LENGTH;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			success = true;
		}
	}

	// SEND_STATUS: Send CMD13, asks the sd card to send its card status register.
	if (success)
	{
		success = false;
		command.index = SD_SEND_STATUS;
		command.argument = context.rca << SDHC_R6_RCA_SHIFT;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			sdContextSetCardStatus(command.response[0]);
			if (context.cardStatus.readyForData)
			{
				success = true;
			}
		}
	}

	// APP_CMD: Send CMD55, tells the SD card that the next command is an application specific command.
	// SD_STATUS: Send ACMD13, asks the SD card to send its sd status
	if (success)
	{
		success = false;
		command.index = SD_APP_CMD;
		command.argument = context.rca << SDHC_R6_RCA_SHIFT;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			command.index = SD_SD_STATUS;
			command.argument = context.rca << SDHC_R6_RCA_SHIFT;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			data.blockCount = 1;
			data.blockSize = 64;
			data.readBuffer = context.buffer;
			data.writeBuffer = NULL;
			if (sdhcTransfer(&command, &data) == SDHC_ERROR_OK)
			{
				sdContextSetCardStatus(command.response[0]);
				sdContextSetSdStatus(context.buffer);
				success = true;
			}
		}
	}

	return success;
}

bool sdRead(uint32_t* readBuffer, uint32_t blockNumber, uint32_t blockCount)
{
	uint32_t recvBlocksCount;
	sdhc_command_t command;
	sdhc_data_t data;
	bool success = true;

	// Compute the address of the starting block
	uint32_t blockAddress = blockNumber * SD_BLOCK_LENGTH;

	// Get the maximum block length supported but either the SD card or the
	// lower layer of software/hardware. The maximum block count is measured
	// as the maximum quantity of blocks of SD_BLOCK_LENGTH bytes allowed.
	uint32_t maximumBlockCount = sdGetMaximumReadBlockLength() < sdhcGetMaximumBlockCount() ? sdGetMaximumReadBlockLength() : sdhcGetMaximumBlockCount();
	maximumBlockCount = maximumBlockCount / SD_BLOCK_LENGTH;

	// Iterate sending blocks using the maximum amount available in the
	// lower layer of software and hardware, until it has sent all blocks.
	while (blockCount && success)
	{
		// Wait until the SD cards enters to the ready for data
		// by updating Card Status register and querying it via the command line
		do
		{
			if (!sdReadCardStatus())
			{
				success = false;
			}
		} while(success && !context.cardStatus.readyForData);

		// Compute the amount of blocks to be sent in the current loop
		if (blockCount > maximumBlockCount)
		{
			recvBlocksCount = maximumBlockCount;
		}
		else
		{
			recvBlocksCount = blockCount;
		}

		// If succeeds entering the ready for data status,
		// performs a single or multiple read
		if (success)
		{
			success = false;
			command.index = blockCount == 1 ? SD_READ_SINGLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
			command.argument = blockAddress;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			data.readBuffer = readBuffer;
			data.writeBuffer = NULL;
			data.blockSize = SD_BLOCK_LENGTH;
			data.blockCount = recvBlocksCount;
			if (sdhcTransfer(&command, &data) == SDHC_ERROR_OK)
			{
				// Move the read buffer to the next block position, and change the
				// next address of memory to be read in the memory map of the sd card
				readBuffer = readBuffer + recvBlocksCount * SD_BLOCK_LENGTH / sizeof(uint32_t);
				blockAddress = blockAddress + recvBlocksCount * SD_BLOCK_LENGTH;
				blockCount -= recvBlocksCount;
				success = true;
			}
		}
	}

	return success;
}

bool sdWrite(uint32_t* writeBuffer, uint32_t blockNumber, uint32_t blockCount)
{
	uint32_t sendBlocksCount;
	sdhc_command_t command;
	sdhc_data_t data;
	bool success = true;

	// Compute the address of the starting block
	uint32_t blockAddress = blockNumber * SD_BLOCK_LENGTH;

	// Get the maximum block length supported but either the SD card or the
	// lower layer of software/hardware. The maximum block count is measured
	// as the maximum quantity of blocks of SD_BLOCK_LENGTH bytes allowed.
	uint32_t maximumBlockCount = sdGetMaximumWriteBlockLength() < sdhcGetMaximumBlockCount() ? sdGetMaximumWriteBlockLength() : sdhcGetMaximumBlockCount();
	maximumBlockCount = maximumBlockCount / SD_BLOCK_LENGTH;

	// Iterate sending blocks using the maximum amount available in the
	// lower layer of software and hardware, until it has sent all blocks.
	while (blockCount && success)
	{
		// Wait until the SD cards enters to the ready for data
		// by updating Card Status register and querying it via the command line
		do
		{
			if (!sdReadCardStatus())
			{
				success = false;
			}
		} while(success && !context.cardStatus.readyForData);

		// Compute the amount of blocks to be sent in the current loop
		if (blockCount > maximumBlockCount)
		{
			sendBlocksCount = maximumBlockCount;
		}
		else
		{
			sendBlocksCount = blockCount;
		}

		// If succeeds entering the ready for data status,
		// performs a single or multiple write
		if (success)
		{
			success = false;
			command.index = blockCount == 1 ? SD_WRITE_SINGLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
			command.argument = blockAddress;
			command.commandType = SDHC_COMMAND_TYPE_NORMAL;
			command.responseType = SDHC_RESPONSE_TYPE_R1;
			data.readBuffer = NULL;
			data.writeBuffer = writeBuffer;
			data.blockSize = SD_BLOCK_LENGTH;
			data.blockCount = sendBlocksCount;
			if (sdhcTransfer(&command, &data) == SDHC_ERROR_OK)
			{
				// Move the write buffer to the next block position, and change the
				// next address of memory to be read in the memory map of the sd card
				writeBuffer = writeBuffer + sendBlocksCount * SD_BLOCK_LENGTH / sizeof(uint32_t);
				blockAddress = blockAddress + sendBlocksCount * SD_BLOCK_LENGTH;
				blockCount -= sendBlocksCount;
				success = true;
			}
		}
	}

	return success;
}

bool sdErase(uint32_t blockNumber, uint32_t blockCount)
{
	bool success = true;
	sdhc_command_t command;

	// Compute the address of the starting block
	uint32_t blockAddress = blockNumber * SD_BLOCK_LENGTH;

	// Wait until the SD cards enters to the ready for data
	// by updating Card Status register and querying it via the command line
	do
	{
		if (!sdReadCardStatus())
		{
			success = false;
		}
	} while(success && !context.cardStatus.readyForData);

	// Performs a ERASE_WR_BLK_START, sets the start address for the erasing process
	if (success)
	{
		success = false;
		command.index = SD_ERASE_WR_BLK_START;
		command.argument = blockAddress;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			success = true;
		}
	}

	// Performs a ERASE_WR_BLK_END, sets the end address for the erasing process
	if (success)
	{
		success = false;
		command.index = SD_ERASE_WR_BLK_END;
		command.argument = blockAddress + (blockCount - 1) * SD_BLOCK_LENGTH;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			success = true;
		}
	}

	// Performs a ERASE, starts the erasing process
	if (success)
	{
		success = false;
		command.index = SD_ERASE;
		command.argument = 0;
		command.commandType = SDHC_COMMAND_TYPE_NORMAL;
		command.responseType = SDHC_RESPONSE_TYPE_R1b;
		if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
		{
			// Wait the process to be finished
			success = true;
			do
			{
				if (!sdReadCardStatus())
				{
					success = false;
				}
			} while(success && !context.cardStatus.readyForData);
		}
	}

	return success;
}

bool sdIsCardInserted(void)
{
	return sdhcIsCardInserted();
}

uint64_t sdGetSize(void)
{
	uint64_t blockLen;
	uint64_t blockCount;
	uint64_t cSizeMult;
	uint64_t cSize;
	uint64_t mult;

	cSize = ((context.csd[1]  >> (SD_CSD_C_SIZE_SHIFT - SD_CSD_OFFSET - 1 * 32)) & 0x3FF) | ((context.csd[2] & 0x3) << 10);
	cSizeMult = (context.csd[1]  >> (SD_CSD_C_SIZE_MULT_SHIFT - SD_CSD_OFFSET - 1 * 32)) & 0x7;

	mult = 1 << (2 + cSizeMult);
	blockLen = sdGetMaximumReadBlockLength();
	blockCount = (cSize + 1) * mult;

	return blockLen * blockCount;
}

sd_file_format_t sdGetFileFormat(void)
{
	sd_file_format_t result;
	uint8_t fileFormatGroup;
	uint8_t fileFormat;

	fileFormatGroup = ((context.csd[0] >> (SD_CSD_FILE_FORMAT_GRP_SHIFT - SD_CSD_OFFSET)) & 0x1);
	fileFormat = (context.csd[0] >> (SD_CSD_FILE_FORMAT_SHIFT - SD_CSD_OFFSET)) & 0x3;

	if (fileFormatGroup)
	{
		result = SD_FF_RESERVED;
	}
	else
	{
		result = fileFormat;
	}

	return result;
}

uint16_t sdGetMaximumReadBlockLength(void)
{
	return 1 << ((context.csd[2]  >> (SD_CSD_READ_BL_LEN_SHIFT - SD_CSD_OFFSET - 2 * 32)) & 0xF);
}

uint16_t sdGetMaximumWriteBlockLength(void)
{
	return 1 << ((context.csd[0] >> (SD_CSD_WRITE_BL_LEN_SHIFT - SD_CSD_OFFSET - 0 * 32)) & 0xF);
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

static void sdContextSetCardStatus(uint32_t cardStatus)
{
	context.cardStatus.applicationCommand = (cardStatus & SD_CARD_STATUS_APP_CMD_MASK) ? true : false;
	context.cardStatus.blockLenError = (cardStatus & SD_CARD_STATUS_BLOCK_LEN_ERROR_MASK) ? true : false;
	context.cardStatus.readyForData = (cardStatus & SD_CARD_STATUS_READY_FOR_DATA_MASK) ? true : false;
	context.cardStatus.currentState = (cardStatus & SD_CARD_STATUS_CURRENT_STATE_MASK) >> SD_CARD_STATUS_CURRENT_STATE_SHIFT;
}

static void sdContextSetSdStatus(uint32_t* sdStatus)
{
    context.sdStatus.busWidth = (uint8_t)((sdStatus[0U] & 0xC0000000U) >> 30U);                                       		/* 511-510 */
    context.sdStatus.securedMode = (uint8_t)((sdStatus[0U] & 0x20000000U) >> 29U);                                     		/* 509 */
    context.sdStatus.sdCardType = (uint16_t)((sdStatus[0U] & 0x0000FFFFU));                                             		/* 495-480 */
    context.sdStatus.protectedSize = sdStatus[1U];                                                                    		/* 479-448 */
    context.sdStatus.speedClass = (uint8_t)((sdStatus[2U] & 0xFF000000U) >> 24U);                                     		/* 447-440 */
    context.sdStatus.performanceMove = (uint8_t)((sdStatus[2U] & 0x00FF0000U) >> 16U);                                		/* 439-432 */
    context.sdStatus.auSize = (uint8_t)((sdStatus[2U] & 0x0000F000U) >> 12U);                                         		/* 431-428 */
    context.sdStatus.eraseSize = (uint16_t)(((sdStatus[2U] & 0x000000FFU) << 8U) | ((sdStatus[3U] & 0xFF000000U) >> 24U)); 	/* 423-408 */
    context.sdStatus.eraseTimeout = (((uint8_t)((sdStatus[3U] & 0x00FF0000U) >> 16U)) & 0xFCU) >> 2U;                 		/* 407-402 */
    context.sdStatus.eraseOffset = ((uint8_t)((sdStatus[3U] & 0x00FF0000U) >> 16U)) & 0x3U;                           		/* 401-400 */
    context.sdStatus.uhsSpeedGrade = (((uint8_t)((sdStatus[3U] & 0x0000FF00U) >> 8U)) & 0xF0U) >> 4U;                 		/* 399-396 */
    context.sdStatus.uhsAuSize = ((uint8_t)((sdStatus[3U] & 0x0000FF00U) >> 8U)) & 0xFU;                              		/* 395-392 */
}

static bool sdReadCardStatus(void)
{
	sdhc_command_t command;
	bool success = false;
	command.index = SD_SEND_STATUS;
	command.argument = context.rca << SDHC_R6_RCA_SHIFT;
	command.commandType = SDHC_COMMAND_TYPE_NORMAL;
	command.responseType = SDHC_RESPONSE_TYPE_R1;
	if (sdhcTransfer(&command, NULL) == SDHC_ERROR_OK)
	{
		sdContextSetCardStatus(command.response[0]);
		success = true;
	}
	return success;
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
