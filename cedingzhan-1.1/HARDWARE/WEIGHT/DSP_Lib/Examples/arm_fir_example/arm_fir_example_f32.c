/* ----------------------------------------------------------------------
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 *
* $Date:         17. January 2013
* $Revision:     V1.4.0
*
* Project:       CMSIS DSP Library
 * Title:        arm_fir_example_f32.c
 *
 * Description:  Example code demonstrating how an FIR filter can be used
 *               as a low pass filter.
 *
 * Target Processor: Cortex-M4/Cortex-M3
 *
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of ARM LIMITED nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
 * -------------------------------------------------------------------- */

/**
 * @ingroup groupExamples
 */

/**
 * @defgroup FIRLPF FIR Lowpass Filter Example
 *
 * \par Description:
 * \par
 * Removes high frequency signal components from the input using an FIR lowpass filter.
 * The example demonstrates how to configure an FIR filter and then pass data through
 * it in a block-by-block fashion.
 * \image html FIRLPF_signalflow.gif
 *
 * \par Algorithm:
 * \par
 * The input signal is a sum of two sine waves:  1 kHz and 15 kHz.
 * This is processed by an FIR lowpass filter with cutoff frequency 6 kHz.
 * The lowpass filter eliminates the 15 kHz signal leaving only the 1 kHz sine wave at the output.
 * \par
 * The lowpass filter was designed using MATLAB with a sample rate of 48 kHz and
 * a length of 29 points.
 * The MATLAB code to generate the filter coefficients is shown below:
 * <pre>
 *     h = fir1(28, 6/24);
 * </pre>
 * The first argument is the "order" of the filter and is always one less than the desired length.
 * The second argument is the normalized cutoff frequency.  This is in the range 0 (DC) to 1.0 (Nyquist).
 * A 6 kHz cutoff with a Nyquist frequency of 24 kHz lies at a normalized frequency of 6/24 = 0.25.
 * The CMSIS FIR filter function requires the coefficients to be in time reversed order.
 * <pre>
 *     fliplr(h)
 * </pre>
 * The resulting filter coefficients and are shown below.
 * Note that the filter is symmetric (a property of linear phase FIR filters)
 * and the point of symmetry is sample 14.  Thus the filter will have a delay of
 * 14 samples for all frequencies.
 * \par
 * \image html FIRLPF_coeffs.gif
 * \par
 * The frequency response of the filter is shown next.
 * The passband gain of the filter is 1.0 and it reaches 0.5 at the cutoff frequency 6 kHz.
 * \par
 * \image html FIRLPF_response.gif
 * \par
 * The input signal is shown below.
 * The left hand side shows the signal in the time domain while the right hand side is a frequency domain representation.
 * The two sine wave components can be clearly seen.
 * \par
 * \image html FIRLPF_input.gif
 * \par
 * The output of the filter is shown below.  The 15 kHz component has been eliminated.
 * \par
 * \image html FIRLPF_output.gif
 *
 * \par Variables Description:
 * \par
 * \li \c testInput_f32_1kHz_15kHz points to the input data
 * \li \c refOutput points to the reference output data
 * \li \c testOutput points to the test output data
 * \li \c firStateF32 points to state buffer
 * \li \c firCoeffs32 points to coefficient buffer
 * \li \c blockSize number of samples processed at a time
 * \li \c numBlocks number of frames
 *
 * \par CMSIS DSP Software Library Functions Used:
 * \par
 * - arm_fir_init_f32()
 * - arm_fir_f32()
 *
 * <b> Refer  </b>
 * \link arm_fir_example_f32.c \endlink
 *
 */


/** \example arm_fir_example_f32.c
 */

/* ----------------------------------------------------------------------
** Include Files
** ------------------------------------------------------------------- */

#include "arm_math.h"
#include "math_helper.h"

/* ----------------------------------------------------------------------
** Macro Defines
** ------------------------------------------------------------------- */

#define TEST_LENGTH_SAMPLES  1120  //样本数
#define BLOCK_SIZE            160  //块大小
#define NUM_TAPS              160  //滤波器阶数

/* -------------------------------------------------------------------
 * The input signal and reference output (computed with MATLAB)
 * are defined externally in arm_fir_lpf_data.c.
 * ------------------------------------------------------------------- */

 float32_t Input_date[TEST_LENGTH_SAMPLES] = {0.0f};//输入输出数据
 float32_t Output_date[TEST_LENGTH_SAMPLES] = {0.0f};

/* -------------------------------------------------------------------
 * Declare Test output buffer
 * ------------------------------------------------------------------- */



/* -------------------------------------------------------------------
 * Declare State buffer of size (numTaps + blockSize - 1)
 * ------------------------------------------------------------------- */

static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];

