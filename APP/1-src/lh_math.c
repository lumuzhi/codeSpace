#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "string.h"
#include "lh_math.h"
#include "fftw3.h"
#include "user_data.h"


extern struct POWER_CNT poer_cnt;
/**
 * 均值函数
*/
float float_mean(float *buff,uint32_t size)
{
	int i = 0;
	float sum=buff[0];
	if(buff==NULL)
    	return 0;
    for(i=1;i<size;i++)
	{
		sum+=buff[i];
	}
   // printf("float_sum:%f\n",sum);
	return sum/size;
}

uint16_t uint16_mean(uint16_t *buff,uint32_t size)
{
	int i;
	uint32_t sum=buff[0];
	if(buff==NULL)
    	return 0;
    for(i=1;i<size;i++)
	{
//    	printf("mean_buff:%d\n",buff[i]);
		sum+=buff[i];
	}
 //   printf("mean_sum:%d\n",sum);
	return sum/size;
}


void check_ad_value(float *buff,uint32_t size)
{
	int i = 0;
	int ad_cnt = 0;
//	if(buff == NULL)
//		return 0;

	printf("check_ad size:%d, buff = %f\n",size,buff[10]);
	for(i=0;i<size;i++)
	{
		if(buff[i] == 32768.00f)
		{
			ad_cnt ++;
		}
	}

	if(ad_cnt >= 300)
	{

		if(poer_cnt.power_on_cnt < 2)
		{
			poer_cnt.power_on_cnt = poer_cnt.power_on_cnt + 1;
			poer_cnt.power_on_type = 1;

			set_power_on_para();

			printf("reset poer_cnt.power_on_cnt:%d\n",poer_cnt.power_on_cnt);
			printf("reset poer_cnt.power_on_type:%d\n",poer_cnt.power_on_type);

			system("reboot -nf");
		}

	}
}

/**
 * 去均值操作
 */
void float_sub_mean(float *buff,uint32_t size)
{
	int i = 0;
	//printf("float_sub_mean start\n");
	float s=float_mean(buff,size);
	//printf("float_sub_mean end\n");
	for(i=0;i<size;i++)
	{
		buff[i]=buff[i]-s;
		//buff[i]=(buff[i]-s)*2;
	}
//	printf("mean:%f\n",s);
}


/**
 * 补0操作
 * */
void supplement_zero(float *buff,uint32_t cnt)
{
	uint32_t cnt_i = 0;
	for(cnt_i = 0;cnt_i < cnt;cnt_i++)
	{
		*(buff + cnt_i) = 0;
	}

}


/**
 * 计算均方根
 */
float float_std_rms(float *src, uint32_t size)
{
    return sqrtf(float_sum_x2(src, size) / size);
}

/**
 * 
 * 计算两个数组的乘积的和
 */
double float_sum_xy(double *x, double *y, uint32_t size)
{
    int  i;
    double s = 0.0f;

    for (  i = 0; i < size; i++)
    {

        s += x[i] * y[i];

    }
    return s;
}



/**
 *
 * 计算两个数组的乘积的和
 ***/
double double_sum_xy(double x[], double y[], uint32_t size)
{
    uint32_t  i = 0;
    double s = 0.0f;

    for (i = 0; i < size; i++)
    {
        s += x[i] * y[i];
    }

    return s;
}

/**
 * 计算两个数组的和乘积
 */
float float_multiply_sumx_sumy(float x[], float y[], uint32_t size)
{
    float sumx = 0.0f, sumy = 0.0f;
    sumx = float_sum(x, size);
    sumy = float_sum(y, size);
    return sumx * sumy;
}

/**
 * 计算一个数组的平方和
 * 
 */
float float_sum_x2(float x[], uint32_t size)
{
    int i;
    float s = 0.0f;

    for (i = 0; i < size; i++)
    {
        s += x[i] * x[i];
    }

    return s;
}

/**
 * 计算一个数组的和的平方
 */
float float_sumx_2(float x[], uint32_t size)
{
    float s = 0.0f;
    s = float_sum(x, size);
    return s * s;
}

/**
 * 计算一个数组的和
 */
float float_sum(float *x, uint32_t size)
{
   int i = 0;
   float sum = 0.0f;

   for (  i = 0; i < size; i++)
   {
//    	if(x[i] > 41800)
       sum += x[i];
   }

   return sum;
}

/**
 *　从小到大排序
 * 去掉100个最大值,去掉100个最小值
 * 自检数据处理
 *
 */
