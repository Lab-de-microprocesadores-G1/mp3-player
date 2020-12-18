#include <stdint.h>
#include <stdio.h>
#include "tinywav.h"
#include "mp3decoder/mp3decoder.h"

// #define FILEPATH					"D:/Users/Joaco/Desktop/playground/HelixMP3Test/testfiles/Haydn_Cello_Concerto_D-1.mp3"

#define FILEPATH		"C:/Users/nicot/Downloads/Haydn_Cello_Concerto_D-1.mp3"
#define FILEPATH_WAV	"C:/Users/nicot/Downloads/Haydn_Cello_Concerto_D-1.wav"

#define NUM_CHANNELS 1
#define SAMPLE_RATE 11025

int main(void)
{
	printf("********************************* \n");
	printf(" HELIX MP3 DECODER TESTBENCH (PC) \n");
	printf("********************************* \n");

	int16_t buffer[MP3_DECODED_BUFFER_SIZE];
	float floatBuffer[MP3_DECODED_BUFFER_SIZE];
	uint16_t sampleCount;
	uint32_t sr;

	TinyWav tw;
	tinywav_open_write(&tw,
		NUM_CHANNELS,
		SAMPLE_RATE,
		TW_FLOAT32, // the output samples will be 32-bit floats. TW_INT16 is also supported
		TW_INLINE,  // the samples will be presented inlined in a single buffer.
					// Other options include TW_INTERLEAVED and TW_SPLIT
		FILEPATH_WAV // the output path
	);


	mp3DecoderInit();

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
				sr = getFrameSampleRate();
				printf("[APP] FRAME SAMPLE RATE: %d ", sr);

				// Int to float
				for (uint32_t i = 0; i < sampleCount; i++)
				{
					floatBuffer[i] = (float) buffer[i];
				}

				// Write to WAV file
				tinywav_write_f(&tw, buffer, sampleCount);
			}
			else if (res == MP3DECODER_FILE_END)
			{
				printf("[APP] FILE ENDED. Decoded %d frames.\n",i-1);
				break;
			}
			else
			{
				int huevo = 0;
				huevo++;
			}
		}
		tinywav_close_write(&tw);
	}
	else
	{
		int huevo = 0;
		huevo++;
	}

	return 0;
}