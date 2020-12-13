/*******************************************************************************
  @file     sdhc.h
  @brief    Secure Digital Host Controller peripheral driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_SDHC_SDHC_H_
#define MCAL_SDHC_SDHC_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// SD bus standard maximum and typical values of clocking frequency
#define SDHC_FREQUENCY_DEFAULT	(400000U)	// fdefault = 400kHz
#define SDHC_FREQUENCY_TYP		(25000000U)	// ftyp = 25MHz
#define SDHC_FREQUENCY_MAX		(50000000U)	// fmax = 50MHz

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
	// Specific errors, driver raises an individual flag for each one
	SDHC_ERROR_OK			= 0x00000000,
	SDHC_ERROR_DMA 			= 0x00000001,
	SDHC_ERROR_AUTO_CMD12 	= 0x00000002,
	SDHC_ERROR_DATA_END		= 0x00000004,
	SDHC_ERROR_DATA_CRC		= 0x00000008,
	SDHC_ERROR_DATA_TIMEOUT	= 0x00000010,
	SDHC_ERROR_CMD_INDEX	= 0x00000020,
	SDHC_ERROR_CMD_END		= 0x00000040,
	SDHC_ERROR_CMD_CRC		= 0x00000080,
	SDHC_ERROR_CMD_TIMEOUT	= 0x00000100,
	SDHC_ERROR_CMD_BUSY		= 0x00000200,

	// Some errors can be grouped
	SDHC_ERROR_DATA			= (SDHC_ERROR_DATA_END | SDHC_ERROR_DATA_CRC | SDHC_ERROR_DATA_TIMEOUT),
	SDHC_ERROR_CMD			= (SDHC_ERROR_CMD_INDEX | SDHC_ERROR_CMD_END | SDHC_ERROR_CMD_CRC | SDHC_ERROR_CMD_TIMEOUT | SDHC_ERROR_CMD_BUSY)
} sdhc_error_t;

typedef enum {
	SDHC_COMMAND_TYPE_NORMAL,
	SDHC_COMMAND_TYPE_SUSPEND,
	SDHC_COMMAND_TYPE_RESUME,
	SDHC_COMMAND_TYPE_ABORT
} sdhc_command_type_t;

typedef enum {
	SDHC_RESPONSE_TYPE_NONE,
	SDHC_RESPONSE_TYPE_R1,
	SDHC_RESPONSE_TYPE_R1b,
	SDHC_RESPONSE_TYPE_R2,
	SDHC_RESPONSE_TYPE_R3,
	SDHC_RESPONSE_TYPE_R4,
	SDHC_RESPONSE_TYPE_R5,
	SDHC_RESPONSE_TYPE_R5b,
	SDHC_RESPONSE_TYPE_R6,
	SDHC_RESPONSE_TYPE_R7
} sdhc_response_type_t;

typedef struct {
	uint8_t					index;			// Index of the command
	uint32_t				argument;		// Argument of the command
	sdhc_command_type_t		commandType;	// Type of command
	sdhc_response_type_t	responseType;	// Type of response expected
	uint32_t				response[4];	// Response placeholder
} sdhc_command_t;

typedef struct {
	uint32_t				blockCount;		// Amount of blocks to be sent or received
	uint32_t				blockSize;		// Size in bytes of each block transfered
	uint32_t*				writeBuffer;	// Buffer with write data, used only when writing, else should be NULL
	uint32_t*				readBuffer;		// Buffer for the read data, used only when reading, else should be NULL
} sdhc_data_t;

typedef struct {
	uint32_t	frequency;					// Frequency for the SD bus clock
	uint8_t		readWatermarkLevel;			// Maximum value of watermark available is 128
	uint8_t		writeWatermarkLevel;		// for both read and write transfer processes.
} sdhc_config_t;

typedef void (*sdhc_callback_t)			(void);
typedef void (*sdhc_error_callback_t)	(sdhc_error_t error);

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/***************
 * GENERAL API *
 **************/

/*
 * @brief Initializes the Secure Digital Host Controller driver.
 * @param config		Configuration of the SDHC driver
 */
void sdhcInit(sdhc_config_t config);

/*
 * @brief Resets the Secure Digital Host Controller driver.
 */
void sdhcReset(void);

/*
 * @brief Changes the clock's frequency used for the clock line in the SD bus communication
 * @param frequency		New target frequency
 */
void sdhcSetClock(uint32_t frequency);

/*
 * @brief Returns the current error status of the Secure Digital Host Controller driver.
 */
sdhc_error_t sdhcGetErrorStatus(void);

/*
 * @brief Returns whether the Secure Digital Host Controller driver is available or not.
 */
bool sdhcIsAvailable(void);

/*
 * @brief Sends 80 clocks via the SD bus clock, used to initialize as active the device.
 */
void sdhcInitializationClocks(void);

/*
 * @brief Returns whether a card is inserted or not.
 */
bool sdhcIsCardInserted(void);

/**
 * @brief Returns whether the last transfer process was completed
 */
bool sdhcIsTransferComplete(void);

/****************
 * TRANSFER API *
 ***************/

/*
 * @brief Starts a transfer process.
 * @param command	Pointer to command structure
 * @param data		Pointer to data structure
 * @returns			Whether it could start the transfer process or not
 */
bool sdhcStartTransfer(sdhc_command_t* command, sdhc_data_t* data);

/*
 * @brief Run a blocking transfer process.
 * @param command	Pointer to the command structure
 * @param data		Pointer to data structure
 * @returns			If the transfer was successful or if returned with some error
 */
sdhc_error_t sdhcTransfer(sdhc_command_t* command, sdhc_data_t* data);

/********************
 * EVENT-DRIVEN API *
 *******************/

/*
 * @brief Whenever the driver detects a card being inserted, it raises an event
 * 		  and calls the callback registered.
 * @param callback		Callback being registered
 */
void sdhcOnCardInserted(sdhc_callback_t callback);

/*
 * @brief Whenever the driver detects a card being removed, it raises an event
 * 		  and calls the callback registered.
 * @param callback		Callback being registered
 */
void sdhcOnCardRemoved(sdhc_callback_t callback);

/*
 * @brief Calls the registered callback whenever a transfer process is completed.
 * @param callback		Callback being registered
 */
void sdhcOnTransferCompleted(sdhc_callback_t callback);

/*
 * @brief Calls the registered callback whenever a transfer process fails due to an error.
 * @param callback		Callback being registered
 */
void sdhcOnTransferError(sdhc_error_callback_t callback);

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_SDHC_SDHC_H_ */
