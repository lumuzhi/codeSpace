#ifndef _LH_MATH_H
#define _LH_MATH_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PI 3.141592653589793

struct CPLX
{
	float real, imag;
};

enum COMPLEX_SUB_TYPE
{
	COMPLEX_REAL,
	COMPLEX_IMAG
};

enum FFT_CONVERSION_TYPE
{
	FFT_E,
	IFFT_E
};

float float_mean(float *buff, uint32_t size);
float float_mean2(uint16_t *buff, uint32_t size);
void float_sub_mean(float *buff, uint32_t size);
float float_sum(float x[], uint32_t size);
double float_sum_xy(double *x, double *y, uint32_t size);
float float_multiply_sumx_sumy(float x[], float y[], uint32_t size);
float float_sum_x2(float x[], uint32_t size);
float float_sumx_2(float x[], uint32_t size);
float float_sum(float x[], uint32_t size);
float self_test_date_deal(float *value_buff, uint32_t size);
float float_get_max(float *buff, uint32_t size);
float float_get_max_fabs(float *buff, uint32_t size);
float float_get_min(float *buff, uint32_t size);
float float_std_rms(float *src, uint32_t size);
void float_fft(float *buff, int size,int FFT_POINT);
void check_ad_value(float *buff,uint32_t size);

void complex_fft(float *buff, float *real, float *imag, int size, int FFT_POINT, uint8_t is_ifft);
void complex_fft_convert(float *fft_data, float *p_real, float *p_imag, int FFT_POINT);

uint16_t uint16_mean(uint16_t *buff,uint32_t size);
double double_sum_xy(double x[], double y[], uint32_t size);
void supplement_zero(float *buff,uint32_t cnt);
void fft_diag_deal(float *fft_data, float *fft_real, float *fft_imag, float fft_size, uint8_t is_ifft);

float self_round(float x, uint8_t n);
#endif