/* ----------------------------------------------------------------------
** FIR Coefficients buffer generated using fir1() MATLAB function.
** ------------------------------------------------------------------- */
//滤波器系数
const float32_t  firCoeffs32[NUM_TAPS] = {
  0.0001862996924395, 0.000230659600748,0.0002794523772752,0.0003328517944823,
  0.0003910255474156,0.0004541344583158,0.0005223316852003,0.0005957619376335,
  0.0006745607029267,0.0007588534860218,0.0008487550663142,0.0009443687746516,
   0.001045785793722, 0.001153084484998, 0.001266329745347, 0.001385572396357,
   0.001510848609316, 0.001642179368727, 0.001779569977088, 0.001923009603568,
   0.002072470879071, 0.002227909540008,  0.00238926412298, 0.002556455712356,
   0.002729387742588, 0.002907945856882, 0.003091997823677, 0.003281393512135,
   0.003475964927675, 0.003675526308321, 0.003879874282431, 0.004088788088142,
   0.004302029854607,  0.00451934494489, 0.004740462360127, 0.004965095204322,
   0.005192941208917,  0.00542368331601, 0.005656990318894, 0.005892517558324,
   0.006129907672704, 0.006368791400162, 0.006608788430258, 0.006849508302864,
   0.007090551351555, 0.007331509688638, 0.007571968228804, 0.007811505748156,
   0.008049695975272, 0.008286108710757, 0.008520310971638, 0.008751868156821,
   0.008980345229721, 0.009205307914091, 0.009426323898996, 0.009642964048807,
   0.009854803614062,    0.010061423439,  0.01026241116156,  0.01045736240165,
    0.01064588193354,  0.01082758483811,  0.01100209763109,  0.01116905936302,
    0.01132812268723,  0.01147895489177,  0.01162123889187,  0.01175467417904,
    0.01187897772363,  0.01199388482742,  0.01209914992319,  0.01219454731846,
    0.01227987188043,  0.01235493965996,  0.01241958845198,  0.01247367829056,
    0.01251709187656,  0.01254973493652,  0.01257153651137,  0.01258244917383,
    0.01258244917383,  0.01257153651137,  0.01254973493652,  0.01251709187656,
    0.01247367829056,  0.01241958845198,  0.01235493965996,  0.01227987188043,
    0.01219454731846,  0.01209914992319,  0.01199388482742,  0.01187897772363,
    0.01175467417904,  0.01162123889187,  0.01147895489177,  0.01132812268723,
    0.01116905936302,  0.01100209763109,  0.01082758483811,  0.01064588193354,
    0.01045736240165,  0.01026241116156,    0.010061423439, 0.009854803614062,
   0.009642964048807, 0.009426323898996, 0.009205307914091, 0.008980345229721,
   0.008751868156821, 0.008520310971638, 0.008286108710757, 0.008049695975272,
   0.007811505748156, 0.007571968228804, 0.007331509688638, 0.007090551351555,
   0.006849508302864, 0.006608788430258, 0.006368791400162, 0.006129907672704,
   0.005892517558324, 0.005656990318894,  0.00542368331601, 0.005192941208917,
   0.004965095204322, 0.004740462360127,  0.00451934494489, 0.004302029854607,
   0.004088788088142, 0.003879874282431, 0.003675526308321, 0.003475964927675,
   0.003281393512135, 0.003091997823677, 0.002907945856882, 0.002729387742588,
   0.002556455712356,  0.00238926412298, 0.002227909540008, 0.002072470879071,
   0.001923009603568, 0.001779569977088, 0.001642179368727, 0.001510848609316,
   0.001385572396357, 0.001266329745347, 0.001153084484998, 0.001045785793722,
  0.0009443687746516,0.0008487550663142,0.0007588534860218,0.0006745607029267,
  0.0005957619376335,0.0005223316852003,0.0004541344583158,0.0003910255474156,
  0.0003328517944823,0.0002794523772752, 0.000230659600748,0.0001862996924395
};

/* ------------------------------------------------------------------
 * Global variables for FIR LPF Example
 * ------------------------------------------------------------------- */

uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = TEST_LENGTH_SAMPLES/BLOCK_SIZE;


/* ----------------------------------------------------------------------
 * FIR LPF Example
 * ------------------------------------------------------------------- */

int32_t fir_test(void)
{
  uint32_t i;
  arm_fir_instance_f32 S;
  float32_t  *inputF32, *outputF32;

  /* Initialize input and output buffer pointers */
  inputF32 = &Input_date[0];
  outputF32 = &Output_date[0];

  /* Call FIR init function to initialize the instance structure. */
  arm_fir_init_f32(&S, NUM_TAPS, (float32_t *)&firCoeffs32[0], &firStateF32[0], blockSize);

  /* ----------------------------------------------------------------------
  ** Call the FIR process function for every blockSize samples
  ** ------------------------------------------------------------------- */
	
  for(i=0; i < numBlocks; i++)
  {
    arm_fir_f32(&S, inputF32 + (i * blockSize), outputF32 + (i * blockSize), blockSize);
  }
 return 0;
}
