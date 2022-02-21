#include "reg.h"
#include "imu.h"
#include "timer.h"
#include "buffer.h"

/* Local function prototypes */
static uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue);
static void GetSN();
static void GetBuildDate();

/** Register update flags for main loop processing. Global scope */
volatile uint32_t g_update_flags = 0;

/** Pointer to buffer entry. Will be 0 if no buffer entry "loaded" to output registers */
volatile uint16_t* g_CurrentBufEntry;

/** iSensor-SPI-Buffer global register array (read-able via SPI). Global scope */
volatile uint16_t g_regs[NUM_REG_PAGES * REG_PER_PAGE] __attribute__((aligned (32))) = {

/* Page 252 (volatile, currently unusued) */
OUTPUT_PAGE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x00 - 0x07 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08 - 0x1F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x10 - 0x17 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x18 - 0x1F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x20 - 0x27 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x28 - 0x2F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x30 - 0x37 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x38 - 0x3F */

/* Page 253 */
BUF_CONFIG_PAGE, /* 0x40 */
BUF_CONFIG_DEFAULT, /* 0x41 */
BUF_LEN_DEFAULT, /* 0x42 */
BTN_CONFIG_DEFAULT, /* 0x43 */
DIO_INPUT_CONFIG_DEFAULT, /* 0x44 */
DIO_OUTPUT_CONFIG_DEFAULT, /* 0x45 */
WATER_INT_CONFIG_DEFAULT, /* 0x46 */
ERROR_INT_CONFIG_DEFAULT, /* 0x47 */
IMU_SPI_CONFIG_DEFAULT, /* 0x48 */
USER_SPI_CONFIG_DEFAULT, /* 0x49 */
CLI_CONFIG_DEFAULT, /* 0x4A */
0x0000, /* 0x4B (command) */
SYNC_FREQ_DEFAULT, /* 0x4C */
0x0000, 0x0000, 0x0000, /* 0x4D - 0x4F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x50 - 0x57 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x58 - 0x5F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x60 - 0x67 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x68 - 0x6F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, FW_REV_DEFAULT, /* 0x70 - 0x77 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x78 - 0x7F */

/* Page 254 */
BUF_WRITE_PAGE, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, /* 0x80 - 0x87 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x88 - 0x8F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x90 - 0x97 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x98 - 0x9F */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xA0 - 0xA7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xA8 - 0xAF */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xB0 - 0xB7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, FLASH_SIG_DEFAULT, /* 0xB8 - 0xBF */

/* Page 255 */
BUF_READ_PAGE, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, /* 0xC0 - 0xC7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xC8 - 0xCF */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xD0 - 0xD7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xD8 - 0xDF */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xE0 - 0xE7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xE8 - 0xEF */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xF0 - 0xF7 */
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0xF8 - 0xFF */
};

/** Selected page. Starts on 253 (config page) */
static volatile uint32_t selected_page = BUF_CONFIG_PAGE;

/**
  * @brief Dequeues an entry from the buffer and loads it to the primary output registers
  *
  * @return void
  *
  * This function is called from the main loop to preserve SPI responsiveness while
  * a buffer entry is being dequeued into the output registers. This allows a user to read
  * the buffer contents while the values are being moved (if they start reading at buffer
  * entry 0). After moving all values to the correct location in the output register array,
  * the function sets up the burst read DMA (if enabled in user SPI config).
  */
void Reg_Buf_Dequeue_To_Outputs()
{
	/* Check if buf count > 0) */
	if(g_regs[BUF_CNT_0_REG] > 0)
	{
		g_CurrentBufEntry = (uint16_t *) Buffer_Take_Element();
	}
	else
	{
		g_CurrentBufEntry = 0;
	}
}

/**
  * @brief Process a register read request (from master)
  *
  * @param regAddr The byte address of the register to read
  *
  * @return Value of register requested
  *
  * For selected pages not addressed by iSensor-SPI-Buffer, the read is
  * passed through to the connected IMU, using the spi_passthrough module.
  * If the selected page is [253 - 255] this read request is processed
  * directly.
  */
