#include <stdint.h>
// #include <portaudio.h>
#include <stdio.h>
#include "mp3decoder/mp3decoder.h"

int main(void)
{
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

	if (loadFile("C:/Users/nicot/Downloads/PizzaConmigo.mp3"))
	{
		getMP3DecodedFrame(buffer, MP3_DECODED_BUFFER_SIZE, &sampleCount);
	}

	return 0;
}