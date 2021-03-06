#include "pico/stdlib.h"
#include "reg.h"
#include "isr.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "data_capture.h"

#define PIN_DR 0

static enum gpio_irq_level irq_level;

/**
  * @brief Enables autonomous data capture by enabling DR ISR in NVIC.
  *
  * @return void
  *
  * This function does not configure the interrupt hardware at all. The config
  * must be performed by UpdateDRConfig prior to calling this function.
  */
void Data_Capture_Enable()
{
	if (g_regs[DIO_INPUT_CONFIG_REG] & DR_POLARITY_BITM) {
		irq_level = GPIO_IRQ_EDGE_RISE;
	} else {
		irq_level = GPIO_IRQ_EDGE_FALL;
	}
	/* Enable data ready interrupts */
	gpio_set_irq_enabled_with_callback(PIN_DR, irq_level, 1, ISR_Start_IMU_Burst);
}

/**
  * @brief disables autonomous data capture by disabling DR ISR.
  *
  * @return void
  *
  * This will stop a new capture from starting. A capture in progress
  * will still run to completion.
  */
void Data_Capture_Disable()
{
	/* Capture in progress set to false */
	g_captureInProgress = 0;

	/* Disable data ready interrupts */
	gpio_set_irq_enabled_with_callback(PIN_DR, irq_level, 0, ISR_Start_IMU_Burst);
}
