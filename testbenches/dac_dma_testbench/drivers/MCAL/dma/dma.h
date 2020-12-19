/***************************************************************************//**
  @file     dma.h
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef MCAL_DMA_DMA_H_
#define MCAL_DMA_DMA_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/


/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define DMA_PPBUFFER_COUNT  2
#define DMA_TCD_COUNT       DMA_PPBUFFER_COUNT
/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/
typedef enum
{
  DMA_CHANNEL_0,
  DMA_CHANNEL_1,
  DMA_CHANNEL_2,
  DMA_CHANNEL_COUNT
}dma_channel_t;

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
} dma_TCD_t;


typedef struct 
{
  void*     ppBuffer[DMA_PPBUFFER_COUNT];
  uint8_t   enablePeriodicTrigger : 1;
  uint8_t   enableScatterGather : 1;
  uint8_t   muxSource;

  // DMA context
  dma_TCD_t tcd[DMA_TCD_COUNT];




}dma_channel_cfg_t;

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

void dmaInit(void);
void dmaChannelConfig(dma_channel_t channel, dma_channel_cfg_t cfg )
/*******************************************************************************
 ******************************************************************************/


#endif /* MCAL_DMA_DMA_H_ */
