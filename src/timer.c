#include "pico/stdlib.h"
#include "reg.h"
#include "timer.h"

/* Pico timer doesn't support resets, so we subtract this timestamp */
static absolute_time_t Last_Reset;

/**
  * @brief Initialize all timers for operation.
  *
  * @return void
  *
  * Timer 2 is used as a 1MHz timer for the timestamp measurements
  * Timer 8 is used as a timeout timer, also at 1MHz
  *
  * The DWT timer is used for a polling based timer
  */
void Timer_Init()
{
	/* Last reset was at 0 us */
	update_us_since_boot(&Last_Reset, 0);
}

/**
  * @brief Gets the current 32-bit value from the internal timer
  *
  * @return The timer value
  */
uint32_t Timer_Get_Millisecond_Uptime()
{
	return to_ms_since_boot(get_absolute_time());
}

/**
  * @brief Gets the current 32-bit value from the internal timer
  *
  * @return The timer value
  */
uint32_t Timer_Get_Microsecond_Timestamp()
{
	/* Time between last reset call and now */
	return absolute_time_diff_us(Last_Reset, get_absolute_time());
}

/**
  * @brief Reset TIM2 counter to 0.
  *
  * @return void
  */
void Timer_Clear_Microsecond_Timer()
{
	Last_Reset = get_absolute_time();
}
