#include <stdint.h>
#include <stdio.h>
#include "tinywav.h"
#include "mp3decoder/mp3decoder.h"

// #define FILEPATH					"D:/Users/Joaco/Desktop/playground/HelixMP3Test/testfiles/Haydn_Cello_Concerto_D-1.mp3"

#define BLOCK_SIZE 480

//#define HAYDN
#define PIZZA_CONMIGO
//#define SULLIVAN
//#define SAMPLE

#define FLOATBUF
//#define INTBUF

#ifdef FLOATBUF
#define SAMPLE_FORMAT TW_FLOAT32
#endif
#ifdef INTBUF
#define SAMPLE_FORMAT TW_INT16
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

int main(void)
{
	printf("********************************* \n");
	printf(" HELIX MP3 DECODER TESTBENCH (PC) \n");
	printf("********************************* \n");

	short buffer[MP3_DECODED_BUFFER_SIZE];
	float floatBuffer[MP3_DECODED_BUFFER_SIZE];
	uint16_t sampleCount;
	uint32_t sr;
	uint8_t j = 0;

	TinyWav twin, twout, tw;

	#ifndef SAMPLE
		tinywav_open_write(&tw,
			NUM_CHANNELS,
			SAMPLE_RATE,
			SAMPLE_FORMAT,	// the output samples will be 32-bit floats. TW_INT16 is also supported
			TW_INLINE,		// the samples will be presented inlined in a single buffer.
							// Other options include TW_INTERLEAVED and TW_SPLIT
			FILEPATH_WAV	// the output path
		);
	#endif // !SAMPLE

	#ifdef SAMPLE
	tinywav_open_read(&twin,
		FILEPATH_SRC,
		TW_INLINE,
		SAMPLE_FORMAT
	);

	tinywav_open_write(&twin,
		NUM_CHANNELS,
		SAMPLE_RATE,
		SAMPLE_FORMAT,	// the output samples will be 32-bit floats. TW_INT16 is also supported
		TW_INLINE,		// the samples will be presented inlined in a single buffer.
						// Other options include TW_INTERLEAVED and TW_SPLIT
		FILEPATH_WAV	// the output path
	);
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

				// Int to float
				#ifdef FLOATBUF
				for (uint32_t i = 0; i < sampleCount; i++)
				{
					floatBuffer[i] = (float) buffer[i] / 10000000.0;
				}
				#endif 
				#ifdef INTBUF
				// Scale buffer
				for (uint32_t i = 0; i < sampleCount; i++)
				{
					buffer[i] /= 11000.0;
				}
				#endif 

				if (++j % 2 == 0)
				{
					// Write to WAV file
					#ifdef FLOATBUF
					tinywav_write_f(&tw, floatBuffer, sampleCount);
					#endif
					#ifdef INTBUF
					tinywav_write_f(&tw, buffer, sampleCount);
					#endif

					j = 0;
				}
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
		printf("¿DANIEL == RANIEL?\n");
	}
	#endif

	#ifdef SAMPLE
	while()
	tinywav_write_f(&tw, , );
		tinywav_read_f(&tw, , );
	}
	#endif

	return 0;
}

