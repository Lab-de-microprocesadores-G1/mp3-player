/*******************************************************************************
  @file     spi.c
  @brief    SPI Peripheral MCAL driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "../gpio/gpio.h"

#include "spi_master.h"

#include "../CMSIS/MK64F12.h"

#include "lib/queue/queue.h"

#include "hardware.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define SPI_PORT_ALTERNATIVE    2             // All pins have the SPI peripheral connected to the 2nd alternative
#define SPI_CLOCK_FREQUENCY     100000000U    // Clock connected to the SPI peripheral in the MCU
#define TX_QUEUE_MAX_SIZE       200           // Maximum size of the FIFO for transmitter
#define RX_QUEUE_MAX_SIZE       5	          // Maximum size of the FIFO for the receiver

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// Declaring the pinout of a SPI peripheral
enum {
  SPI_SOUT,  // Data out the SPI
  SPI_SIN,   // Data in the SPI
  SPI_SCLK,   // Clock driven by the Master in the SPI communication
  SPI_SS_0,   // Selection of the Slave 0
  SPI_SS_1,   // Selection of the Slave 1
  SPI_SS_2,   // Selection of the Slave 2
  SPI_SS_3,   // Selection of the Slave 3
  SPI_SS_4,   // Selection of the Slave 4
  SPI_SS_5,   // Selection of the Slave 5
  SPI_PIN_COUNT
};

// Declaring the data structure of a baud rate setting
typedef struct {
  uint8_t       DBR : 1;
  uint8_t       PBR : 2;
  uint8_t       BR  : 4;
  
  uint32_t      baudRate;
} baud_rate_cfg_t;

typedef struct {
  uint8_t       slaves : 6;
  uint8_t       eoq    : 1;
  uint16_t      frame;
} spi_package_t;

// Declaring SPI instance data structure
typedef struct {

  // Hardware FIFO size
  uint8_t           hwFifoSize;

  // Buffering interface  
  spi_package_t     txBuffer[TX_QUEUE_MAX_SIZE];  // Data buffer for tx
  uint16_t          rxBuffer[RX_QUEUE_MAX_SIZE];  // Data buffer for rx
  queue_t           txQueue;                      // Queue instance for tx
  queue_t           rxQueue;                      // Queue instance for rx

  // Configuration of SPI hardware
  spi_cfg_t         config;
  
  // Flags
  bool transferComplete;

  // Callbacks
  spi_callback_t    onTransferCompleted;
} spi_instance_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

/**
 * @brief Computes the settings required to use a given baud rate in the SPI peripheral
 * @param baudRate     Target baud rate for the serial communication
 * @return Baud rate settings for the CTAR register
 */
static baud_rate_cfg_t computeBaudRateSettings(uint32_t baudRate);

/**
 * @brief Computes the baud rate used on the SPI peripheral if the given properties
 *        are set in the CTAR register.
 * @param dbr  Double Baud Rate
 * @param br   Baud Rate Scaler
 * @param pbr  Baud Rate Prescaler
 */
static uint32_t computeBaudRate(uint8_t dbr, uint8_t br, uint8_t pbr);

/**
 * @brief Sends a message to a some slaves, remember multiple slaves
 *        can be selected with the following syntax:
 *        slave = SPI_SLAVE_0 | SPI_SLAVE_1 | SPI_SLAVE_4;
 *        Adds the functionality that the message can be ignored and
 *        just send "trash".
 * @param id          SPI module id
 * @param slave   		Slaves to be selected
 * @param message     Message to be sent
 * @param len         Message length
 * @param sendTrash   When true, message content is ignored and just the software queue
 *                    is incremented in len.If false, sends the message
 * @return Whether it could send or not
 */
static bool smartSend(spi_id_t id, spi_slave_id_t slave, const uint16_t message[], size_t len, bool sendTrash);

/**
 * @brief Transfers the first element in the software queue to the hardware FIFO.
 */
static void softQueue2HardFIFO(spi_id_t id);