uint16_t Reg_Read(uint8_t regAddr)
{
	uint16_t regIndex;
	uint16_t status;

	if(selected_page < OUTPUT_PAGE)
	{
		return IMU_Read_Register(regAddr);
	}
	else
	{
		/* Find offset from page */
		regIndex = (selected_page - OUTPUT_PAGE) * REG_PER_PAGE;
		/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
		regIndex += (regAddr >> 1);

		/* Handler buffer retrieve case by setting deferred processing flag */
		if(regIndex == BUF_RETRIEVE_REG)
		{
			/* Set update flag for main loop */
			g_update_flags |= DEQUEUE_BUF_FLAG;

			/* Return 0 */
			return 0;
		}

		if(regIndex > BUF_RETRIEVE_REG)
		{
			if(g_CurrentBufEntry && (regIndex < g_bufLastRegIndex))
			{
				return g_CurrentBufEntry[regIndex - BUF_UTC_TIMESTAMP_REG];
			}
			else
			{
				return 0;
			}
		}

		/* Clear status upon read */
		if(regIndex == STATUS_0_REG || regIndex == STATUS_1_REG)
		{
			status = g_regs[STATUS_0_REG];
			g_regs[STATUS_0_REG] &= STATUS_CLEAR_MASK;
			g_regs[STATUS_1_REG] = g_regs[STATUS_0_REG];
			return status;
		}

		uint32_t microseconds = Timer_Get_Microsecond_Timestamp();

		/* Load time stamp on demand upon read */
		if(regIndex == TIMESTAMP_LWR_REG)
			return microseconds & 0xFFFF;
		if(regIndex == TIMESTAMP_UPR_REG)
			return microseconds >> 16;

		/* get value from reg array */
		return g_regs[regIndex];
	}
}

/**
  * @brief Process a register write request (from master)
  *
  * @param regAddr The address of the register to write to
  *
  * @param regValue The value to write to the register
  *
  * @return The contents of the register being written, after write is processed
  *
  * For selected pages not addressed by iSensor-SPI-Buffer, the write is
  * passed through to the connected IMU, using the spi_passthrough module.
  * If the selected page is [252 - 255] this write request is processed
  * directly. The firmware echoes back the processed write value so that
  * the master can verify the write contents on the next SPI transaction.
  */
uint16_t Reg_Write(uint8_t regAddr, uint8_t regValue)
{
	uint16_t regIndex;

	/* Handle page register writes first */
	if(regAddr == 0)
	{
		/* Are we moving to page 255? Enable capture first time */
		if((regValue == BUF_READ_PAGE) && (selected_page != BUF_READ_PAGE))
		{
			g_update_flags |= ENABLE_CAPTURE_FLAG;
		}
		/* Are we leaving page 255? Then disable capture */
		if((regValue != BUF_READ_PAGE) && (selected_page == BUF_READ_PAGE))
		{
			g_update_flags |= DISABLE_CAPTURE_FLAG;
		}
		/* Save page */
		selected_page = regValue;
	}

	if(selected_page < OUTPUT_PAGE)
	{
		/* Pass to IMU */
		return IMU_Write_Register(regAddr, regValue);
	}
	else
	{
		/* Process reg write then return value from addressed register */
		regIndex = ProcessRegWrite(regAddr, regValue);
		/* get value from reg array */
		return g_regs[regIndex];
	}
}

/**
  * @brief Process a write to the iSensor-SPI-Buffer registers
  *
  * @return The index to the register within the global register array
  *
  * This function handles filtering for read-only registers. It also handles
  * setting the deferred processing flags as needed for any config/command register
  * writes. This are processed on the next pass of the main loop.
  */
