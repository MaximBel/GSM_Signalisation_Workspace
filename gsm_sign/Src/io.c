/*
 * io.c
 *
 *  Created on: 13 θών. 2018 γ.
 *      Author: Max
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "io.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "string.h"

#define BUZZER_PERIOD_ACTIVE 700

#define BUZZER_PERIOD_PASSIVE 500

#define BUTTON_UPDATE_COUNT 20

#define DOOR_UPDATE_COUNT 200

#define BUTTON_PRESS_OFFSET (BUTTON_UPDATE_COUNT * 0.8)

typedef enum {
	BuzzerPhase_Active,
	BuzzerPhase_Passive
} BuzzerPhase_t;

extern TaskHandle_t *ioTaskHandler;

static const uint8_t sig_patterns[17][8] = {
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
		{ 1, 1, 0, 1, 1, 0, 1, 0 }, // e
		{ 1, 1, 1, 0, 1, 0, 0, 0 }, // r
		{ 1, 1, 1, 1, 1, 0, 0, 0 },	// p
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	// nothing
};

static volatile uint8_t seg_buffer[3][8];


static BuzzerPhase_t buzzer_phase = BuzzerPhase_Passive;

static io_handlers_t io_handlers;

static Buzzer_state_t buzzer_state = Buzzer_Off;


static void io_handler(void *pvParameters);

static void buzzer_run(void);

static void button_run(void);

static void ssi_run(void);

static void door_run(void);

uint8_t io_init(io_handlers_t *handlers) {

	if(handlers->ButtonHandler == NULL || handlers->DoorHandler == NULL) {

		return 1;

	}

	memcpy(&io_handlers, handlers, sizeof(io_handlers_t));

	if(xTaskCreate(io_handler, "IO handler", configMINIMAL_STACK_SIZE * 3, NULL, 1, ioTaskHandler) == pdFAIL) {

		return 1;

	}

	return 0;

}

void ToggleBuzzer(Buzzer_state_t newState) {

	if((newState == Buzzer_Pulse) && (buzzer_state != newState)) {

		buzzer_state = newState;

		buzzer_phase = BuzzerPhase_Passive;

	} else {

		buzzer_state = Buzzer_Off;

	}

}

void SetSSI(uint8_t seg1, uint8_t seg2, uint8_t seg3, uint8_t dots) {

	memcpy((uint8_t *)&seg_buffer[0], &sig_patterns[seg1], 8);
	memcpy((uint8_t *)&seg_buffer[1], &sig_patterns[seg2], 8);
	memcpy((uint8_t *)&seg_buffer[2], &sig_patterns[seg3], 8);

	seg_buffer[0][7] = ((dots & 0x01) != 0) ? 1 : 0;
	seg_buffer[1][7] = ((dots & 0x02) != 0) ? 1 : 0;
	seg_buffer[2][7] = ((dots & 0x04) != 0) ? 1 : 0;

}





static void io_handler(void *pvParameters) {

	while(1) {

		buzzer_run();

		button_run();

		ssi_run();

		door_run();

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
	static uint8_t checkCount[4] = { 0, 0, 0, 0 };
	static uint8_t lastState[4] = { 0, 0, 0, 0 };
	static uint8_t updateCounter = 0;

	if(updateCounter < BUTTON_UPDATE_COUNT) {

		checkCount[0] += (HAL_GPIO_ReadPin(button_GPIO_Port, button_Pin) != 0) ? 1 : 0;

		checkCount[1] += (HAL_GPIO_ReadPin(buttonA7_GPIO_Port, buttonA7_Pin) != 0) ? 1 : 0;

		checkCount[2] += (HAL_GPIO_ReadPin(buttonB0_GPIO_Port, buttonB0_Pin) != 0) ? 1 : 0;

		checkCount[3] += (HAL_GPIO_ReadPin(buttonA6_GPIO_Port, buttonA6_Pin) != 0) ? 1 : 0;

		updateCounter++;

	} else {

		if(io_handlers.ButtonHandler != NULL) {

			if((checkCount[0] > BUTTON_PRESS_OFFSET) && (lastState[0] == 0)) {
				io_handlers.ButtonHandler(1);
				lastState[0] = 1;
			} else if(checkCount[0] < BUTTON_PRESS_OFFSET){
				lastState[0] = 0;
			}

			if((checkCount[1] > BUTTON_PRESS_OFFSET) && (lastState[1] == 0)) {
				io_handlers.ButtonHandler(2);
				lastState[1] = 1;
			} else if(checkCount[1] < BUTTON_PRESS_OFFSET){
				lastState[1] = 0;
			}

			if((checkCount[2] > BUTTON_PRESS_OFFSET) && (lastState[2] == 0)) {
				io_handlers.ButtonHandler(3);
				lastState[2] = 1;
			} else if(checkCount[2] < BUTTON_PRESS_OFFSET){
				lastState[2] = 0;
			}

			if((checkCount[3] > BUTTON_PRESS_OFFSET) && (lastState[3] == 0)) {
				io_handlers.ButtonHandler(4);
				lastState[3] = 1;
			} else if(checkCount[3] < BUTTON_PRESS_OFFSET){
				lastState[3] = 0;
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

static void door_run(void) {
	static uint8_t checkCount = 0;
	static uint8_t updateCounter = 0;
	static uint8_t lastState = 0;

	if (updateCounter < DOOR_UPDATE_COUNT) {

		checkCount += (HAL_GPIO_ReadPin(door_GPIO_Port, door_Pin) != 0) ? 1 : 0;

		updateCounter++;

	} else {

		if (io_handlers.DoorHandler != NULL) {

			if (((DOOR_UPDATE_COUNT - checkCount) < (DOOR_UPDATE_COUNT / 2)) && (lastState == 1)) {

				io_handlers.DoorHandler(0); // closed

				lastState = 0;

			} else if(lastState == 0){

				lastState = 1;

				io_handlers.DoorHandler(1); // opened

			}

		}

		checkCount = 0;

		updateCounter = 0;

	}

}
