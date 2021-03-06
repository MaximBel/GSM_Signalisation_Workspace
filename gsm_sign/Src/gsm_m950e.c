/*
 * gsm_m950e.c
 *
 *  Created on: 11 ���. 2018 �.
 *      Author: Max
 */

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include "string.h"
#include "flash.h"
#include "gsm_m950e.h"

#define CALL_WAITON_TIMEOUT 120000

extern TaskHandle_t *gsmTaskHandler;

static char command_pwroff[] = "AT+CPWROFF\r\n";

static char command_chkntw[] = "AT+COPS?\r\n";

static char command_abortcall[] = "ATH\r\n";

static char command_inccall[] = "AT+CLIP=1\r\n";

static char responce_ready[] = "+PBREADY";

static char responce_chkntw[] = "+COPS: 0";


typedef enum {
	GSM_RunState_Idle,
	GSM_RunState_TurnOn_Start,
	GSM_RunState_TurnOff,
	GSM_RunState_WaitSignal,
	GSM_RunState_Call,
	GSM_RunState_CheckCalling,
	GSM_RunState_WaitAbort,
	GSM_RunState_MonitorIncomming_Start,
	GSM_RunState_MonitorIncomming,
	GSM_RunState_Empty
} GSM_RunStates_t;

static callAbortTimeout;

static ModemStates_t modem_state = ModemState_Off;

static GSM_RunStates_t RunState = GSM_RunState_Idle;
static GSM_RunStates_t newState = GSM_RunState_Empty;
static OutCalling_t callingState = OutCalling_Busy;

static uint8_t uart_buffer[100];

static char call_number_buffer[20];

static volatile char trustedPhones[2][15];

static ModemHandlers_t gsm_handlers;


static void gsm_task(void * pvParameters);

static uint8_t modem_init(void);

static void modem_run(void);

static void flush_rx(void);


uint8_t gsm_init(ModemHandlers_t *handlers) {
	SetupStruct_t setup;

	Flash_ReadSetup(&setup);

	callAbortTimeout = setup.callAbortTimeout;

	memcpy(&gsm_handlers, handlers, sizeof(ModemHandlers_t));

	if(xTaskCreate(gsm_task, "GSM task", configMINIMAL_STACK_SIZE * 3, NULL, 1, gsmTaskHandler) == pdFAIL) {

		return 1;

	}

	return 0;

}

void ToggleModem(ModemStates_t State) {

	if(State == ModemState_On) {

		if(modem_state != ModemState_On) {

			newState = GSM_RunState_TurnOn_Start;

		}

	} else {

		if (modem_state != ModemState_Off) {

			newState = GSM_RunState_TurnOff;

		}

	}

}

ModemStates_t GetModemState(void) {

	return modem_state;

}

void CallToNumber(char *Number) {

	if(modem_state == ModemState_On) {

		sprintf(call_number_buffer, "ATD+%s;\r\n", Number);

		callingState = OutCalling_Busy;

		newState = GSM_RunState_Call;

	} else {

		callingState = OutCalling_Error;

	}

}

OutCalling_t GetCallingState(void) {

	return callingState;

}

uint8_t AddTrustedPhones(char *phoneOne, char *phoneTwo){

	if(strstr(phoneOne, "380") != NULL && strstr(phoneTwo, "380") != NULL && RunState != GSM_RunState_MonitorIncomming_Start) {

		sprintf(trustedPhones[0], "%s", phoneOne);

		sprintf(trustedPhones[1], "%s", phoneTwo);

		return 0;

	}

	return 1;

}

uint8_t MonitorIncommingCalls(void) {

	if(modem_state == ModemState_On && RunState != GSM_RunState_Idle) {

		newState = GSM_RunState_MonitorIncomming_Start;

		return 0;

	}

	return 1;

}


//---------STATIC-------//

