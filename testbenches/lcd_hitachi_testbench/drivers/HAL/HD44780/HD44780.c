/***************************************************************************//**
  @file     HD44780.c
  @brief    Hitachi HD44780 LCD Display Low-Level Driver using SPI
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "drivers/HAL/timer/timer.h"
#include "drivers/MCAL/spi/spi_master.h"
#include "HD44780.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

// Parallel output port positions
#define HD44780_RS_POS		5
#define HD44780_E_POS		4
#define HD44780_VALUE_POS 	0

// @brief Obtains the output port from the given data
// @param rs	1 bit for LCD RS pin
// @param e		1 bit for LCD E pin
// @param v		4 bits for LCD D4-D7
#define HD44780_PORT_VALUE(rs, e, v)	((((rs) & 1) << HD44780_RS_POS) | (((e) & 1) << HD44780_E_POS) | (((v) & 0xF) << HD44780_VALUE_POS))

// Delay constants
#define HD44780_LONG_DELAY	10000
#define HD44780_E_DELAY		5000

// SPI Serial2Parallel port definitions
#define SPORT_SPI_INSTANCE	SPI_INSTANCE_0
#define SPORT_SPI_SLAVE		1
#define SPORT_SPI_BAUD_RATE	700000	// 700 kbps ensures 40us between instructions
#define SPORT_SPI_FRAME_SIZE	6	// 4 data bits, 1 for E and 1 for RS

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
	HD44780_NOT_INIT,
	HD44780_INITIALIZING_8BIT,
	HD44780_INITIALIZING_4BIT,	// Changes when MPU is set to 4bit operation
	HD44780_INITIALIZED
} hd44780_init_state_t;

typedef struct {
	uint8_t	rs 		: 1;
	uint8_t nybble 	: 4;
} hd44780_nyb_pkg_t;

// Driver context
typedef struct {

	// Initialization-related variables
	uint8_t 				initState;		// Indicates if init has started, is in progress or ended
	uint8_t 				initStepCount;	// Keeps track of initialization steps

	// LCD configuration given by the user
	hd44780_cfg_t 			config;

	// Output port context
	uint8_t 				currentPortValue;

	// Timer ID given by the driver
	tim_id_t				idTimer;

} hd44780_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static hd44780_context_t lcdContext;

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

// HD44780 non-blocking functions

/**
 * @brief Writes the given instruction nybble to the LCD
 * @param instruction	Instruction nybble to be written
 */
static void writeInstructionNybble(uint8_t instruction);

/**
 * @brief Writes the given data nybble to the LCD
 * @param data	Data nybble to be written
 */
static void writeDataNybble(uint8_t data);

/**
 * @brief Starts writing the given nybble to the LCD and queues
 * 			the E pulse.
 * @param rs		RS bit to be written
 * @param nybble	Nybble to be written to D4-D7
 */
static void writeNybble(uint8_t rs, uint8_t nybble);

// EVENT CALLBACKS

/**
 * @brief Performs the LCD initialization step-by-step when called on timeout
 */
static void cycleInitialization(void);

// SERIAL TO PARALLEL PORT INTERFACE

/**
 * @brief Updates the whole output port
 * @param value		Output value
 */
static void portWriteOutput(uint8_t value);

/**
 * @brief Updates one of the 8 bits of the output port
 * @param pos	Bit number
 * @param value Bit value
 */
static void portWriteOutputBit(uint8_t pos, bool value);

/**
 * @brief Writes a sequence of values to the output port
 * @param buffer	Pointer to the sequence
 * @param len		Amount of values to send
 */
static bool portWriteMany(uint16_t * buffer, size_t len);

/**
 * @brief Updates the value of the output port
 * Note: this function should only be used by portWrite...()
 */
static bool portUpdateOutput(void);


/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void HD44780Init(hd44780_cfg_t config)
{
	if (lcdContext.initState == HD44780_NOT_INIT)
	{
		// Set states
		lcdContext.initState = HD44780_INITIALIZING_8BIT;

		// Set initialization step
		lcdContext.initStepCount = 0;

		// Output starts at 0x00
		lcdContext.currentPortValue = 0;

		// Save user configuration
		lcdContext.config = config;

		// Initialization of the timer driver
		timerInit();
		lcdContext.idTimer = timerGetId();

		// Initialization of the SPI driver
		spi_cfg_t spiConfig = {
			.baudRate = SPORT_SPI_BAUD_RATE,
			.frameSize = SPORT_SPI_FRAME_SIZE,
			.slaveSelectPolarity = SPI_SS_INACTIVE_HIGH,
			.continuousPcs = SPI_CONTINUOUS_PCS_DIS,
			.clockPhase = SPI_CPHA_FIRST_CAPTURE,
			.clockPolarity = SPI_CPOL_INACTIVE_LOW,
			.endianness = SPI_ENDIANNESS_MSB_FIRST
		};
		spiInit(SPORT_SPI_INSTANCE, SPORT_SPI_SLAVE, spiConfig);

		// Schedule initialization start. Controller needs 15ms since power-up.
		timerStart(lcdContext.idTimer, TIMER_MS2TICKS(15), TIM_MODE_SINGLESHOT, cycleInitialization);
	}
}

bool HD44780InitReady(void)
{
	return (lcdContext.initState == HD44780_INITIALIZED);
}

