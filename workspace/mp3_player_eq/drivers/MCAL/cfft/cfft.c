/***************************************************************************//**
  @file     equaliser.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "cfft.h"
#include "arm_const_structs.h"
#include <stdio.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/

static arm_cfft_instance_f32 * cfftSizeToInstance(cfft_size_t size);
static uint32_t cfftInstanceToSize(arm_cfft_instance_f32 * instance);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

arm_cfft_instance_f32 * cfftInstance;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void cfftInit(cfft_size_t size)
{
  cfftInstance = cfftSizeToInstance(size);
}

void cfft(float32_t * inputF32, float32_t * outputF32, bool doBitReverse)
{
  memcpy(outputF32, inputF32, cfftInstanceToSize(cfftInstance) * sizeof(float32_t));    // Copying input array to preserve it.
  arm_cfft_f32(cfftInstance, outputF32, false, doBitReverse);
}

void icfft(float32_t * inputF32, float32_t * outputF32, bool doBitReverse)
{
  memcpy(outputF32, inputF32, cfftInstanceToSize(cfftInstance) * 2 * sizeof(float32_t));    // Copying input array to preserve it.
  arm_cfft_f32(cfftInstance, outputF32, true, doBitReverse);
}

void cfftGetMag(float32_t * inputF32, float32_t * outputF32)
{
  arm_cmplx_mag_f32(inputF32, outputF32, cfftInstanceToSize(cfftInstance));
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
arm_cfft_instance_f32 * cfftSizeToInstance(cfft_size_t size)
{
  arm_cfft_instance_f32 * instance = &arm_cfft_sR_f32_len1024;

  switch (size)
  {
  case CFFT_16: instance = &arm_cfft_sR_f32_len16; break;
  case CFFT_32: instance = &arm_cfft_sR_f32_len32; break;
  case CFFT_64: instance = &arm_cfft_sR_f32_len64; break;
  case CFFT_128: instance = &arm_cfft_sR_f32_len128; break;
  case CFFT_256: instance = &arm_cfft_sR_f32_len256; break;
  case CFFT_512: instance = &arm_cfft_sR_f32_len512; break;
  case CFFT_1024: instance = &arm_cfft_sR_f32_len1024; break;
  case CFFT_2048: instance = &arm_cfft_sR_f32_len2048; break;
  case CFFT_4096: instance = &arm_cfft_sR_f32_len4096; break;
  }

  return instance;
}

uint32_t cfftInstanceToSize(arm_cfft_instance_f32 * instance)
{
  uint32_t size = 1024;

  if (instance == &arm_cfft_sR_f32_len16) size = 16;
  else if (instance == &arm_cfft_sR_f32_len32) size = 32;
  else if (instance == &arm_cfft_sR_f32_len64) size = 64;
  else if (instance == &arm_cfft_sR_f32_len128) size = 128;
  else if (instance == &arm_cfft_sR_f32_len256) size = 256;
  else if (instance == &arm_cfft_sR_f32_len512) size = 512;
  else if (instance == &arm_cfft_sR_f32_len1024) size = 1024;
  else if (instance == &arm_cfft_sR_f32_len2048) size = 2048;
  else if (instance == &arm_cfft_sR_f32_len4096) size = 4096;

  return size;
}

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
