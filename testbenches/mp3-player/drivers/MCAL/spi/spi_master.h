/*******************************************************************************
  @file     spi.h
  @brief    SPI Peripheral MCAL driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_SPI_SPI_H_
#define MCAL_SPI_SPI_H_

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

// Declaring a callback used to raise events from the SPI driver
typedef void  (*spi_callback_t)(void);

// Declaring the ID of the SPI slaves
typedef enum {
  SPI_SLAVE_0 = 0b000001,
  SPI_SLAVE_1 = 0b000010,
  SPI_SLAVE_2 = 0b000100,
  SPI_SLAVE_3 = 0b001000,
  SPI_SLAVE_4 = 0b010000,
  SPI_SLAVE_5 = 0b100000
} spi_slave_id_t;

// Declaring the ID of the SPI peripheral instances
typedef enum {
  SPI_INSTANCE_0,
  SPI_INSTANCE_1,
  SPI_INSTANCE_2,
  SPI_INSTANCE_AMOUNT
} spi_id_t;

// Slave select active state modes
typedef enum {
  SPI_SS_INACTIVE_LOW,
  SPI_SS_INACTIVE_HIGH
} spi_ss_active_t;

// Clock polarity modes
typedef enum {
  SPI_CPOL_INACTIVE_LOW,
  SPI_CPOL_INACTIVE_HIGH
} spi_cpol_t;

// Clock phase modes
typedef enum {
  SPI_CPHA_FIRST_CAPTURE,
  SPI_CPHA_FIRST_CHANGE
} spi_cpha_t;

// Endianness of the frame size sent
typedef enum {
  SPI_ENDIANNESS_MSB_FIRST,
  SPI_ENDIANNESS_LSB_FIRST
} spi_endianness_t;

// Whether PCS is disabled after transfer or not
typedef enum {
  SPI_CONTINUOUS_PCS_DIS,
  SPI_CONTINUOUS_PCS_EN
} spi_continuous_pcs_t;

// Declaring the SPI configuration
typedef struct{
  uint32_t          baudRate;
  uint8_t           frameSize;
  uint8_t           slaveSelectPolarity   : 1;
  uint8_t           clockPolarity         : 1;
  uint8_t           clockPhase            : 1;
  uint8_t           endianness            : 1;
  uint8_t           continuousPcs         : 1;
} spi_cfg_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief SPI module initialization
 * @param id      SPI module id
 * @param slave   Declares the slaves to be used with SPI
 * @param config  SPI configuration attributes 
 */
void spiInit(spi_id_t id, spi_slave_id_t slave, spi_cfg_t config);

/*********************
 * SPI SEND SERVICES *
 ********************/

/**
 * @brief Sends a message to a some slaves, remember multiple slaves
 *        can be selected with the following syntax:
 *        slave = SPI_SLAVE_0 | SPI_SLAVE_1 | SPI_SLAVE_4;
 * @param id      SPI module id
 * @param slave   Slaves to be selected
 * @param message Message to be sent
 * @param len     Message length
 * @return Whether it could send or not
 */
bool spiSend(spi_id_t id, spi_slave_id_t slave, const uint16_t message[], size_t len);

/**
 * @brief Checks if ready to send
 * @param id      SPI module id
 * @param len     Message length
 * @return Whether it can send the given amount of frames
 */
bool spiCanSend(spi_id_t id, size_t len);

/************************
 * SPI RECEIVE SERVICES *
 ***********************/

/**
 * @brief Request to receive from SPI a given amount of frames. Non blocking.
 * @param id    SPI module id
 * @param slave   Slaves to be selected
 * @param len   Message length
 * @return Whether it could receive or not (needs to send len 0s)
 */
bool spiReceive(spi_id_t id, spi_slave_id_t slave, size_t len);

/**
 * @brief Reads a message
 * @param id          SPI module id
 * @param readBuffer  Buffer to save the received message
 * @param len         Message length
 */
bool spiRead(spi_id_t id, uint16_t readBuffer[], size_t len);

/**
 * @brief Returns the count of frames received until this moment.
 * @param id          SPI module id
 * @return Amount of frames received
 */
size_t spiGetReceiveCount(spi_id_t id);

/**
 * @brief Flush the receive buffer, clears all its entries.
 * @param id          SPI module id
 */
void spiReceiveFlush(spi_id_t id);

/***************************
 * POLLING STATUS SERVICES *
 **************************/

/**
 * @brief When a transfer process is completed
 * @param id		  SPI module id
 */
bool spiTransferComplete(spi_id_t id);

/********************************
 * EVENT-DRIVEN STATUS SERVICES *
 *******************************/

/*
 * @brief Registers a callback to be called when the spi transfer is completed
 * @param id		  SPI module id
 * @param callback  Callback to be registered 
 */
void spiOnTransferCompleted(spi_id_t id, spi_callback_t callback);

#endif
