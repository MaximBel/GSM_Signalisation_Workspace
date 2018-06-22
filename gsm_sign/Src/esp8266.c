/*
 * esp8266.c
 *
 *  Created on: 22 θών. 2018 γ.
 *      Author: Max
 */

#include "esp8266.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "stm32f1xx_hal.h"
#include "usart.h"
#include "Logic.h"
#include "flash.h"


extern TaskHandle_t *espTaskHandler;

static const char esp_commands[12][20] = {
		"AT+RBT",		// reboot
		"AT+TNBR=",		// trusted numbers AT+TNBR=380950451110,380950451110
		"AT+PAUSE",		// pause signalisation
		"AT+SPREP=",	// sign prepare delay
		"AT+SP=",		// sign pause delay
		"AT+SSC=",		// sign start calling
		"AT+SSB=",		// sign start beeping
		"AT+CAT=",		// call abort timeout
		"AT+BUZZ=",		// infinite buzz active/passive period AT+BUZZ=1500,1000
		"AT+BUZZS=",	// the same as AT+BUZZ but for some beeps
		"AT+DUC=",		// door update count
		"AT+PWD=",		// password
};

static uint8_t esp_buffer[200] = { 0 };

static char *dataPointer;
static SetupStruct_t temp_setup;
static char trustedPhones[2][20] = { 0 };
static uint8_t temp_pwd[4];


static void ESPtask(void *pvParameters);

static void flush_rx(void);

static uint8_t send_command(char *command, char *responce);

uint8_t esp_init(void) {

	if(xTaskCreate(ESPtask, "ESP task", configMINIMAL_STACK_SIZE * 2, NULL, 1, espTaskHandler) == pdFAIL) {

				return 1;

	}

	return 0;

}




static void ESPtask(void *pvParameters) {

	HAL_GPIO_WritePin(ch_pd_GPIO_Port, ch_pd_Pin, GPIO_PIN_SET);

	vTaskDelay(3000);

	if(send_command("ATE0\r\n", "OK\r\n") == 0 &&
			send_command("AT+CWMODE_CUR=2\r\n", "OK\r\n") == 0 &&
			send_command("AT+CWSAP_CUR=\"ESP\",\"1234567890\",1,3\r\n", "OK\r\n") == 0 &&
			send_command("AT+CIPAP_CUR=\"192.168.0.1\"\r\n", "OK\r\n") == 0 &&
			send_command("AT+CIPMUX=1\r\n", "OK\r\n") == 0 &&
			send_command("AT+CIPSERVER=1,8080\r\n", "OK\r\n") == 0 &&
			send_command("AT+CIPSTO=3600\r\n", "OK\r\n") == 0 &&
			send_command("AT+CWMODE_CUR=2\r\n", "OK\r\n") == 0) {

		asm("NOP");

	}

	while(1) {

		flush_rx();

		HAL_UART_Receive(&huart2, esp_buffer, sizeof(esp_buffer), 1000);

		if(strstr(esp_buffer, &esp_commands[0][0]) != NULL) {

			Logic_Reboot();

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[1][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			if (sscanf(dataPointer, "%12s,%12s", &trustedPhones[0], &trustedPhones[1]) != NULL) {

				Flash_ReadSetup(&temp_setup);

				memcpy(temp_setup.firstPhone, &trustedPhones[0], strlen(&trustedPhones[0]));
				memcpy(temp_setup.secondPhone, &trustedPhones[1], strlen(&trustedPhones[1]));

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[2][0]) != NULL) {

			Logic_PauseSignalisation();

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[3][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data = 0;

			if(sscanf(dataPointer, "%d", &data) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.signPrepareDelay = data;

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[4][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data = 0;

			if (sscanf(dataPointer, "%d", &data) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.signPauseDelay = data;

				Flash_WriteSetup(&temp_setup);

			}

		//	goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[5][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data = 0;

			if (sscanf(dataPointer, "%d", &data) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.signStartCallDelay = data;

				Flash_WriteSetup(&temp_setup);

			}

		//	goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[6][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data = 0;

			if (sscanf(dataPointer, "%d", &data) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.signStartBeepDelay = data;

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[7][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data = 0;

			if (sscanf(dataPointer, "%d", &data) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.callAbortTimeout = data;

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[8][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data[2] = { 0 };

			if (sscanf(dataPointer, "%d,%d", &data[0], &data[1]) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.buzzerActive = data[0];
				temp_setup.buzzerPassive = data[1];

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[9][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data[2] = { 0 };

			if (sscanf(dataPointer, "%d,%d", &data[0], &data[1]) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.buzzerActimeSome = data[0];
				temp_setup.buzzerPassiveSome = data[1];

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if(strstr(esp_buffer, &esp_commands[10][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			uint32_t data = 0;

			if (sscanf(dataPointer, "%d", &data) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.doorUpdateCount = data;

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

		if (strstr(esp_buffer, &esp_commands[11][0]) != NULL) {

			dataPointer = strstr(esp_buffer, "=") + 1;

			if (sscanf(dataPointer, "%d,%d,%d,%d", &temp_pwd[0], &temp_pwd[1], &temp_pwd[2], &temp_pwd[3]) != NULL) {

				Flash_ReadSetup(&temp_setup);

				temp_setup.password[0] = temp_pwd[0];
				temp_setup.password[1] = temp_pwd[1];
				temp_setup.password[2] = temp_pwd[2];
				temp_setup.password[3] = temp_pwd[3];

				Flash_WriteSetup(&temp_setup);

			}

			//goto loopStart;

		}

	}

}

static void flush_rx(void) {

	//HAL_UART_AbortReceive_IT(&huart2);

	memset(esp_buffer, 0, sizeof(esp_buffer));

}

static uint8_t send_command(char *command, char *responce) {
	char temp_responce[60];

	sprintf(temp_responce, "%s", command);

	flush_rx();

	HAL_UART_Transmit_IT(&huart2, command, strlen(command));

	HAL_UART_Receive(&huart2, esp_buffer, sizeof(esp_buffer), 2000);

	if(strstr(esp_buffer, responce) != NULL) {

		return 0;

	} else {

		return 1;

	}

}


//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