/*******************************************************************************
 *******************************************************************************
						      PROTOTYPES FOR INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

__ISR__  SPI0_IRQHandler(void);
__ISR__  SPI1_IRQHandler(void);
__ISR__  SPI2_IRQHandler(void);
static void     SPI_IRQDispatcher(spi_id_t id);
static void     SPI_EOQFDispatcher(spi_id_t id);
static void     SPI_RFDFDispatcher(spi_id_t id);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

static PORT_Type*     portPointers[]  = PORT_BASE_PTRS;
static SPI_Type*      spiPointers[]   = SPI_BASE_PTRS;
static const pin_t    spiPins[SPI_INSTANCE_AMOUNT][SPI_PIN_COUNT] = {
//  SOUT                SIN                 SCLK                SS0                SS1                SS2                SS3                SS4                SS5
  { PORTNUM2PIN(PD, 2), PORTNUM2PIN(PD, 3), PORTNUM2PIN(PD, 1), PORTNUM2PIN(PD, 0),PORTNUM2PIN(PC, 3),PORTNUM2PIN(PC, 2),PORTNUM2PIN(PC, 1),PORTNUM2PIN(PC, 0),PORTNUM2PIN(PB, 23)}, // SPI0
  { PORTNUM2PIN(PB, 16),PORTNUM2PIN(PB, 17),PORTNUM2PIN(PB, 2), PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3)}, // SPI1
  { PORTNUM2PIN(PB, 16),PORTNUM2PIN(PB, 17),PORTNUM2PIN(PB, 2), PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3),PORTNUM2PIN(PB, 3)}  // SPI2
};

static spi_instance_t spiInstances[SPI_INSTANCE_AMOUNT] = {
  { .hwFifoSize = 4 },
  { .hwFifoSize = 1 },
  { .hwFifoSize = 1 }
};

static uint8_t spiIrqs[] = SPI_IRQS;

// Look-up table for the SPI Prescaler
static uint8_t spiPrescaler[] = {
  2,
  3,
  5, 
  7
};

// Look-up table for the SPI Scaler
static uint16_t spiScaler[] = {
  2,
  4,
  6,
  8,
  16,
  32,
  64,
  128,
  256, 
  512,
  1024,
  2048,
  4096,
  8192,
  16384,
  32768
};


/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void spiInit(spi_id_t id, spi_slave_id_t slave, spi_cfg_t config)
{
  // Clock gating of the SPI peripheral
  SIM->SCGC6 |= SIM_SCGC6_SPI0(1);
  SIM->SCGC6 |= SIM_SCGC6_SPI1(1);
  SIM->SCGC3 |= SIM_SCGC3_SPI2(1);

  // Clock gating of the PORT peripheral
  SIM->SCGC5 |= SIM_SCGC5_PORTA(1);
  SIM->SCGC5 |= SIM_SCGC5_PORTB(1);
  SIM->SCGC5 |= SIM_SCGC5_PORTC(1);
  SIM->SCGC5 |= SIM_SCGC5_PORTD(1);
  SIM->SCGC5 |= SIM_SCGC5_PORTE(1);

  // Selecting the mux alternative for the port used
  for (uint8_t i = 0 ; i < SPI_PIN_COUNT ; i++)
  {
	  bool enableSlave = (slave & (0b000001 << (i - SPI_SS_0) )) > 0;
	  if (i == SPI_SIN || i == SPI_SOUT || i == SPI_SCLK || enableSlave)
	  {
		  portPointers[PIN2PORT(spiPins[id][i])]->PCR[PIN2NUM(spiPins[id][i])] = PORT_PCR_MUX(SPI_PORT_ALTERNATIVE);
	  }
  }

  // Configuration of the MCR register
  spiPointers[id]->MCR = SPI_MCR_PCSIS(config.slaveSelectPolarity == SPI_SS_INACTIVE_HIGH ? 0x3F : 0x00);
  spiPointers[id]->MCR |= SPI_MCR_HALT(1) | SPI_MCR_MSTR(1) | SPI_MCR_DIS_TXF(0) | SPI_MCR_DIS_RXF(0) | SPI_MCR_CLR_RXF(1) | SPI_MCR_CLR_TXF(1);

  // Computing the settings required to set the SPI peripheral with the given baud rate
  baud_rate_cfg_t settings = computeBaudRateSettings(config.baudRate);

  // Setting the frame size, clock polarity, clock phase, and baud rate in the CTAR register,
  // also adding delay to the signal to avoid frame overlapping
  spiPointers[id]->CTAR[0] = SPI_CTAR_FMSZ(config.frameSize - 1) |  SPI_CTAR_CPOL(config.clockPolarity) | SPI_CTAR_CPHA(config.clockPhase); 
  spiPointers[id]->CTAR[0] |= SPI_CTAR_PBR(settings.PBR) | SPI_CTAR_BR(settings.BR) | SPI_CTAR_DBR(settings.DBR);
  spiPointers[id]->CTAR[0] |= SPI_CTAR_PASC(settings.PBR) | SPI_CTAR_ASC(settings.BR);
  spiPointers[id]->CTAR[0] |= SPI_CTAR_PDT(settings.PBR) | SPI_CTAR_DT(settings.BR);
  spiPointers[id]->CTAR[0] |= SPI_CTAR_LSBFE(config.endianness);

  // Clear the flags and enable the interruption for the SPI peripheral
  spiPointers[id]->SR = SPI_SR_EOQF(1) | SPI_SR_TCF(1) | SPI_SR_TFUF(1) | SPI_SR_TFFF(1) | SPI_SR_RFOF(1) | SPI_SR_RFDF(1);
  spiPointers[id]->RSER = SPI_RSER_RFDF_RE(1) | SPI_RSER_EOQF_RE(1);
  NVIC_EnableIRQ(spiIrqs[id]);

  // Start the SPI peripheral
  spiPointers[id]->MCR = (SPI0->MCR & ~SPI_MCR_MDIS_MASK) | SPI_MCR_MDIS(0);

  // Instance initialization
  spiInstances[id].rxQueue = createQueue(spiInstances[id].rxBuffer, RX_QUEUE_MAX_SIZE, sizeof(uint16_t));
  spiInstances[id].txQueue = createQueue(spiInstances[id].txBuffer, TX_QUEUE_MAX_SIZE, sizeof(spi_package_t));
}

bool spiSend(spi_id_t id, spi_slave_id_t slave, const uint16_t message[], size_t len)
{
  return smartSend(id, slave, message, len, false);
}

bool spiCanSend(spi_id_t id, size_t len)
{
  return emptySize(&(spiInstances[id].txQueue)) >= len; // checks for empty space in driver's tx queue
}

bool spiReceive(spi_id_t id, spi_slave_id_t slave, size_t len)
{
  return smartSend(id, slave, NULL, len, true);
}

bool spiRead(spi_id_t id, uint16_t readBuffer[], size_t len)
{
  queue_t *rxQueue = &(spiInstances[id].rxQueue);

  if (size(rxQueue) >= len) // check if there is enough data to copy
  {
    popMany(rxQueue, (void*)readBuffer, len); // pop from rx queue
    return true;
  }
  else
  {
    return false;
  }
}

size_t spiGetReceiveCount(spi_id_t id)
{
  return size(&(spiInstances[id].rxQueue));
}

void spiReceiveFlush(spi_id_t id)
{
  clear(&(spiInstances[id].rxQueue));
}

bool spiTransferComplete(spi_id_t id)
{
	bool result = spiInstances[id].transferComplete;
	if (result)
	{
		spiInstances[id].transferComplete = false;
	}
	return result;
}

void spiOnTransferCompleted(spi_id_t id, spi_callback_t callback)
{
  spiInstances[id].onTransferCompleted = callback;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

bool smartSend(spi_id_t id, spi_slave_id_t slave, const uint16_t message[], size_t len, bool sendTrash)
{
  // Return false if there's not enough space in the software queue.
  if (!spiCanSend(id, len))
  {
    return false;
  }

  // If there is enough space, proceed to write the message in the software queue.
  if (sendTrash)
  {
    return pushTrash(&(spiInstances[id].txQueue), len);
  }
  else
  {
    for (uint8_t i=0; i < len; i++)
    {
      // Creating package.
      spi_package_t newPackage = {
    		  .slaves = slave,
			  .frame = message[i],
			  .eoq = ( ((i % spiInstances[id].hwFifoSize) == (spiInstances[id].hwFifoSize - 1) && (i > 0)) || i == len-1)  ? 1 : 0
      };
      // Pushing package to software queue.
      push(&(spiInstances[id].txQueue), (void *)(&newPackage));
    }
  }

  // If the transmission is not currently active (TXRXS is set), the firsts elements in the software queue
  // should be sent to the hardware FIFO to start the transmission.
  if ( (spiPointers[id]->SR & SPI_SR_TXRXS_MASK ) != SPI_SR_TXRXS_MASK )
  {
    softQueue2HardFIFO(id);
    spiPointers[id]->MCR = (spiPointers[id]->MCR & ~SPI_MCR_HALT_MASK) | SPI_MCR_HALT(0);
  }
  return true;
}

void softQueue2HardFIFO(spi_id_t id)
{
  while ((spiPointers[id]->SR & SPI_SR_TFFF_MASK) && !isEmpty(&(spiInstances[id].txQueue)))
  {
    // Getting package in software queue.
    spi_package_t* package = (spi_package_t*)pop(&(spiInstances[id].txQueue));
    
    // Writing message to hardware TX FIFO.
    spiPointers[id]->PUSHR = SPI_PUSHR_CONT(spiInstances[id].config.continuousPcs) | SPI_PUSHR_CTAS(0b000) | SPI_PUSHR_EOQ(package->eoq) | 
                             SPI_PUSHR_CTCNT(1) | SPI_PUSHR_PCS(package->slaves) | SPI_PUSHR_TXDATA(package->frame);

    // Clear the flag just in case
    spiPointers[id]->SR = SPI_SR_TFFF_MASK;
  }
}

static baud_rate_cfg_t computeBaudRateSettings(uint32_t baudRate)
{
  baud_rate_cfg_t setting = { .BR = 0 , .DBR = 0 , .PBR = 0 };
  uint32_t bestBaudRate = 0;
  uint32_t bestError = 0;
  uint32_t currentBaudRate = 0;
  uint32_t currentError = 0;

  for (uint8_t dbr = 0 ; dbr < 1 ; dbr++)
  {
    for (uint8_t pbr = 0 ; pbr < 4 ; pbr++)
    {
      for (uint8_t br = 0 ; br < 16 ; br++)
      {
        currentBaudRate = computeBaudRate(dbr, br, pbr);
        currentError = baudRate < currentBaudRate ? currentBaudRate - baudRate : baudRate - currentBaudRate;
        if (bestBaudRate == 0 || currentError < bestError)
        {
          // Save the current best baud rate values
          bestBaudRate = currentBaudRate;
          bestError = currentError;
          
          // Save the current configuration of the register for the best baud rate
          setting.BR = br;
          setting.PBR = pbr;
          setting.DBR = dbr;
          setting.baudRate = bestBaudRate;
        }
      }
    }
  }

  return setting;
}

static uint32_t computeBaudRate(uint8_t dbr, uint8_t br, uint8_t pbr)
{
  return ( SPI_CLOCK_FREQUENCY * (1 + dbr) ) / ( spiScaler[br] * spiPrescaler[pbr] );
}

__ISR__  SPI0_IRQHandler(void)
{
  SPI_IRQDispatcher(SPI_INSTANCE_0);
}

__ISR__  SPI1_IRQHandler(void)
{
  SPI_IRQDispatcher(SPI_INSTANCE_1);
}

__ISR__  SPI2_IRQHandler(void)
{
  SPI_IRQDispatcher(SPI_INSTANCE_2); 
}

static void SPI_IRQDispatcher(spi_id_t id)
{
  // Read Status Register
  uint32_t sr = spiPointers[id]->SR;

  // If last package was sent
  if (sr & SPI_SR_EOQF_MASK)
  {
	// Clear flag
    spiPointers[id]->SR = SPI_SR_EOQF_MASK;
    // Do what needs to be done
    SPI_EOQFDispatcher(id);
  }

  // If something was 
  if (sr & SPI_SR_RFDF_MASK)
  {
    // Clear flag
    spiPointers[id]->SR = SPI_SR_RFDF_MASK;
    // Do what needs to be done
    SPI_RFDFDispatcher(id);
  }
}

static void SPI_EOQFDispatcher(spi_id_t id)
{
  if (isEmpty(&(spiInstances[id].txQueue)))
  {
    spiPointers[id]->MCR = (spiPointers[id]->MCR & ~SPI_MCR_HALT_MASK) | SPI_MCR_HALT(1);
    if (spiInstances[id].onTransferCompleted)
    {
      spiInstances[id].onTransferCompleted();
    }
    else
    {
      spiInstances[id].transferComplete = true;
    }
  }
  else
  {
    softQueue2HardFIFO(id);
  }
}

static void SPI_RFDFDispatcher(spi_id_t id)
{
  // Read RX Hardware FIFO
  if (spiPointers[id]->SR & SPI_SR_RXCTR_MASK)
  {
    uint16_t newFrame = spiPointers[id]->POPR;
    if (emptySize(&(spiInstances[id].rxQueue)) > 0)
    {
	  push(&(spiInstances[id].rxQueue), (void*) &newFrame);
    }
  }
}