float self_test_date_deal(float *value_buff, uint32_t size)
{
	int i = 0,j = 0;
	int min_index = 0;
	float sum = 0.0f;
	float temp = 0;

	//将数据从小到大排序，得到排序后的顺序值
	 for (i = 0; i < size - 1; i++)
	 {
		 min_index = i;
	    for (j = i + 1; j < size; j++)
	    {
	      if (value_buff[j] < value_buff[min_index]) 		//寻找最小的数
	      {
	    	  min_index = j;                 // 将最小数的索引保存
	       }
	    }
	        temp = value_buff[i];
	        value_buff[i] = value_buff[min_index];
	        value_buff[min_index] = temp;
	  }

//	 printf("date_size:%d\n",size);

//	 printf("data:\n");
//	 for(i = 0;i < size;i++)
//	 {
//		 printf("%f\n",value_buff[i]);
//	 }
//	 printf("\n");


	 //将排序后的值,去掉100个最大,去掉100个最小,再求和
	for ( i= 0; i < (size-200); i++)
	{
	//    	if(x[i] > 41800)
	   sum += value_buff[i+100];
//	   printf("sum:%f,index:%d\n",sum,(i+100));
	}

	 return sum;
}


/**
 * 计算最大值
 * check
 */
float float_get_max(float *buff, uint32_t size)
{
    int i;
    float max_val = buff[0];

    for (  i = 0; i < size; i++)
    {
        if (max_val < buff[i])
        {
            max_val = buff[i];
        }
    }
    return max_val;
}

/**
 * 计算最大值
 * check
 */
float float_get_max_fabs(float *buff, uint32_t size)
{
    int i;
    float max_val = fabs(buff[0]);
    float fabs_temp = 0.0f;

    for (i = 0; i < size; i++)
    {
    	fabs_temp = fabs(buff[i]);
        if (max_val < fabs_temp)
        {
            max_val = fabs_temp;
        }
    }
    return max_val;
}

/**
 * 计算最小值
 * check
 */
float float_get_min(float *buff, uint32_t size)
{
    int i;
    float min_val = buff[0];
    for (  i = 0; i < size; i++)
    {
        if (min_val >buff[i])
        {
            min_val = buff[i];
        }
    }
    return min_val;
}


/**
 * fft的C语言实现
 */

void float_fft(float *buff, int size,int FFT_POINT)
{
#ifdef MCU_ARM
	static float32_t fft_temp[4096 * 2];
	uint16_t i;
	arm_cfft_radix4_instance_f32 scfft;
	memset(fft_temp, 0, sizeof(fft_temp));
	for (i = 0; i < size; i++)
	{
		fft_temp[2 * i] = buff[i];
	}
	arm_cfft_radix4_init_f32(&scfft, size, 0, 1);
	arm_cfft_radix4_f32(&scfft, fft_temp);
	arm_cmplx_mag_f32(fft_temp, buff, size);

	for (i = 0; i < size; i++)
	{
		buff[i] = buff[i] / (size / 2);
	}
#else
	fftw_complex *din, *out;
	fftw_plan plan;
	int i;
	din = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_POINT);
	out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_POINT);

	for (i = 0; i < FFT_POINT; i++)
	{
		din[i][0] = buff[i];
		din[i][1] = 0;
	}
	plan = fftw_plan_dft_1d(FFT_POINT, din, out, FFTW_FORWARD, FFTW_ESTIMATE); //建立一个fft转换计划
	fftw_execute(plan);														   //执行转换
	fftw_destroy_plan(plan);												   //释放转换plan
	for (i = 0; i < FFT_POINT / 2; i++)
	{
		buff[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]) / FFT_POINT * 2;
	}

	fftw_free(din);
	fftw_free(out);
#endif
}

/*********************************
 * fft ifft快速滤波算法
 *　input:	buff(前1024实部,后1024虚部)   size=2048  fft_point=1024
 * output:	real   imag  fft取实部、虚部　　ifft取实部
 * ifft:	is_ifft=1
 * fft:		is_ifft=0
 ********************************/
