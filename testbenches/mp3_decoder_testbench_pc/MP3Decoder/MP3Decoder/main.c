#include <stdint.h>
#include <stdio.h>
#include "wav.h"
#include "mp3decoder/mp3decoder.h"

// #define FILEPATH					"D:/Users/Joaco/Desktop/playground/HelixMP3Test/testfiles/Haydn_Cello_Concerto_D-1.mp3"

#define BLOCK_SIZE 50000

//#define HAYDN
//#define PIZZA_CONMIGO
#define SULLIVAN
//#define SAMPLE

#define INTBUF

#ifdef FLOATBUF
#define SAMPLE_FORMAT WAV_FORMAT_IEEE_FLOAT
#endif
#ifdef INTBUF
#define SAMPLE_FORMAT WAV_FORMAT_PCM
#endif
#ifdef HAYDN
#define FILEPATH		"C:/Users/nicot/Downloads/Haydn_Cello_Concerto_D-1.mp3"
#define FILEPATH_WAV	"C:/Users/nicot/Downloads/Haydn_Cello_Concerto_D-1.wav"
#define SAMPLE_RATE		11025
#endif
#ifdef PIZZA_CONMIGO
#define FILEPATH		"C:/Users/nicot/Downloads/PizzaConmigo_notag.mp3"
#define FILEPATH_WAV	"C:/Users/nicot/Downloads/PizzaConmigo.wav"
#define SAMPLE_RATE		48000
#endif
#ifdef SULLIVAN
#define FILEPATH		"C:/Users/nicot/Downloads/sullivan.mp3"
#define FILEPATH_WAV	"C:/Users/nicot/Downloads/sullivan.wav"
#define SAMPLE_RATE		44100
#endif
#ifdef SAMPLE
#define FILEPATH_SRC	"C:/Users/nicot/Downloads/sample.wav"
#define FILEPATH_WAV	"C:/Users/nicot/Downloads/sampleOut.wav"
#define SAMPLE_RATE		44100
#endif

#define NUM_CHANNELS	1

static short buffer[MP3_DECODED_BUFFER_SIZE];

int main(void)
{
	printf("********************************* \n");
	printf(" HELIX MP3 DECODER TESTBENCH (PC) \n");
	printf("********************************* \n");

	uint16_t sampleCount;
	uint32_t sr;
	uint8_t j = 0;
	WavFile *wavIn, *wavOut, *wav;


	#ifndef SAMPLE
	wav = wav_open(FILEPATH_WAV, "wb");
	wav_set_format(wav, SAMPLE_FORMAT);
	wav_set_sample_rate(wav, SAMPLE_RATE);
	#endif // !SAMPLE

	#ifdef SAMPLE
	// Open read and write file
	wavIn = wav_open(FILEPATH_SRC, "rb");
	WavU16 format = wav_get_format(wavIn);
	wavOut = wav_open(FILEPATH_WAV, "wb");
	wav_set_format(wavOut, SAMPLE_FORMAT);
	#endif

	#ifndef SAMPLE
	MP3DecoderInit();

	if (MP3LoadFile(FILEPATH))
	{
		int i = 0;
		while(1)
		{
			printf("\n[APP] Frame %d decoding started.\n", i);
			mp3decoder_result_t res = MP3GetDecodedFrame(buffer, MP3_DECODED_BUFFER_SIZE, &sampleCount);
			if (res == 0)
			{
				printf("[APP] Frame %d decoded.\n", i);
				i++;
				sr = MP3GetFrameSampleRate();
				printf("[APP] FRAME SAMPLE RATE: %d \n", sr);
				wav_write(wav, buffer, sampleCount);
			}
			else if (res == MP3DECODER_FILE_END)
			{
				printf("[APP] FILE ENDED. Decoded %d frames.\n",i-1);
				wav_close(wav);
				break;
			}
			else
			{
				int huevo = 0;
				huevo++;
			}
		}
	}
	else
	{
		int huevo = 0;
		huevo++;
		printf("¿DANIEL == RANIEL?\n");
	}
	#endif

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

