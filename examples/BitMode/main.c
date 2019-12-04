/*
 * Assuming libftd2xx.so is in /usr/local/lib, build with:
 * 
 *     gcc -o bitmode main.c -L. -lftd2xx -Wl,-rpath /usr/local/lib
 * 
 * and run with:
 * 
 *     sudo ./bitmode [port number]
 */
#include <stdio.h>
#include "../ftd2xx.h"
#include <unistd.h>

static const int DATALEN = 1000000;

int main(int argc, char *argv[])
{
	DWORD       bytesWritten = 0;
	DWORD       baudRate = 96000; //Origin:9600
	FT_STATUS	ftStatus = FT_OK;
	FT_HANDLE	ftHandle;
	UCHAR       outputData;
	UCHAR       pinStatus;
	int         portNumber;
	int 		data[DATALEN];
	
	if (argc > 1) 
	{
		sscanf(argv[1], "%d", &portNumber);
	}
	else 
	{
		portNumber = 0;
	}
	
	ftStatus = FT_Open(portNumber, &ftHandle);
	if (ftStatus != FT_OK) 
	{
		/* FT_Open can fail if the ftdi_sio module is already loaded. */
		printf("FT_Open(%d) failed (error %d).\n", portNumber, (int)ftStatus);
		printf("Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
		printf("If so, unload them using rmmod, as they conflict with ftd2xx.\n");
		return 1;
	}

	/* Enable bit-bang mode, where 8 UART pins (RX, TX, RTS etc.) become
	 * general-purpose I/O pins.
	 */
	printf("Selecting asynchronous bit-bang mode.\n");	
	ftStatus = FT_SetBitMode(ftHandle, 
	                         0xFF, /* sets all 8 pins as outputs */
	                         FT_BITMODE_ASYNC_BITBANG); //origin FT_BITMODE_ASYNC_BITBANG
	if (ftStatus != FT_OK) 
	{
		printf("FT_SetBitMode failed (error %d).\n", (int)ftStatus);
		goto exit;
	}

	/* In bit-bang mode, setting the baud rate gives a clock rate
	 * 16 times higher, e.g. baud = 9600 gives 153600 bytes per second.
	 */
	printf("Setting clock rate to %d\n", baudRate * 16);
	ftStatus = FT_SetBaudRate(ftHandle, baudRate);
	if (ftStatus != FT_OK) 
	{
		printf("FT_SetBaudRate failed (error %d).\n", (int)ftStatus);
		goto exit;
	}
	
	/* Use FT_Write to set values of output pins.  Here we set
	 * them to alternate low and high (0xAA == 10101010).
	 */
	outputData = 0xAA;
	ftStatus = FT_Write(ftHandle, &outputData, 1, &bytesWritten);
	if (ftStatus != FT_OK)
	{
		printf("FT_Write failed (error %d).\n", (int)ftStatus);
		goto exit;
	}

	/* Despite its name, GetBitMode samples the values of the data pins. */
	ftStatus = FT_GetBitMode(ftHandle, &pinStatus);
	if (ftStatus != FT_OK) 
	{
		printf("FT_GetBitMode failed (error %d).\n", (int)ftStatus);
		goto exit;
	}

	if (pinStatus != outputData) 
	{
		printf("Failure: pin data is %02X, but expected %02X\n", 
		       (unsigned int)pinStatus,
		       (unsigned int)outputData);
		goto exit;
	}

	printf("Success: pin data is %02X, as expected.\n", 
		   (unsigned int)pinStatus);

	/* Initial the test data */
	for(int i=0;i<DATALEN;i++)
	{
		if(i % 2)
			data[i] = 0x00;
		else
			data[i] = 0xFF;
	}

	/* Toggle the IO with continuous data */
	ftStatus = FT_Write(ftHandle, data, DATALEN, &bytesWritten);
	if(ftStatus != FT_OK)
	{
		printf("Data buffer transmits error!\n");
	}
	else
	{
		printf("Bytes transferred: %d!\n",bytesWritten);
	}
	
	// /* Toggle the IO without delay */
	// printf("Start output toggle test.\n");
	// for(int i;i<10000;i++)
	// {
	// 	outputData = 0x00;
	// 	ftStatus = FT_Write(ftHandle, &outputData, 1, &bytesWritten);
	// 	usleep(1000);
	// 	outputData = 0xFF;
	// 	ftStatus = FT_Write(ftHandle, &outputData, 1, &bytesWritten);
	// 	usleep(1000);
	// }

exit:
// 	/* Return chip to default (UART) mode. */
// 	(void)FT_SetBitMode(ftHandle, 
// 	                    0, /* ignored with FT_BITMODE_RESET */
// 	                    FT_BITMODE_RESET);

	(void)FT_Close(ftHandle);
	return 0;
}
