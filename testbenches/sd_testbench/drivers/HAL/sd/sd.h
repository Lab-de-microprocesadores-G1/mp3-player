/*******************************************************************************
  @file     sd.h
  @brief    SD card driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef HAL_SD_SD_H_
#define HAL_SD_SD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

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
	SD_ACMD_6	=	0,
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
	SD_GO_IDLE_STATE		=	SD_CMD_0,
	SD_SEND_IF_COND			=	SD_CMD_8,
	SD_APP_CMD				=	SD_CMD_55,
	SD_SEND_OP_COND			=	SD_ACMD_41,
	SD_ALL_SEND_CID			=	SD_CMD_2,
	SD_SEND_RELATIVE_ADDR	=	SD_CMD_3
};

typedef void (sd_callback_t)	(void);

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
 * @brief Initializes the driver.
 */
void sdInit(void);

/*
 * @brief 	Initializes the SD card, following the steps described by the SD physical
 * 		  	layer specification.
 * @returns Whether it could initialize the card or not.
 */
bool sdCardInit(void);

/*
 * @brief	Returns whether a card is inserted in the SD socket or not.
 */
bool sdIsCardInserted(void);

/********************
 * EVENT-DRIVEN API *
 *******************/

/*
 * @brief Registers a callback to be called whenever a card is inserted.
 * @param callback	Callback to be registered
 */
void sdOnCardInserted(sd_callback_t callback);

/*
 * @brief Registers a callback to be called whenever a card is removed.
 * @param callback	Callback to be registered
 */
void sdOnCardRemoved(sd_callback_t callback);

/*******************************************************************************
 ******************************************************************************/


#endif /* HAL_SD_SD_H_ */
