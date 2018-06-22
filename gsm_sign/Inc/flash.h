/*
 * halFLASH.h
 *
 *  Created on: 12 ���. 2017 �.
 *      Author: Maksim
 */

#ifndef HAL_INC_HALFLASH_H_
#define HAL_INC_HALFLASH_H_

#include "stdint.h"
#include "stddef.h"

typedef enum {
	SignState_Disarmed = 0xFA,
	SignState_Armed = 0xAF
} SignState_t;

typedef struct {
	char firstPhone[15];
	char secondPhone[15];
	uint32_t signPrepareDelay;
	uint32_t signPauseDelay;
	uint32_t signStartCallDelay;
	uint32_t signStartBeepDelay;
	uint32_t callAbortTimeout;
	uint32_t buzzerActive;
	uint32_t buzzerPassive;
	uint32_t buzzerActimeSome;
	uint32_t buzzerPassiveSome;
	uint32_t doorUpdateCount;
	uint8_t password[4];
	uint8_t dummy;
	uint8_t CRC8;

} __attribute__((aligned(1),packed)) SetupStruct_t;


typedef struct {
	SignState_t signal_state;
	uint8_t CRC8;
} __attribute__((aligned(1),packed)) SignalState_t;

void Flash_Init(void);

void Flash_WriteSetup(SetupStruct_t *setup);

uint8_t Flash_ReadSetup(SetupStruct_t *setup);

void Flash_WriteSignalState(SignalState_t *sigstate);

uint8_t Flash_ReadSignalState(SignalState_t *sigstate);

#endif /* HAL_INC_HALFLASH_H_ */
