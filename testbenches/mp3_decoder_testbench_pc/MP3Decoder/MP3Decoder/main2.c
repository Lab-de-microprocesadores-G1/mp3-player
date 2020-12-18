#include <stdint.h>
#include <stdio.h>
#include "mp3decoder/mp3decoder.h"
#include "wav.h"

#define BLOCK_SIZE 50000

#define SAMPLE

//#define FLOATBUF
#define INTBUF

#ifdef SAMPLE
#define FILEPATH_SRC	"C:/Users/nicot/Downloads/sample.wav"
#define FILEPATH_WAV	"C:/Users/nicot/Downloads/sampleOut.wav"
#define SAMPLE_RATE		44100
#endif

#ifdef FLOATBUF
#define SAMPLE_FORMAT WAV_FORMAT_IEEE_FLOAT
#endif
#ifdef INTBUF
#define SAMPLE_FORMAT WAV_FORMAT_PCM
#endif

#define NUM_CHANNELS	1

static float readBuffer[NUM_CHANNELS][BLOCK_SIZE];
static short buffer[MP3_DECODED_BUFFER_SIZE];
static float floatBuffer[MP3_DECODED_BUFFER_SIZE];

int caca(void);

int caca(void)
{
	printf("********************************* \n");
	printf(" HELIX MP3 DECODER TESTBENCH (PC) \n");
	printf("********************************* \n");

	uint16_t sampleCount;
	uint32_t sr;
	uint8_t j = 0;
	WavFile* wavIn, *wavOut;


	// Open read and write file
	wavIn = wav_open(FILEPATH_SRC, "rb");
	WavU16 format = wav_get_format(wavIn);
	wavOut = wav_open(FILEPATH_WAV, "wb");
	wav_set_format(wavOut, SAMPLE_FORMAT);

	#ifdef SAMPLE
	uint16_t readBytes;

	// Read and write files
	while (!wav_eof(wavIn))
	{
		static int i = 0;
		i++;
		readBytes = wav_read(wavIn, readBuffer, BLOCK_SIZE);
		if (readBytes == 0)
		{
			i++;
		}
		wav_write(wavOut, readBuffer, readBytes);
	}
	wav_close(wavOut);
	wav_close(wavIn);
	#endif

	return 0;
}

