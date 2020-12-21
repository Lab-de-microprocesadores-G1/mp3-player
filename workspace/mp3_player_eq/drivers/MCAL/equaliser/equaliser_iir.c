/***************************************************************************//**
  @file     equaliser.c
  @brief    ...
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "equaliser_iir.h"
#include "arm_math.h"
#include "math_helper.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
#define IIR_EQ_GAIN_LEVELS  (8)   // Levels of gain
#define IIR_EQ_BANDS        (8)   // Equaliser bands
#define IIR_EQ_STAGES       ()    // Stages per filter
#define IIR_EQ_COEFFS       ()    // Coefficients per stages
#define IIR_EQ_STATE_VARS   ()    // State var
#define IIR_EQ_MAX_GAIN     ()    // Maximum gain

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef struct
{
  q15_t             pState[IIR_EQ_STATE_VARS * IIR_EQ_STAGES];
  q15_t             coefficients[IIR_EQ_COEFFS * IIR_EQ_STAGES];
}eq_iir_filter_t;

typedef struct
{
  float32_t                     sampleFreq;           
  eq_iir_filter_t               filterBands[IIR_EQ_BANDS];      // array that contains a filter-type for each band
  arm_biquad_casd_df1_inst_q15  filter;  
  q15_t                         coefficients[IIR_EQ_BANDS * IIR_EQ_COEFFS * IIR_EQ_STAGES];             
}eq_iir_context_t;

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
eq_iir_filter_t calculateCoefficients(uint32_t freq, uint32_t gain, float32_t qFactor);

/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static const float32_t  equaliserCoeff[IIR_EQ_GAIN_LEVELS][IIR_EQ_BANDS][IIR_EQ_STAGES*IIR_EQ_COEFFS] = 
{
  {/* Llenarlo con números turbios*/}
};

static const float32_t eqBands[IIR_EQ_BANDS];
static const float32_t eqQFactor[IIR_EQ_BANS]; 

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/
static eq_iir_context_t context;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
void eqIirInit(float32_t sampleFreq)
{
  context.sampleFreq = sampleFreq;

}

void eqIirFilterFrame(float32_t * inputF32, float32_t * outputF32)
{
  arm_biquad_cascade_df1_q15(&(context.filter), (q15_t*) inputF32, (q15_t*) outputF32);

}

void eqIirSetFilterGains(float32_t gains[EQ_NUM_OF_FILTERS])
{
  for (uint8_t i = 0 ; i < IIR_EQ_BANDS ; i++)
  {
    context.filterBands[i].coefficients = calculateCoefficients(eqBands[i], qFactor[i] gains[i]);

    for (uint8_t j = 0 ; j < (IIR_EQ_COEFFS * IIR_EQ_STAGES) ; j++)
    {
      context.coefficients[j + i * IIR_EQ_COEFFS * IIR_EQ_STAGES];
    }

    arm_biquad_cascade_df1_init_q15(&(context.filter), IIR_EQ_STAGES * IIR_EQ_BANDS, context.coefficients, context.filterBands[0].pState, 1);
  }
}
/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
static eq_iir_filter_t calculateCoefficients(float32_t freq, uint32_t gain, float32_t qFactor)
{
  eq_iir_filter_t filter;
  uin32_t g = gain + IIR_EQ_MAX_GAIN;
  uint8_t band = 0;

  for (uint8_t i = 0 ; i < IIR_EQ_BANDS ; i++)
  {
    if (freq == eqBands[i])
    {
      band = i;
      break;
    }
  }

	arm_float_to_q15(equaliserCoeff[g][band],context.filter.pCoeffs, IIR_EQ_COEFFS * IIR_EQ_STAGES);

	return filter;
}
/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
