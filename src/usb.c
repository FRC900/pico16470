#include <stdio.h>
#include "pico/stdlib.h"
#include "usb.h"
#include "script.h"
#include "reg.h"

/** Current command string */
static uint8_t CurrentCommand[64];

/** Buffer for output of script */
static uint8_t OutputBuffer[512];

/** Script object (parsed from current command) */
static script scr = {};

/**
  * @brief Handler for received USB data
  *
  * @return void
  *
  * This function should be called periodically from
  * the main loop to check if new USB data has been received
  */
void USB_Rx_Handler()
{
	/* Track index within current command string */
	static uint32_t commandIndex = 0;

	uint32_t bufIndex;
	uint32_t numBytes;

	/* Iterate over all available characters */
	for (int c = getc(stdin);
		 c != EOF;
		 c = getc(stdin))
	{
		/* Backspace typed in console */
		if(c == '\b')
		{
			/* Move command index back one space */
			if(commandIndex > 0)
				commandIndex--;
			/* Replace character with space and put cursor on it */
			if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
			{
				USB_Tx_Handler("\b \b", 3);
			}
		}
		/* Carriage return char (end of command) */
		else if(c == '\r')
		{
			/* Send newline char if CLI echo is enabled */
			if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
			{
				USB_Tx_Handler("\r\n", 2);
			}
			/* Place a string terminator */
			CurrentCommand[commandIndex] = 0;
			/* Parse command */
			Script_Parse_Element(CurrentCommand, &scr);
			/* Execute command */
			Script_Run_Element(&scr, OutputBuffer);
			//printf("Parse command: %s\r\n", CurrentCommand);

			/* Clear command buffer */
			for(int i = 0; i < sizeof(CurrentCommand); i++) {
				CurrentCommand[i] = 0;
			}
			commandIndex = 0;
			return;
		}
		else
		{
			/* Add char to current command */
			if(commandIndex < 64)
			{
				CurrentCommand[commandIndex] = c;
				commandIndex++;
			}
			/* Echo to console */
			if(!(g_regs[CLI_CONFIG_REG] & USB_ECHO_BITM))
			{
				USB_Tx_Handler((uint8_t*)&c, 1);
			}
		}
	}
}

/**
  * @brief USB write handler
  *
  * @param buf Buffer containing data to write
  *
  * @param count Number of bytes to write
  *
  * @return void
  *
  * This function is called by the script execution
  * routines, when a script object is executed
  * from a USB CLI context.
  */
void USB_Tx_Handler(const uint8_t* buf, uint32_t count)
{
	if(count == 0)
		return;
	fwrite(buf, count, 1, stdout);

	/* Put the stdout buffer in a known empty state */
	fflush(stdout);
}