void HD44780WriteInstruction(uint8_t instruction)
{
	if (lcdContext.initState >= HD44780_INITIALIZING_4BIT)
	{
		// Write both nybbles, MSN first
		writeInstructionNybble( (instruction & 0xF0) >> 4);
		writeInstructionNybble( instruction & 0x0F );
	}
}

void HD44780WriteData(uint8_t data)
{
	if (lcdContext.initState >= HD44780_INITIALIZING_4BIT)
	{
		// Write both nybbles, MSN first
		writeDataNybble( (data & 0xF0) >> 4);
		writeDataNybble( data & 0x0F );
	}
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

// HD44780 helper functions

void writeInstructionNybble(uint8_t instruction)
{
	// Write nybble with RS=0 (instruction)
	writeNybble(0, instruction);
}

void writeDataNybble(uint8_t data)
{
	// Write nybble with RS=1 (data)
	writeNybble(1, data);
}


void writeNybble(uint8_t rs, uint8_t nybble)
{
	uint16_t message[3];

	message[0] = HD44780_PORT_VALUE(rs, 0, nybble);	// E = 0, write D4-D7 and RS
	message[1] = HD44780_PORT_VALUE(rs, 1, nybble);	// E = 1, write D4-D7 and RS
	message[2] = HD44780_PORT_VALUE(rs, 0, nybble);	// E = 0, write D4-D7 and RS

	// Start transfer
	portWriteMany(message, 3);
}

uint8_t parallelOutput4bit(uint8_t enable, uint8_t rs, uint8_t value)
{
	uint8_t newPortValue = ((rs & 1) << HD44780_RS_POS) | ((enable & 1) << HD44780_E_POS) | ((value & 0xF) << HD44780_VALUE_POS);
	portWriteOutput(newPortValue);
}

// EVENT CALLBACKS

// TIMER CALLBACK
void cycleInitialization(void)
{
	uint8_t initStepCount = ++lcdContext.initStepCount;
	hd44780_cfg_t config = lcdContext.config;

	switch(initStepCount - 1)
	{
	case 0: // Function set 8 bit, dummy DL and F
		writeInstructionNybble(HD44780_FUNCTION_SET(1, 0, 0) >> 4);
		timerStart(lcdContext.idTimer, TIMER_MS2TICKS(4), TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 1: // Function set 8 bit, dummy DL and F
		writeInstructionNybble(HD44780_FUNCTION_SET(1, 0, 0) >> 4);
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 2: // Function set 8 bit, dummy DL and F
		writeInstructionNybble(HD44780_FUNCTION_SET(1, 0, 0) >> 4);
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 3: // Function set 4 bit
		writeInstructionNybble(HD44780_FUNCTION_SET(0, 0, 0) >> 4);
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		lcdContext.initState = HD44780_INITIALIZING_4BIT;
		break;
	case 4:	// Function set 4 bit, specify display lines and character font
		HD44780WriteInstruction(HD44780_FUNCTION_SET(0, config.bothLines, config.charFont));
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 5: // Display Off
		HD44780WriteInstruction(HD44780_DISPLAY_CONTROL(0,0,0));
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 6:	// Display Clear, may take some time
		HD44780WriteInstruction(HD44780_CLEAR_DISPLAY);
		timerStart(lcdContext.idTimer, TIMER_MS2TICKS(2), TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 7:	// Entry Mode Set, specify increment and shift
		HD44780WriteInstruction(HD44780_ENTRY_MODE_SET(config.inc, config.shift));
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 8: // Display On, specify cursor and cursor position blink
		HD44780WriteInstruction(HD44780_DISPLAY_CONTROL(1, config.cursor, config.blink));
		timerStart(lcdContext.idTimer, 1, TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 9: // Set cursor to 0. RETURN_HOME takes up to 1.52 ms
		HD44780WriteInstruction(HD44780_RETURN_HOME);
		timerStart(lcdContext.idTimer, TIMER_MS2TICKS(2), TIM_MODE_SINGLESHOT, cycleInitialization);
		break;
	case 10: // Init ready
		lcdContext.initState = HD44780_INITIALIZED;
		break;
	}
}

// SERIAL TO PARALLEL PORT INTERFACE

void portWriteOutput(uint8_t value)
{
	lcdContext.currentPortValue = value;
	portUpdateOutput();
}

void portWriteOutputBit(uint8_t pos, bool value)
{
	lcdContext.currentPortValue = (lcdContext.currentPortValue & ~(1 << pos)) | ((value ? 1 : 0) << pos);
	portUpdateOutput();
}

bool portWriteMany(uint16_t * buffer, size_t len)
{
	bool ret = false;

	// Write if SPI available
	if (spiCanSend(SPORT_SPI_INSTANCE, len))
	{
		// Send values to the output
		spiSend(SPORT_SPI_INSTANCE, SPORT_SPI_SLAVE, buffer, len);

		// Save last value as current value
		lcdContext.currentPortValue = buffer[len-1];

		// Success
		ret = true;
	}
	else
	{
		ret = false;
	}

	return ret;
}

bool portUpdateOutput(void)
{
	bool ret = false;
	uint16_t message = lcdContext.currentPortValue;

	return portWriteMany(&message, 1);
}


/******************************************************************************/
