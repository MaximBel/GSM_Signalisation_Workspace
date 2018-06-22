/*
 * gsm_m950e.h
 *
 *  Created on: 11 θών. 2018 γ.
 *      Author: Max
 */

#ifndef GSM_M950E_H_
#define GSM_M950E_H_


#include "stdint.h"
#include "stddef.h"

typedef enum {
	OutCalling_Refused,
	OutCalling_Connected,
	OutCalling_NoAnswear,
	OutCalling_Error,
	OutCalling_Busy
} OutCalling_t;

typedef enum {
	ModemState_On,
	ModemState_Off,
	ModemState_Error,
	ModemState_Busy
} ModemStates_t;

typedef struct {
	void (*IncommingCall)(char *Number);
} ModemHandlers_t;

uint8_t gsm_init(ModemHandlers_t *handlers);

void ToggleModem(ModemStates_t State);

ModemStates_t GetModemState(void);

void CallToNumber(char *Number);

OutCalling_t GetCallingState(void);

uint8_t AddTrustedPhones(char *phoneOne, char *phoneTwo);

uint8_t MonitorIncommingCalls(void);



#endif /* GSM_M950E_H_ */
