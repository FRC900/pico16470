#ifndef INC_TIMER_H_
#define INC_TIMER_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

void Timer_Init();
void Timer_Clear_Microsecond_Timer();
uint32_t Timer_Get_Microsecond_Timestamp();
uint32_t Timer_Get_Millisecond_Uptime();

#endif /* INC_TIMER_H_ */