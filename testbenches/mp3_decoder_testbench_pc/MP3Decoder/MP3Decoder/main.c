#include <stdint.h>
// #include <portaudio.h>
#include <stdio.h>
#include "mp3decoder/mp3decoder.h"

#define FILEPATH					"D:/Users/Joaco/Desktop/playground/HelixMP3Test/testfiles/Haydn_Cello_Concerto_D-1.mp3"

int main(void)
{
	printf("********************************* \n");
	printf(" HELIX MP3 DECODER TESTBENCH (PC) \n");
	printf("********************************* \n");

	uint16_t buffer[MP3_DECODED_BUFFER_SIZE];
	uint16_t sampleCount;

	// Pa_Initialize();
	mp3DecoderInit();

	//PaSampleFormat sampleFormat = paInt16;
	//unsigned long sampleRate = 44100;
	//unsigned long framesPerBuffer;

	// Device settings
	//PaTime defaultLowInputLatency;
	//PaTime defaultHighInputLatency;
	//PaTime defaultLowOutputLatency;
	//PaTime defaultHighOutputLatency;
	//PaHostApiIndex hostApi;

	//PaStreamParameters parameters;
	//PaStream* stream;

	//PaDeviceIndex deviceIndex = Pa_GetDefaultOutputDevice();
	//const PaDeviceInfo* info = Pa_GetDeviceInfo(deviceIndex);
	//printf("Default output device %d\n", deviceIndex);
	//printf("Device name: %s", info->name);

	if (loadFile(FILEPATH))
	{
		int i = 0;
		while(1)
		{
		printf("\n[APP] Frame %d decoding started.\n", i);
		mp3decoder_result_t res = getMP3DecodedFrame(buffer, MP3_DECODED_BUFFER_SIZE, &sampleCount);
		if (res == 0)
		{
			printf("[APP] Frame %d decoded.\n", i);
			i++;
		}
		else if (res == MP3DECODER_FILE_END)
		{
			printf("[APP] FILE ENDED. Decoded %d frames.\n",i-1);
			break;
		}
		}
		
	}

	return 0;
}