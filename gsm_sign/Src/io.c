/*
 * io.c
 *
 *  Created on: 13 ���. 2018 �.
 *      Author: Max
 */

#include "FreeRTOS.h"
#include "task.h"
#include "io.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "strings.h"

#define BUZZER_PERIOD_ACTIVE 700

#define BUZZER_PERIOD_PASSIVE 500

#define BUTTON_UPDATE_COUNT 20

#define BUTTON_PRESS_OFFSET (BUTTON_UPDATE_COUNT * 0.8)

typedef enum {
	BuzzerPhase_Active,
	BuzzerPhase_Passive
} BuzzerPhase_t;

static const uint8_t sig_patterns[14][8] = {
		{ 1, 1, 1, 0, 1, 1, 1, 0 }, // 0
		{ 0, 0, 1, 0, 0, 1, 0, 0 }, // 1
		{ 1, 0, 1, 1, 1, 0, 1, 0 }, // 2
		{ 1, 0, 1, 1, 0, 1, 1, 0 }, // 3
		{ 0, 1, 1, 1, 0, 1, 0, 0 }, // 4
		{ 1, 1, 0, 1, 0, 1, 1, 0 }, // 5
		{ 1, 1, 0, 1, 1, 1, 1, 0 }, // 6
		{ 1, 0, 1, 0, 0, 1, 0, 0 }, // 7
		{ 1, 1, 1, 1, 1, 1, 1, 0 }, // 8
		{ 1, 1, 1, 1, 0, 1, 1, 0 }, // 9
		{ 0, 0, 0, 0, 0, 0, 0, 1 }, // .
		{ 1, 1, 1, 0, 1, 1, 1, 0 }, // o
		{ 1, 1, 1, 0, 1, 1, 0, 0 }, // n
		{ 1, 1, 0, 1, 1, 0, 0, 0 }, // f
};

static volatile uint8_t seg_buffer[3][8];


static BuzzerPhase_t buzzer_phase = BuzzerPhase_Passive;

static io_handlers_t io_handlers;

static Buzzer_state_t buzzer_state = Buzzer_Off;




static void io_handler(void *pvParameters);

static void buzzer_run(void);

static void button_run(void);

static void ssi_run(void);


uint8_t io_init(io_handlers_t *handlers) {

	if(handlers->ButtonHandler == NULL || handlers->DoorHandler == NULL) {

		return 1;

	}

	memcpy(&io_handlers, handlers, sizeof(io_handlers_t));

	xTaskCreate(io_handler, "IO handler", configMINIMAL_STACK_SIZE * 5, NULL, 1, NULL);

}

void ToggleBuzzer(Buzzer_state_t newState);

void SetSSI(uint8_t seg1, uint8_t seg2, uint8_t seg3, uint8_t dots);





static void io_handler(void *pvParameters) {

	while(1) {

		buzzer_run();

		button_run();

		ssi_run();

		vTaskDelay(10);


	}


}

static void buzzer_run(void) {
	static uint32_t next_phase_change = 0;

	if (buzzer_state == Buzzer_Pulse && xTaskGetTickCount() > next_phase_change) {

		switch (buzzer_phase) {

		case BuzzerPhase_Active:

			next_phase_change = xTaskGetTickCount() + BUZZER_PERIOD_ACTIVE;

			HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_SET);

			buzzer_phase = BuzzerPhase_Passive;

			break;

		case BuzzerPhase_Passive:

			next_phase_change = xTaskGetTickCount() + BUZZER_PERIOD_PASSIVE;

			HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_RESET);

			buzzer_phase = BuzzerPhase_Active;

			break;

		default:

			next_phase_change = xTaskGetTickCount() + BUZZER_PERIOD_PASSIVE;

			HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_RESET);

			buzzer_phase = BuzzerPhase_Active;

		}

	} else {

		HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, GPIO_PIN_RESET);

	}

}

