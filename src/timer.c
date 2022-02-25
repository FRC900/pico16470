#include "pico/stdlib.h"
#include "reg.h"
#include "timer.h"
#include "isr.h"

#define PIN_PPS 20

/* Private function prototypes */
static void ConfigurePPSPins(uint32_t enable);

/* Pico timer doesn't support resets, so we subtract this timestamp */
static absolute_time_t Last_Reset;

/** Track number of "PPS" ticks which have occurred in last second */
static uint32_t PPS_TickCount;

/** Number of PPS ticks per sec */
static uint32_t PPS_MaxTickCount;

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

	/* Init PPS pin */
    gpio_init(PIN_PPS);
    gpio_set_dir(PIN_PPS, GPIO_IN);
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
  * @brief Gets the current 32-bit value from PPS timestamp registers
  *
  * @return The PPS counter value
  */
uint32_t Timer_Get_PPS_Timestamp()
{
	return (g_regs[UTC_TIMESTAMP_LWR_REG] | (g_regs[UTC_TIMESTAMP_UPR_REG] << 16));
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

/**
  * @brief Increment PPS timestamp by 1
  *
  * @return void
  *
  * This function should be called whenever a PPS interrupt is generated. It
  * handles PPS tick count incrementation, and resetting the microsecond timestamp
  * every time one second has elapsed.
  */
void Timer_Increment_PPS_Time()
{
	printf("Increment PPS Time\r\n");

	uint32_t internalTime, startTime;

	/* Increment tick count and check if one second has passed */
	PPS_TickCount++;
	if(PPS_TickCount >= PPS_MaxTickCount)
	{
		/* Get the internal timestamp for period measurement */
		internalTime = Timer_Get_Microsecond_Timestamp();

		/* Clear microsecond tick counter */
		Timer_Clear_Microsecond_Timer();

		/* Reset tick count for next second */
		PPS_TickCount = 0;

		/* Get starting PPS timestamp and increment */
		startTime = Timer_Get_PPS_Timestamp();
		startTime++;
		g_regs[UTC_TIMESTAMP_LWR_REG] = startTime & 0xFFFF;
		g_regs[UTC_TIMESTAMP_UPR_REG] = (startTime >> 16);

		/* Check for unlock: If the PPS period is outside of 1 second +- 10ms (1%) then flag PPS unlock error */
		if((internalTime > 1010000) || (internalTime < 990000))
		{
			g_regs[STATUS_0_REG] |= STATUS_PPS_UNLOCK;
			g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
		}
	}
}

/**
  * @brief Enable PPS timer functionality
  *
  * @return void
  */
void Timer_Enable_PPS()
{
	ConfigurePPSPins(1);
}

/**
  * @brief Disable PPS timer functionality
  *
  * @return void
  */
void Timer_Disable_PPS()
{
	ConfigurePPSPins(0);
}

/**
  * @brief Configure PPS timer based on DIO_INPUT_CONFIG value
  *
  * @param enable Bool flagging if PPS functionality is being enabled or disabled
  *
  * This function sets up the PPS input pin, as well as the expected "PPS" period.
  */
static void ConfigurePPSPins(uint32_t enable)
{
	/* Get current config value */
	uint32_t config = g_regs[DIO_INPUT_CONFIG_REG];

	/* Set freq values */
	switch(config & PPS_FREQ_BITM)
	{
	case 0:
		/* 1Hz sync */
		PPS_MaxTickCount = 1;
		break;
	case 1:
		/* 1Hz sync */
		PPS_MaxTickCount = 10;
		break;
	case 2:
		/* 1Hz sync */
		PPS_MaxTickCount = 100;
		break;
	case 3:
		/* 1Hz sync */
		PPS_MaxTickCount = 1000;
		break;
	default:
		/* 1Hz sync */
		PPS_MaxTickCount = 1;
		break;
	}

	/* Init tick count to 0 */
	PPS_TickCount = 0;

	/* Initialize based on polarity.
	 * 0x04: GPIO_IRQ_EDGE_FALL
	 * 0x08: GPIO_IRQ_EDGE_RISE */
	enum gpio_irq_level irq_level = 0x04 << (config & PPS_POLARITY_BITM);
	gpio_set_irq_enabled_with_callback(PIN_PPS, irq_level, enable, Timer_Increment_PPS_Time);
}
