/*
 * halFLASH_stm32f103.c
 *
 *  Created on: 12 апр. 2017 г.
 *      Author: Maksim
 */

#include <flash.h>
#include "stm32f1xx_hal.h"
#include "stm32_hal_legacy.h"
//#include "portable.h"
#include "string.h"
#include "stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define FIRST_PAGE_ADDRESS  0x08000000

#define PAGE_SIZE 1024

#define PAGE_COUNT 64

#define PAGE_SETUP 62

#define PAGE_SIGNALISATION_STATE 61


static uint8_t halFlash_WritePage(uint8_t PageNumber, uint8_t *DataPointer, uint16_t DataCount);

static uint8_t halFlash_ReadPage(uint8_t PageNumber, uint8_t *DataPointer, uint16_t DataCount);

/*
  Name  : CRC-8
  Poly  : 0x31    x^8 + x^5 + x^4 + 1
  Init  : 0xFF
  Revert: false
  XorOut: 0x00
  Check : 0xF7 ("123456789")
  MaxLen: 15 байт(127 бит) - обнаружение
    одинарных, двойных, тройных и всех нечетных ошибок
*/
static uint8_t Crc8(uint8_t *pcBlock, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i;

    while (len--)
    {
        crc ^= *pcBlock++;

        for (i = 0; i < 8; i++)
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }

    return crc;
}


void Flash_Init(void) {

	SetupStruct_t defaultSetup = {
			.firstPhone = "380950451110",
			.secondPhone = "380971910961",
			.signPrepareDelay = 30000,
			.signPauseDelay = 60000,
			.signStartCallDelay = 3000,
			.signStartBeepDelay = 10000,
			.callAbortTimeout = 20000,
			.buzzerActive = 1500,
			.buzzerPassive = 1000,
			.buzzerActimeSome = 250,
			.buzzerPassiveSome = 250,
			.doorUpdateCount = 200,
			.password = { 1, 2, 3, 4 },
			.dummy = 0,
			.CRC8 = 0
	};

	SignalState_t defaultSignState = {
			.signal_state = SignState_Disarmed,
			.CRC8 = 0
	};

	if(Flash_ReadSetup(&defaultSetup) != 0) {

		Flash_WriteSetup(&defaultSetup);

	}

	if(Flash_ReadSignalState(&defaultSignState) != 0) {

		Flash_WriteSignalState(&defaultSignState);

	}

	/* Enable Prefetch Buffer */
	//FLASH_PrefetchBufferCmd( FLASH_PrefetchBuffer_Enable);

	/* Flash 2 wait state */
	//FLASH_SetLatency( FLASH_Latency_2);

}

void Flash_WriteSetup(SetupStruct_t *setup) {

	setup->CRC8 = 0;

	setup->CRC8 = Crc8((uint8_t *)setup, sizeof(SetupStruct_t));

	halFlash_WritePage(PAGE_SETUP, (uint8_t *)setup, sizeof(SetupStruct_t));

}

uint8_t Flash_ReadSetup(SetupStruct_t *setup) {
	SetupStruct_t temp_setup;
	uint8_t temp_crc = 0;
	halFlash_ReadPage(PAGE_SETUP, (uint8_t *)&temp_setup, sizeof(SetupStruct_t));

	temp_crc = temp_setup.CRC8;

	temp_setup.CRC8 = 0;

	if(Crc8((uint8_t *)&temp_setup, sizeof(SetupStruct_t)) == temp_crc) {

		memcpy(setup, &temp_setup, sizeof(SetupStruct_t));

		return 0;

	}

	return 1;

}

void Flash_WriteSignalState(SignalState_t *sigstate) {

	sigstate->CRC8 = 0;

	sigstate->CRC8 = Crc8((uint8_t *)sigstate, sizeof(SignalState_t));

	halFlash_WritePage(PAGE_SIGNALISATION_STATE, (uint8_t *) sigstate, sizeof(SignalState_t));

}

uint8_t Flash_ReadSignalState(SignalState_t *sigstate) {
	SignalState_t temp_sigstate;
	uint8_t temp_crc = 0;
	halFlash_ReadPage(PAGE_SIGNALISATION_STATE, (uint8_t *)&temp_sigstate, sizeof(SignalState_t));

	temp_crc = temp_sigstate.CRC8;

	temp_sigstate.CRC8 = 0;

	if(Crc8((uint8_t *)&temp_sigstate, sizeof(SignalState_t)) == temp_crc) {

		memcpy(sigstate, &temp_sigstate, sizeof(SignalState_t));

		return 0;

	}

	return 1;

}

static uint8_t halFlash_WritePage(uint8_t PageNumber, uint8_t *DataPointer, uint16_t DataCount) {
	uint16_t *WriteDataPointer;
	uint16_t DataCounter;
	uint8_t returnState = 1;
	uint32_t page_error;

	if(PageNumber < PAGE_COUNT && DataCount <= PAGE_SIZE) {

			WriteDataPointer = (uint16_t *)DataPointer;

			HAL_FLASH_Unlock();

			FLASH_EraseInitTypeDef flash_erase;

			flash_erase.TypeErase = TYPEERASE_PAGES;
			flash_erase.Banks = 0;
			flash_erase.NbPages = 1;
			flash_erase.PageAddress = FIRST_PAGE_ADDRESS + PageNumber * PAGE_SIZE;

			if(HAL_FLASHEx_Erase(&flash_erase, &page_error) == HAL_OK) {

				for (DataCounter = 0; DataCounter < (DataCount / 2); DataCounter++) {

					if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint16_t)(FIRST_PAGE_ADDRESS + PageNumber * PAGE_SIZE + DataCounter * 2), *(WriteDataPointer + DataCounter) ) != HAL_OK) {

						returnState = 1;

						break;

					} else {

						returnState = 0;

					}


				}

			}

			HAL_FLASH_Lock();


	} else {

		returnState = 1;

	}

	return returnState;

}

static uint8_t halFlash_ReadPage(uint8_t PageNumber, uint8_t *DataPointer, uint16_t DataCount) {
	uint8_t returnState = 1;

	if(PageNumber < PAGE_COUNT && DataCount <= PAGE_SIZE) {

		memcpy((void *)DataPointer, (void *)(FIRST_PAGE_ADDRESS + PageNumber * PAGE_SIZE), DataCount);

		returnState = 0;

	} else {

		returnState = 1;

	}

	return returnState;
}


