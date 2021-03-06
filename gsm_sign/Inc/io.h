/*
 * io.h
 *
 *  Created on: 13 ���. 2018 �.
 *      Author: Max
 */

#ifndef IO_H_
#define IO_H_

#include "stddef.h"
#include "stdint.h"

typedef enum {
	Buzzer_Pulse,
	Buzzer_SomePulse,
	Buzzer_Off
} Buzzer_state_t;

typedef struct {
	void (*DoorHandler)(uint8_t);
	void (*ButtonHandler)(uint8_t);
} io_handlers_t;


uint8_t io_init(io_handlers_t *handlers);

void ToggleBuzzer(Buzzer_state_t newState);

void Buzzer_ContinuouslyBeep(void);

void Buzzer_SomeBeeps(uint8_t count);

void Buzzer_Silent(void);

void SetSSI(uint8_t seg1, uint8_t seg2, uint8_t seg3, uint8_t dots);



#endif /* IO_H_ */
