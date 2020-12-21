/***************************************************************************//**
  @file     dma_sga.h
  @brief    Handler for DMA with Scatter and Gather operation
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_DMA_DMA_H_
#define MCAL_DMA_DMA_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
 #include <stdint.h>


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define DMA_SGA_TCD_COUNT       2
#define DMA_SGA_PPBUFFER_COUNT  DMA_SGA_TCD_COUNT

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

// DMA Event callback for interrupt requests
typedef void (*dma_sga_callback_t)(void);

// DMA Channels enumeration
typedef enum
{
  DMA_CHANNEL_0,
  DMA_CHANNEL_1,
  DMA_CHANNEL_2,
  DMA_CHANNEL_3,
  DMA_CHANNEL_4,
  DMA_CHANNEL_5,
  DMA_CHANNEL_6,
  DMA_CHANNEL_7,
  DMA_CHANNEL_8,
  DMA_CHANNEL_9,
  DMA_CHANNEL_10,
  DMA_CHANNEL_11,
  DMA_CHANNEL_12,
  DMA_CHANNEL_13,
  DMA_CHANNEL_14,
  DMA_CHANNEL_15,
  DMA_CHANNEL_COUNT
} dma_channel_t;

// DMA TCD Structure
typedef struct {
  uint32_t SADDR;                               /**< TCD Source Address, array offset: 0x1000, array step: 0x20 */
  uint16_t SOFF;                                /**< TCD Signed Source Address Offset, array offset: 0x1004, array step: 0x20 */
  uint16_t ATTR;                                /**< TCD Transfer Attributes, array offset: 0x1006, array step: 0x20 */
  union {                                       /* offset: 0x1008, array step: 0x20 */
    uint32_t NBYTES_MLNO;                       /**< TCD Minor Byte Count (Minor Loop Disabled), array offset: 0x1008, array step: 0x20 */
    uint32_t NBYTES_MLOFFNO;                    /**< TCD Signed Minor Loop Offset (Minor Loop Enabled and Offset Disabled), array offset: 0x1008, array step: 0x20 */
    uint32_t NBYTES_MLOFFYES;                   /**< TCD Signed Minor Loop Offset (Minor Loop and Offset Enabled), array offset: 0x1008, array step: 0x20 */
  };
  uint32_t SLAST;                               /**< TCD Last Source Address Adjustment, array offset: 0x100C, array step: 0x20 */
  uint32_t DADDR;                               /**< TCD Destination Address, array offset: 0x1010, array step: 0x20 */
  uint16_t DOFF;                                /**< TCD Signed Destination Address Offset, array offset: 0x1014, array step: 0x20 */
  union {                                       /* offset: 0x1016, array step: 0x20 */
    uint16_t CITER_ELINKNO;                     /**< TCD Current Minor Loop Link, Major Loop Count (Channel Linking Disabled), array offset: 0x1016, array step: 0x20 */
    uint16_t CITER_ELINKYES;                    /**< TCD Current Minor Loop Link, Major Loop Count (Channel Linking Enabled), array offset: 0x1016, array step: 0x20 */
  };
  uint32_t DLAST_SGA;                           /**< TCD Last Destination Address Adjustment/Scatter Gather Address, array offset: 0x1018, array step: 0x20 */
  uint16_t CSR;                                 /**< TCD Control and Status, array offset: 0x101C, array step: 0x20 */
  union {                                       /* offset: 0x101E, array step: 0x20 */
    uint16_t BITER_ELINKNO;                     /**< TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Disabled), array offset: 0x101E, array step: 0x20 */
    uint16_t BITER_ELINKYES;                    /**< TCD Beginning Minor Loop Link, Major Loop Count (Channel Linking Enabled), array offset: 0x101E, array step: 0x20 */
  };
} dma_tcd_t;


typedef struct 
{
  uint8_t   muxSource;
  uint8_t   pitEn : 1;                // Enable period triggering
  dma_tcd_t tcds[DMA_SGA_TCD_COUNT] __attribute__ ((aligned(32)));

  // Arbitration configurations
  uint8_t   fpArb : 1;                // Enable fixed-priority arbitration (Otherwise )
  uint8_t   ecp : 1;                  // Enable preemption by a higher priority channel
  uint8_t   dpa : 1;                  // Disable channel from suspending lower priority channel
  uint8_t   priority;                 // For fixed-priority arbitrarion, choose priority
} dma_sga_channel_cfg_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/**
 * @brief Initializes the eDMA and DMAMUX peripherals
 */ 
void dmasgaInit(void);

/**
 * @brief DMA channel configuration
 * @param channel DMA channel 
 * @param config
 */ 
void dmasgaChannelConfig(dma_channel_t channel, dma_sga_channel_cfg_t config);

/**
 * @brief Registers callback to be called on Major Loop interrupt request
 * @param channel   DMA channel 
 * @param callback  
 */ 
void dmasgaOnMajorLoop(dma_channel_t channel, dma_sga_callback_t callback);

/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_DMA_DMA_H_ */