void complex_fft(float *buff, float *real, float *imag, int size, int fft_point, uint8_t is_ifft)
{
	fftw_complex *din, *out;
	fftw_plan plan;
	int i;
	din = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fft_point);
	out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fft_point);

	if(size != 2*fft_point)
		return;

	for (i = 0; i < fft_point; i++)//1024
	{
		din[i][COMPLEX_REAL] = buff[i];//real
		din[i][COMPLEX_IMAG] = buff[i+fft_point];//imag
	}

	if(is_ifft == FFT_E)
		plan = fftw_plan_dft_1d(fft_point, din, out, FFTW_FORWARD, FFTW_ESTIMATE); //建立一个fft转换计划
	else
		plan = fftw_plan_dft_1d(fft_point, din, out, FFTW_BACKWARD, FFTW_ESTIMATE); //建立一个ifft转换计划

	fftw_execute(plan);														   //执行转换
	fftw_destroy_plan(plan);												   //释放转换plan

	for (i = 0; i < fft_point; i++)
	{
		if(is_ifft == FFT_E)
		{
			real[i] = out[i][COMPLEX_REAL];
			imag[i] = out[i][COMPLEX_IMAG];
			buff[i] = sqrt(pow(real[i],2) + pow(imag[i],2) ) / fft_point;
		}
		else
		{
			real[i] = out[i][COMPLEX_REAL]/fft_point;//如果模除去fft_point，再ifft则不用再除fft_point,此处未用fft的模作为输入
		}
	}


	fftw_free(din);
	fftw_free(out);
}

/*********************************
 * fft data 复数转换（滤波算法）
 *　input:	fft_data(前1024实部,后1024虚部)    p_real实部   p_imag虚部   fft_point=1024
 * output:	fft_data
 ********************************/
void complex_fft_convert(float *fft_data, float *p_real, float *p_imag, int fft_point)
{
	float temp_data[2048]={0};
	int i;

	if(fft_point != 1024)
		return;

	memmove((float*)temp_data, fft_data, 2048*sizeof(float));


	for(i=0; i<fft_point; i++)
	{
		fft_data[i] = temp_data[i] * p_real[i] - temp_data[i+fft_point] * p_imag[i];//实部real = x_real*h_real-x_imag*h_imag

		fft_data[i+fft_point] = temp_data[i] * p_imag[i] + temp_data[i+fft_point] * p_real[i];//虚部imag = x_real*h_imag+x_imag*h_real
	}

}

/*********************************
 * fft ifft快速滤波算法
 *　input:	fft_data
 * output:	fft_res     fft取模值　　ifft取实部
 * ifft:	is_ifft=1
 * fft:		is_ifft=0
 ********************************/
