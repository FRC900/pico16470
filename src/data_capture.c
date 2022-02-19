#include "pico/stdlib.h"
#include "reg.h"
#include "isr.h"
#include "hardware/irq.h"
#include "data_capture.h"

/* DR pin goes from low to high when registers are updated */
#define IRQ_LEVEL GPIO_IRQ_EDGE_RISE
#define PIN_LED   25
#define PIN_DR    20

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
	/* TODO: Use DIO config register to control interrupt behavior */
	/* Enable data ready interrupts */
	gpio_set_irq_enabled_with_callback(PIN_DR, IRQ_LEVEL, 1, ISR_Start_IMU_Burst);
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
	/* TODO: Use DIO config register to control interrupt behavior */
	/* Capture in progress set to false */
	g_captureInProgress = 0;

	/* Disable data ready interrupts */
	gpio_set_irq_enabled_with_callback(PIN_DR, IRQ_LEVEL, 0, ISR_Start_IMU_Burst);
}
