/*
 * Logic.c
 *
 *  Created on: 19 ���. 2018 �.
 *      Author: Max
 */

#include "Logic.h"
#include "io.h"
#include "gsm_m950e.h"
#include "flash.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef enum {
	SignalisationState_Armed,
	SignalisationState_Disarmed
} SignalStates_t;

typedef enum {
	SignalisationPhase_Prepare,
	SignalisationPhase_Wait,
	SignalisationPhase_Pause,
	SignalisationPhase_Alarm
} SignalPhases_t;

#define SIGN_PREPARE_DELAY 	30000
#define SIGN_REACTION_DELAY 20000
#define SIGN_WAIT_DELAY 	60000
#define SIGN_CALL_DELAY		20000
#define SIGN_START_CALLING	3000
#define SIGN_START_BEEPING	10000

extern TaskHandle_t *logicTaskHandler;

static xQueueHandle buttonQueue;

static uint8_t password[4] = { 1, 2, 3, 4 };

static char trustedPhones[2][15];

static uint8_t doorState = 0;

static volatile uint8_t incommingCallFromTrust = 0;

static uint8_t callingPhase = 0;


static void LogicTask(void *pvParameters);

static void callHandler(char *number);
static void buttonHandler(uint8_t button);
static void doorHandler(uint8_t state);

static uint8_t checkPassword(void);
static uint8_t signalProc(void);
static uint8_t CallingProcessing(void);


void Logic_InitEverything(void) {
	io_handlers_t io_hand;
	ModemHandlers_t mod_hand;

	Flash_Init();

	mod_hand.IncommingCall = callHandler;

	io_hand.ButtonHandler = buttonHandler;
	io_hand.DoorHandler = doorHandler;

	//gsm_init(&mod_hand);
	//io_init(&io_hand);

	if(gsm_init(&mod_hand) == 0 &&
			io_init(&io_hand) == 0) {

		AddTrustedPhones("380950451110", "380950451110");

		buttonQueue = xQueueCreate(4, 1);

		if(xTaskCreate(LogicTask, "Logic task", configMINIMAL_STACK_SIZE * 3, NULL, 1, logicTaskHandler) == pdFAIL) {

			while(1);

		}

	}

}

static void LogicTask(void *pvParameters) {
	static SignalStates_t State = SignalisationState_Disarmed;

	while(1) {

		if(State == SignalisationState_Disarmed) {

			switch(checkPassword()) {

			case 0:

				State = SignalisationState_Armed;

				break;

			case 1:

				//do nothing

				break;

			case 2:

				// XXX: need to add displaying of temporary message "ERR" on ssi

				break;

			default:

				State = SignalisationState_Disarmed;

			}

		} else {

			if(signalProc() == 0) {

				State = SignalisationState_Disarmed;

			}

		}



		vTaskDelay(100);

	}

}

static void callHandler(char *number) {

	incommingCallFromTrust = 1;

}

static void buttonHandler(uint8_t button) {

	if(button <= 4) {

		xQueueSendToBack(buttonQueue, &button, NULL);

	}

}

static void doorHandler(uint8_t state) {

	if(doorState < 2) {

		doorState = state;

	}



}

static uint8_t checkPassword(void) {
	static uint8_t pass_buff[4] = { 0, 0, 0, 0 };
	static lastButtTime = 0;
	static buttonCounter = 0;

	if((xTaskGetTickCount() - lastButtTime) > 5000) {

		buttonCounter = 0;

	}

	if(xQueueReceive(buttonQueue, &pass_buff[buttonCounter], NULL) == pdTRUE) {

		lastButtTime = xTaskGetTickCount();

		if(buttonCounter >= 3) {

			buttonCounter = 0;

			if(pass_buff[0] == password[0] &&
					pass_buff[1] == password[1] &&
					pass_buff[2] == password[2] &&
					pass_buff[3] == password[3] ) {

				return 0;

			} else {

				return 2;

			}

		} else {

			buttonCounter++;

		}

	}

	return 1;

}

static uint8_t signalProc(void) {
	static SignalPhases_t phase = SignalisationPhase_Prepare;
	static uint32_t waitTimer = 0;
	static uint8_t callingNeed = 0;

	uint8_t passState = checkPassword();

	if(passState == 0) {

		// display OFF on ssi

		ToggleModem(ModemState_Off);

		ToggleBuzzer(Buzzer_Off);

		phase = SignalisationPhase_Prepare;

		waitTimer = 0;

		return 0;

	} else if(passState == 2) {

		// display ERR on ssi

	}

	if(incommingCallFromTrust == 1) {

		if(phase == SignalisationPhase_Wait) {

			// display noting on ssi

			incommingCallFromTrust = 0;

			phase = SignalisationPhase_Pause;

			waitTimer = xTaskGetTickCount();

			doorState = 0;

		}

		incommingCallFromTrust = 0;

		MonitorIncommingCalls();

	}

	switch(phase) {

	case SignalisationPhase_Prepare:

		// PRP on ssi

		if(waitTimer == 0) {

			ToggleModem(ModemState_On);

			waitTimer = xTaskGetTickCount();

		} else {

			if(((xTaskGetTickCount() - waitTimer) > SIGN_PREPARE_DELAY) && (GetModemState() == ModemState_On)) {

				if(MonitorIncommingCalls() == 0) {

					phase = SignalisationPhase_Wait;

					waitTimer = 0;

				}

			}

		}

		break;

	case SignalisationPhase_Wait:

		// ON on ssi

		if(doorState == 1) {

			waitTimer = xTaskGetTickCount();

			phase = SignalisationPhase_Alarm;

			callingNeed = 1;

		}


		break;

	case SignalisationPhase_Pause:

		if((xTaskGetTickCount() - waitTimer) > SIGN_WAIT_DELAY) {

			phase = SignalisationPhase_Wait;

		}

		break;

	case SignalisationPhase_Alarm:

		if(xTaskGetTickCount() - waitTimer > SIGN_START_BEEPING) {

			ToggleBuzzer(Buzzer_Pulse);

		}

		if(((xTaskGetTickCount() - waitTimer) > SIGN_START_CALLING) && (callingNeed == 1)) {

			if(CallingProcessing() == 0) {

				callingNeed = 0;

			}

		}

		break;

	default:

		phase = SignalisationPhase_Prepare;

	}

}


static uint8_t CallingProcessing(void) {

	switch(callingPhase) {

	case 0:

		CallToNumber(trustedPhones[0], SIGN_CALL_DELAY);

		callingPhase = 1;

		//break;

	case 1:

		if(GetCallingState() == OutCalling_Connected ||
				GetCallingState() == OutCalling_Refused) {

			callingPhase = 0;

			return 0;

		} else if(GetCallingState() == OutCalling_NoAnswear ||
				GetCallingState() == OutCalling_Error) {

			callingPhase = 2;

		}

		break;

	case 2:

		CallToNumber(trustedPhones[1], SIGN_CALL_DELAY);

		callingPhase = 3;

		//break;

	case 3:

		if(GetCallingState() == OutCalling_Connected ||
				GetCallingState() == OutCalling_Refused) {

			callingPhase = 0;

			return 0;

		} else if(GetCallingState() == OutCalling_NoAnswear ||
				GetCallingState() == OutCalling_Error) {

			callingPhase = 0;

		}

		break;

	default:

		callingPhase = 0;

	}

	return 1;

}