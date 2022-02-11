#ifndef INC_SCRIPT_H_
#define INC_SCRIPT_H_

/* Header includes require for prototypes */
#include <stdint.h>

/** Available script commands */
typedef enum
{
	read,
	write,
	delim,
	echo,
	readbuf,
	stream,
	freset,
	cmd,
	status,
	cnt,
	about,
	uptime,
	help,
	sleep,
	loop,
	endloop,
	invalid
}command;

/** Script entry data structure. Used for CLI and SD card logging */
typedef struct
{
	/** The command type associated with the script */
	command scrCommand;

	/** Command arguments */
	uint32_t args[3];

	/** Number of command line arguments */
	uint32_t numArgs;

	/** Track if script entry has valid arguments  */
	uint32_t invalidArgs;
}script;

/* Public function prototypes */
void Script_Check_Stream();
void Script_Parse_Element(const uint8_t* commandBuf, script * scr);
void Script_Run_Element(script* scr, uint8_t * outBuf);

/** Buffer output base address (on page 255) */
#define BUF_BASE_ADDR			8

/** Buffer output data base address (on page 255) */
#define BUF_DATA_BASE_ADDR		16

/** Stream buffer size */
#define STREAM_BUF_SIZE			512

#endif /* INC_SCRIPT_H_ */