void fft_diag_deal(float *fft_data, float *fft_real, float *fft_imag, float fft_size, uint8_t is_ifft)
{
	int t0, t1, t2, t3, t4, m, N_fft, all = 0;
	float *c = NULL;
	float *z = NULL;
	float *d = NULL;
	float *fft_complex = NULL;
	float *fft_abs = NULL;

	struct CPLX u, v;
	//FFT
	//m = 13;
//	printf("------->fft_diag<---------1\n");
	m = (int)(log(fft_size)/log(2));
	//N_fft = POLYGON_DIAG_STEP;
//	printf("------->fft_diag<---------m:%d\n",m);
	N_fft = pow(2,m);					//得到真实的FFT点数
//	printf("------->fft_diag<---------N_fft:%d\n",N_fft);
//	float c[2 * POLYGON_DIAG_STEP + 10] = {0};
	c = (float *)malloc((2 * N_fft + 10)*sizeof(float));
	memset(c, 0, (2 * N_fft + 10)*sizeof(float));
//	printf("------->fft_diag<---------2\n");
//	float *z = new float[2 * N_fft + 10];
//	float z[2 * POLYGON_DIAG_STEP + 10] = {0};
	z = (float *)malloc((2 * N_fft + 10)*sizeof(float));
	memset(z, 0, (2 * N_fft + 10)*sizeof(float));
//	printf("------->fft_diag<---------3\n");
//	float *d = new float[2 * N_fft + 10];
//	float d[2 * POLYGON_DIAG_STEP + 10]= {0};
	d = (float *)malloc((2 * N_fft + 10)*sizeof(float));
	memset(d, 0, (2 * N_fft + 10)*sizeof(float));
//	printf("------->fft_diag<---------4\n");
//	float *fft_complex = new float[2 * N_fft + 10];
//	float fft_complex[2 * POLYGON_DIAG_STEP + 10] = {0};
	fft_complex = (float *)malloc((2 * N_fft + 10)*sizeof(float));
	memset(fft_complex, 0, (2 * N_fft + 10)*sizeof(float));
//	printf("------->fft_diag<---------5\n");
//	float *fft_abs = new float[N_fft];
//	float fft_abs[POLYGON_DIAG_STEP] = {0};
	fft_abs = (float *)malloc(N_fft*sizeof(float));
	memset(fft_abs, 0, sizeof(float)*N_fft);
	//printf("------->fft_diag<---------5_1\n");
	for (int i = 0; i < N_fft; i++)
	{
		if(is_ifft==IFFT_E)
		{
			c[i] = *(fft_data + i)/N_fft;
		    c[i + N_fft] = 0;
		}
		else
		{
			c[i] = *(fft_data + i);
		    c[i + N_fft] = 0;
		}
	}
//	printf("------->fft_diag<---------6\n");
	for (int i = 0; i < N_fft; i++)
	{
		if(is_ifft==IFFT_E)
		{
			z[i] = cos(2 * PI*i / N_fft);
			z[i + N_fft] = sin(2 * PI*i / N_fft);
		}
		else
		{
			z[i] = cos(2 * PI*i / N_fft);
			z[i + N_fft] = -sin(2 * PI*i / N_fft);
		}
	}
//	printf("------->fft_diag<---------7\n");
	//计算总的计算次数
	for (int n = 0; n < m; n++)
	{
		for (int k = 0; k <= pow(2, (m - n - 1)) - 1; k++)
		{
			for (int j = 0; j <= pow(2, n) - 1; j++)
			{
				all++;
			}
		}
	}
//	printf("------->fft_diag<---------8\n");
	//all *= N_fftbox_all;		//这是计算fft的总的计算次数，N_fftbox是功率谱计算中进入fft的次数
	//


	for (int n = 0; n < m; n++)
	{
		for (int k = 0; k <= pow(2, (m - n - 1)) - 1; k++)
		{
			for (int j = 0; j <= pow(2, n) - 1; j++)
			{
				t0 = pow(2, n)*k + j;
				t1 = j*pow(2, (m - n - 1));
				t2 = pow(2, n)*k + pow(2, (m - 1)) + j;
				t3 = pow(2, (n + 1))*k + j;
				t4 = pow(2, (n + 1))*k + j + pow(2, n);
				u.real = *(c + t0);
				u.imag = *(c + t0 + N_fft);
				v.real = z[t1] * c[t2] - z[t1 + N_fft] * c[t2 + N_fft];
				v.imag = z[t1 + N_fft] * c[t2] + z[t1] * c[t2 + N_fft];
				d[t3] = u.real + v.real;
				d[t3 + N_fft] = u.imag + v.imag;
				d[t4] = u.real - v.real;
				d[t4 + N_fft] = u.imag - v.imag;
			}
		}
		for (int i = 0; i < N_fft; i++)
		{
			*(c + i) = *(d + i);
			*(c + i + N_fft) = *(d + i + N_fft);
		}
	}
//	printf("------->fft_diag<---------9\n");


	for (int i = 0; i < N_fft; i++)
	{
		*(fft_complex + i) = *(c + i);
		*(fft_complex + i + N_fft) = *(c + i + N_fft);

		*(fft_real + i) = *(fft_complex + i);//实部数据
		*(fft_imag + i) = *(fft_complex + i + N_fft);//虚部数据
		//定义为*fft_complex指向的数据:   (0:N_fft) - 1为实部数据, (N_fft : 2N_fft - 1)为虚部数据
		//test[i] = *(fft_complex + i);
		//test[i + N_fft] = *(fft_complex + i + N_fft);
	}

//	printf("------->fft_diag<---------10\n");
	for (int i = 0; i < N_fft; i++)
	{
//		if(is_ifft==1)
//			*(fft_real + i) = *(fft_complex + i);//实部数据
//		else
//			*(fft_real + i) = sqrt(pow(*(fft_complex + i), 2) + pow(*(fft_complex + i + N_fft), 2)) / (float)N_fft * 2;  //fft幅频幅值换算
		*(fft_data + i) = sqrt(pow(*(fft_real + i), 2) + pow(*(fft_imag + i), 2)) / (float)N_fft * 2;  //fft幅频幅值换算
	}




//	for (int i = 0; i < N_fft; i++)
//	{
//		*(fft_real + i) = *(fft_complex + i);//实部数据
//	}
//
//	if(is_ifft==FFT_E)
//	{
//		for (int i = N_fft; i < 2*N_fft; i++)
//		{
//			*(fft_imag + i) = *(fft_complex + i);//虚部数据
//		}
//	}

//	printf("------->fft_diag<---------11\n");
	free(c);
	c = NULL;
	free(z);
	z = NULL;
	free(d);
	d = NULL;
	free(fft_complex);
	fft_complex = NULL;
	free(fft_abs);
	fft_abs = NULL;
}

float self_round(float x, uint8_t n)
{
	float z = 0.0f;

	if(x>0)
	{
		z = ((uint32_t)(x * pow(10, n) + 0.5))/pow(10, n) * 1.0;
	}
	else
	{
		z = ((uint32_t)(x * pow(10, n) - 0.5))/pow(10, n) * 1.0;
	}

	return z;
}