static void gsm_task(void * pvParameters) {

	if(modem_init() == 1) {

		HAL_GPIO_WritePin(led13_GPIO_Port, led13_Pin, GPIO_PIN_SET);

		while(1) {

			vTaskDelay(10000);

		}

	}

	HAL_GPIO_WritePin(led13_GPIO_Port, led13_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(Boot_output_GPIO_Port, Boot_output_Pin, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart3, (uint8_t *) command_pwroff, sizeof(command_pwroff), 1000);
	modem_state = ModemState_Off;

	while(1) {

		modem_run();

		vTaskDelay(10);

	}


}

static uint8_t modem_init(void) {
	uint8_t try_counter;
	uint8_t inner_counter;

	flush_rx();

	for(try_counter = 0; try_counter < 5; try_counter++) {

		HAL_UART_Receive_IT(&huart3, uart_buffer, sizeof(uart_buffer));

		HAL_GPIO_WritePin(Boot_output_GPIO_Port, Boot_output_Pin, GPIO_PIN_SET);

		vTaskDelay(1000);

		HAL_GPIO_WritePin(Boot_output_GPIO_Port, Boot_output_Pin, GPIO_PIN_RESET);

		vTaskDelay(1000);

		HAL_GPIO_WritePin(Boot_output_GPIO_Port, Boot_output_Pin, GPIO_PIN_SET);

		for(inner_counter = 0;inner_counter < 9; inner_counter++) {

			vTaskDelay(5000);

			if (strstr(uart_buffer, responce_ready) != NULL) {

				flush_rx();

				return 0;

			}

		}

		HAL_GPIO_WritePin(Boot_output_GPIO_Port, Boot_output_Pin, GPIO_PIN_SET);

		HAL_UART_AbortReceive_IT(&huart3);

	}

	return 1;

}

static void modem_run(void) {
	static uint32_t Call_Time;
	static uint32_t TurnOn_Time = 0;

	if(newState != GSM_RunState_Empty) {

		RunState = newState;

		newState = GSM_RunState_Empty;

	}

	switch (RunState) {

	case GSM_RunState_Idle:

		//do nothing

		break;

		//------------Turn on-----------------//

	case GSM_RunState_TurnOn_Start:

		if(modem_init() == 0) {

			HAL_UART_Receive_IT(&huart3, uart_buffer, sizeof(uart_buffer));

			if(HAL_UART_Transmit(&huart3, command_chkntw, sizeof(command_chkntw), 50) != HAL_OK) {

				modem_state = ModemState_Error;

				RunState = GSM_RunState_Idle;

				return;

			}

			TurnOn_Time = xTaskGetTickCount();

			RunState = GSM_RunState_WaitSignal;

		} else {

			modem_state = ModemState_Error;

			RunState = GSM_RunState_Idle;

		}

		break;

	case GSM_RunState_WaitSignal:

		if (strstr(uart_buffer, responce_chkntw) != NULL) {

			modem_state = ModemState_On;

			RunState = GSM_RunState_Idle;

		} else {

			if (xTaskGetTickCount() - TurnOn_Time > CALL_WAITON_TIMEOUT) {

				modem_state = ModemState_Error;

				RunState = GSM_RunState_Idle;

			} else {

				flush_rx();

				HAL_UART_Receive_IT(&huart3, uart_buffer, sizeof(uart_buffer));

				if(HAL_UART_Transmit(&huart3, command_chkntw, sizeof(command_chkntw), 50) != HAL_OK) {
					modem_state = ModemState_Error;
					RunState = GSM_RunState_Idle;
					return;

				}

				vTaskDelay(1000);

				RunState = GSM_RunState_WaitSignal;

			}

		}

		break;

		//------------Turn on-----------------//



	case GSM_RunState_Call:

		flush_rx();

		HAL_UART_Receive_IT(&huart3, uart_buffer, sizeof(uart_buffer));

		if(HAL_UART_Transmit(&huart3, call_number_buffer, strlen(call_number_buffer), 50) != HAL_OK) {
			modem_state = ModemState_Error;
			RunState = GSM_RunState_Idle;
			callingState = OutCalling_Error;
			return;

		}

		vTaskDelay(1000);

		if(strstr(uart_buffer, "OK") != NULL) {

			Call_Time = xTaskGetTickCount();

			flush_rx();

			HAL_UART_Receive_IT(&huart3, uart_buffer, sizeof(uart_buffer));

			RunState = GSM_RunState_WaitAbort;

		} else {



		}

		break;

	case GSM_RunState_WaitAbort:

		if (xTaskGetTickCount() - Call_Time > callAbortTimeout) {

			if (HAL_UART_Transmit(&huart3, command_abortcall, strlen(command_abortcall), 50) != HAL_OK) {
				modem_state = ModemState_Error;
				callingState = OutCalling_Error;
				RunState = GSM_RunState_Idle;
				return;

			}

			callingState = OutCalling_NoAnswear;

			RunState = GSM_RunState_Idle;

		} else {

			if((strstr(uart_buffer, "CONNECT") != NULL) ) {

				callingState = OutCalling_Connected;

				if (HAL_UART_Transmit(&huart3, command_abortcall, strlen(command_abortcall), 50) != HAL_OK) {
					modem_state = ModemState_Error;
					RunState = GSM_RunState_Idle;
					return;

				}

				RunState = GSM_RunState_Idle;

			} else if((strstr(uart_buffer, "NO CARRIER") != NULL)) {

				callingState = OutCalling_Refused;

				if (HAL_UART_Transmit(&huart3, command_abortcall, strlen(command_abortcall), 50) != HAL_OK) {
					modem_state = ModemState_Error;
					RunState = GSM_RunState_Idle;
					return;

				}

				RunState = GSM_RunState_Idle;

			}


		}

		break;

	//------------modem off-----------------//

	case GSM_RunState_TurnOff:

		HAL_GPIO_WritePin(Boot_output_GPIO_Port, Boot_output_Pin, GPIO_PIN_SET);

		HAL_UART_Transmit(&huart3, (uint8_t *) command_pwroff, sizeof(command_pwroff), 1000);

		modem_state = ModemState_Off;

		RunState = GSM_RunState_Idle;

		break;

	//------------modem check incomming------------------//

	case GSM_RunState_MonitorIncomming_Start:

		flush_rx();

		HAL_UART_Receive_IT(&huart3, uart_buffer, sizeof(uart_buffer));

		if (HAL_UART_Transmit(&huart3, command_inccall, sizeof(command_inccall), 50) != HAL_OK) {
			modem_state = ModemState_Error;
			RunState = GSM_RunState_Idle;
			return;
		}

		RunState = GSM_RunState_MonitorIncomming;

		break;

	case GSM_RunState_MonitorIncomming:

		if (strstr(uart_buffer, "RING") != NULL) {

			if (strstr(uart_buffer, (char *)trustedPhones[0]) != NULL) {

				gsm_handlers.IncommingCall((char *)trustedPhones[0]);

			} else if (strstr(uart_buffer, (char *)trustedPhones[1]) != NULL) {

				gsm_handlers.IncommingCall((char *)trustedPhones[1]);

			}

			if (HAL_UART_Transmit(&huart3, command_abortcall, strlen(command_abortcall), 50) != HAL_OK) {
				modem_state = ModemState_Error;
				RunState = GSM_RunState_Idle;
				return;

			}

			RunState = GSM_RunState_Idle;

		}

	default:

		RunState = GSM_RunState_TurnOff;

	}

}

static void flush_rx(void) {

	HAL_UART_AbortReceive_IT(&huart3);

	memset(uart_buffer, 0, sizeof(uart_buffer));

}