static void button_run(void) {
	static uint8_t checkCount[4] = { 0, 0, 0, 0 };;
	static uint8_t updateCounter = 0;

	if(updateCounter < BUTTON_UPDATE_COUNT) {

		checkCount[0] += (HAL_GPIO_ReadPin(button_GPIO_Port, button_Pin) != 0) ? 1 : 0;

		checkCount[1] += (HAL_GPIO_ReadPin(buttonA7_GPIO_Port, buttonA7_Pin) != 0) ? 1 : 0;

		checkCount[2] += (HAL_GPIO_ReadPin(buttonB0_GPIO_Port, buttonB0_Pin) != 0) ? 1 : 0;

		checkCount[3] += (HAL_GPIO_ReadPin(buttonB1_GPIO_Port, buttonB1_Pin) != 0) ? 1 : 0;

		updateCounter++;

	} else {

		if(io_handlers.ButtonHandler != NULL) {

			if(checkCount[0] > BUTTON_PRESS_OFFSET) {
				io_handlers.ButtonHandler(0);
			}

			if(checkCount[1] > BUTTON_PRESS_OFFSET) {
				io_handlers.ButtonHandler(1);
			}

			if(checkCount[2] > BUTTON_PRESS_OFFSET) {
				io_handlers.ButtonHandler(2);
			}

			if(checkCount[3] > BUTTON_PRESS_OFFSET) {
				io_handlers.ButtonHandler(3);
			}

		}

		checkCount[0] = 0;
		checkCount[1] = 0;
		checkCount[2] = 0;
		checkCount[3] = 0;

		updateCounter = 0;

	}

}

static void ssi_run(void) {
	static uint8_t sig_counter = 0;

	taskENTER_CRITICAL();

	HAL_GPIO_WritePin(segments_GPIO_Port, segments_Pin, seg_buffer[sig_counter][0]);
	HAL_GPIO_WritePin(segmentsB15_GPIO_Port, segmentsB15_Pin, seg_buffer[sig_counter][1]);
	HAL_GPIO_WritePin(segmentsA8_GPIO_Port, segmentsA8_Pin, seg_buffer[sig_counter][2]);
	HAL_GPIO_WritePin(segmentsA9_GPIO_Port, segmentsA9_Pin, seg_buffer[sig_counter][3]);
	HAL_GPIO_WritePin(segmentsA10_GPIO_Port, segmentsA10_Pin, seg_buffer[sig_counter][4]);
	HAL_GPIO_WritePin(segmentsA15_GPIO_Port, segmentsA15_Pin, seg_buffer[sig_counter][5]);
	HAL_GPIO_WritePin(segmentsB3_GPIO_Port, segmentsB3_Pin, seg_buffer[sig_counter][6]);
	HAL_GPIO_WritePin(segmentsB4_GPIO_Port, segmentsB4_Pin, seg_buffer[sig_counter][7]);

	switch(sig_counter) {

	case 0:

		HAL_GPIO_WritePin(seg_inB8_GPIO_Port, seg_inB8_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(seg_inB9_GPIO_Port, seg_inB9_Pin, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(seg_in_GPIO_Port, seg_in_Pin, GPIO_PIN_SET);

		sig_counter++;

		break;

	case 1:

		HAL_GPIO_WritePin(seg_in_GPIO_Port, seg_in_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(seg_inB9_GPIO_Port, seg_inB9_Pin, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(seg_inB8_GPIO_Port, seg_inB8_Pin, GPIO_PIN_SET);

		sig_counter++;

		break;

	case 2:

		HAL_GPIO_WritePin(seg_in_GPIO_Port, seg_in_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(seg_inB8_GPIO_Port, seg_inB8_Pin, GPIO_PIN_RESET);

		HAL_GPIO_WritePin(seg_inB9_GPIO_Port, seg_inB9_Pin, GPIO_PIN_SET);

		sig_counter = 0;

		break;

	default:

		sig_counter = 0;

	}

	taskEXIT_CRITICAL();


}