static uint16_t ProcessRegWrite(uint8_t regAddr, uint8_t regValue)
{
	/* Index within the register array */
	uint32_t regIndex;

	/* Value to write to the register */
	uint16_t regWriteVal;

	/* Track if write is to the upper word of register */
	uint32_t isUpper = regAddr & 0x1;

	/* Find offset from page */
	regIndex = (selected_page - OUTPUT_PAGE) * REG_PER_PAGE;

	/* The regAddr will be in range 0 - 127 for register index in range 0 - 63*/
	regIndex += (regAddr >> 1);

	/* Handle page reg */
	if(regAddr < 2)
	{
		/* Return reg index (points to page reg instance) */
		return regIndex;
	}

	/* Ignore writes to out of bound or read only registers */
	if(selected_page == BUF_CONFIG_PAGE)
	{
		/* Last writable reg on config page is UTC_TIMESTAMP_UPR_REG */
		if(regIndex > UTC_TIMESTAMP_UPR_REG)
		{
			return regIndex;
		}

		/* Any registers which require filtering or special actions in main loop */
		if(regIndex == IMU_SPI_CONFIG_REG)
		{
			if(isUpper)
			{
				/* Need to set a flag to update IMU spi config */
				g_update_flags |= IMU_SPI_CONFIG_FLAG;
			}
		}
		else if(regIndex == USER_SPI_CONFIG_REG)
		{
			/* No updating needed, we don't provide user SPI */
		}
		else if(regIndex == DIO_OUTPUT_CONFIG_REG)
		{
			if(isUpper)
			{
				/* Need to set a flag to update DIO output config */
				g_update_flags |= DIO_OUTPUT_CONFIG_FLAG;
			}
		}
		else if(regIndex == DIO_INPUT_CONFIG_REG)
		{
			/* No updating needed, we only use the DR_POLARITY register
			 * and both 0 and 1 are valid values */
		}
		else if(regIndex == USER_COMMAND_REG)
		{
			if(isUpper)
			{
				/* Need to set a flag to process command */
				g_update_flags |= USER_COMMAND_FLAG;
			}
		}
		else if((regIndex == UTC_TIMESTAMP_UPR_REG) ||
				(regIndex == UTC_TIMESTAMP_LWR_REG))
		{
			/* Clear microsecond timestamp on write to UTC time */
			Timer_Clear_Microsecond_Timer();
		}
	}
	else if(selected_page == BUF_WRITE_PAGE)
	{
		/* regs under write data are reserved */
		if(regIndex < BUF_WRITE_0_REG)
			return regIndex;

		/* regs over write data are reserved */
		if(regIndex > (BUF_WRITE_0_REG + 31))
			return regIndex;
	}
	else if(selected_page == BUF_READ_PAGE)
	{
		/* Buffer output registers / buffer retrieve are read only */
		if(regIndex == BUF_CNT_1_REG)
		{
			if(regValue == 0)
			{
				/* Clear buffer for writes of 0 to count */
				Buffer_Reset();
				return regIndex;
			}
			else
			{
				/* Ignore non-zero writes */
				return regIndex;
			}
		}
		else
		{
			/* Can't write any other registers on page */
			return regIndex;
		}
	}
	else if(selected_page == OUTPUT_PAGE)
	{
		/* Don't currently have any special actions here, all writes allowed */
	}
	else
	{
		/* Block all other pages */
		return regIndex;
	}

	/* Get initial register value */
	regWriteVal = g_regs[regIndex];

	/* Perform write to reg array */
	if(isUpper)
	{
		/* Write upper reg byte */
		regWriteVal &= 0x00FF;
		regWriteVal |= (regValue << 8);
	}
	else
	{
		/* Write lower reg byte */
		regWriteVal &= 0xFF00;
		regWriteVal |= regValue;
	}
	/* Apply to reg array */
	g_regs[regIndex] = regWriteVal;

	/* Check for buffer reset actions which should be performed in ISR */
	if(regIndex == BUF_CONFIG_REG || regIndex == BUF_LEN_REG)
	{
		if(isUpper)
		{
			/* Reset the buffer after writing upper half of register (applies new settings) */
			Buffer_Reset();
		}
	}

	/* return index for readback after write */
	return regIndex;
}

