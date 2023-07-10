/*
 * data_test.c
 *
 *  Created on: Nov 18, 2021
 *      Author: linux-ls
 */
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/types.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <semaphore.h>
#include "user_data.h"
#include "self_test.h"
#include "ptu_app.h"
#include "data_test.h"

sem_t data_test_sem;

#if (FILE_CONVERT_TXT_TO_BIN==1)

FILE *acc_ptr = NULL;
FILE *speed_ptr = NULL;
FILE *acc_ptr1 = NULL;
FILE *speed_ptr1 = NULL;
uint32_t acc_data_cnt=0;

//float acc_data_f[ACC_SAMPLE_FRE_HZ];
//float acc_data_f1[ACC_SAMPLE_FRE_HZ];
//float speed_data_f[ACC_SAMPLE_FRE_HZ];
//float speed_data_f1[ACC_SAMPLE_FRE_HZ];

float *acc_data_f=NULL;
float *acc_data_f1=NULL;
#ifdef ONE_SPEED_ONE_SEC
	float speed_data_f=0.0;
//	float speed_data_f1=0.0;
#else
	float *speed_data_f=NULL;
//	float *speed_data_f1=NULL;
#endif
//------------------------------------------20211122
void init_test_data()
{
	if(acc_data_f == NULL)
	{
		acc_data_f = (float*)malloc(ACC_SAMPLE_FRE_HZ*sizeof(float));

		memset(acc_data_f, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));
	}

	if(acc_data_f1 == NULL)
	{
		acc_data_f1 = (float*)malloc(ACC_SAMPLE_FRE_HZ*sizeof(float));

		memset(acc_data_f1, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));
	}

#ifdef ONE_SPEED_ONE_SEC
	float speed_data_f=0.0;
//	float speed_data_f1=0.0;
#else
	if(speed_data_f == NULL)
	{
		speed_data_f = (float*)malloc(ACC_SAMPLE_FRE_HZ*sizeof(float));

		memset(speed_data_f, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));
	}

//	if(speed_data_f1 == NULL)
//	{
//		speed_data_f1 = (float*)malloc(ACC_SAMPLE_FRE_HZ*sizeof(float));
//
//		memset(speed_data_f1, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));
//	}
#endif

	acc_data_cnt = 0;
}

void write_acc_data()
{
	char acc_buf[25] = {0};
	char speed_buf[25] = {0};
	float acc_data = 0.0f;
	float speed_data = 0.0f;
	uint32_t file_inner_index = 0;
	uint16_t i = 0;

	if(acc_data_cnt == ACC_DATA_LENGTH)
		return;

	printf("acc_data_cnt=%d\n", acc_data_cnt);
	//---
	if(acc_ptr == NULL)
	{
		acc_ptr = fopen("/media/pw_data/acc_data.txt","r");
		fseek(acc_ptr, 0, SEEK_SET);
	}

	for(i=0; i<ACC_SAMPLE_FRE_HZ; i++)
	{
		fgets(acc_buf, sizeof(acc_buf), acc_ptr);
		acc_data = (float)atof(acc_buf);
		acc_data_f[i] = acc_data;
	}
	printf("acc_data_f[0] = %f\n", acc_data_f[0]);

	//---
	if(speed_ptr == NULL)
	{
		speed_ptr = fopen("/media/pw_data/speed_data.txt","r");
		fseek(speed_ptr, 0, SEEK_SET);
	}

#ifdef ONE_SPEED_ONE_SEC
	fgets(speed_buf, sizeof(speed_buf), speed_ptr);
	speed_data = (float)atof(speed_buf);
	speed_data_f = speed_data;

	file_inner_index = acc_data_cnt * sizeof(float);
#else
	for(i=0; i<ACC_SAMPLE_FRE_HZ; i++)
	{
		fgets(speed_buf, sizeof(speed_buf), speed_ptr);
		speed_data = (float)atof(speed_buf);
		speed_data_f[i] = speed_data;
	}
	printf("speed_data_f[0] = %f\n", speed_data_f[0]);

//	printf("ftell = %ld\n",ftell(acc_ptr));
#endif

	file_inner_index = acc_data_cnt*ACC_SAMPLE_FRE_HZ * sizeof(float);

	//---
	if(acc_ptr1 == NULL)
	{
		if(access("/media/pw_data/acc_data.dat", F_OK) == 0)
		{
			system("rm /media/pw_data/acc_data.dat");
			printf("del---acc_data.dat\n");
		}

		acc_ptr1 = fopen("/media/pw_data/acc_data.dat","wb+");
	}

	fseek(acc_ptr1, file_inner_index, SEEK_SET);
	fwrite(acc_data_f, ACC_SAMPLE_FRE_HZ*sizeof(float), 1, acc_ptr1);

	//---
	if(speed_ptr1 == NULL)
	{
		if(access("/media/pw_data/speed_data.dat", F_OK) == 0)
		{
			system("rm /media/pw_data/speed_data.dat");
			printf("del---speed_data.dat\n");
		}

		speed_ptr1 = fopen("/media/pw_data/speed_data.dat","wb+");
	}

#ifdef ONE_SPEED_ONE_SEC
	fseek(speed_ptr1, acc_data_cnt*sizeof(float), SEEK_SET);
	fwrite(&speed_data_f, sizeof(float), 1, speed_ptr1);
#else
	fseek(speed_ptr1, file_inner_index, SEEK_SET);
	fwrite(speed_data_f, ACC_SAMPLE_FRE_HZ*sizeof(float), 1, speed_ptr1);
#endif

	acc_data_cnt++;

	if(acc_data_cnt == ACC_DATA_LENGTH)
	{
		fclose(acc_ptr);
		acc_ptr = NULL;

		fclose(speed_ptr);
		speed_ptr = NULL;

		fclose(acc_ptr1);
		acc_ptr1 = NULL;

		fclose(speed_ptr1);
		speed_ptr1 = NULL;
	}
}

void read_acc_data()
{
	uint16_t i=0;

	printf("-------read_acc_data\n");
//	if(acc_data_cnt == 512)
//		return;

//	if(acc_ptr1 == NULL)
	{
		acc_ptr1 = fopen("/media/pw_data/acc_data.dat","r");
//		fseek(acc_ptr1, 0, SEEK_SET);
	}
//	else
//	{
//		fseek(acc_ptr1, 0, SEEK_SET);
		fseek(acc_ptr1, ACC_SAMPLE_FRE_HZ*sizeof(float), SEEK_SET);
//		fseek(acc_ptr1, -10*sizeof(float), SEEK_END);
//	}

	fread(acc_data_f1, sizeof(float), ACC_SAMPLE_FRE_HZ, acc_ptr1);//read是write的2倍
//	fread(acc_data_f, sizeof(acc_data_f), 1, acc_ptr1);

	acc_data_cnt += ACC_SAMPLE_FRE_HZ;

	for(i=0;i<10;i++)
	{
		printf("acc_data_f1[%d] = %f\n", i, acc_data_f1[i]);
	}

	for(i=0;i<10;i++)
	{
		printf("acc_data_f1[%d] = %f\n", ACC_SAMPLE_FRE_HZ-(10-i), acc_data_f1[ACC_SAMPLE_FRE_HZ-(10-i)]);
	}
//	if(acc_data_cnt == 512)
//	{
		fclose(acc_ptr1);
//		acc_ptr1 = NULL;
//	}

	return;
}
#else

//extern sem_t pw_diagnos_sem;

FILE *acc_ptr = NULL;
FILE *speed_ptr = NULL;
uint16_t acc_data_cnt;

float *acc_data_f = NULL;//512
#ifdef ONE_SPEED_ONE_SEC
	float speed_data_f = 0.0f;
#else
	float *speed_data_f = NULL;//512
	float average_speed_f = 0.0f;
#endif

void init_test_data()
{
	if(acc_data_f == NULL)
	{
		acc_data_f = (float*)malloc(ACC_SAMPLE_FRE_HZ*sizeof(float));

		memset(acc_data_f, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));
	}

#ifdef ONE_SPEED_ONE_SEC
	speed_data_f = 0.0f;
#else
	if(speed_data_f == NULL)
	{
		speed_data_f = (float*)malloc(ACC_SAMPLE_FRE_HZ*sizeof(float));

		memset(speed_data_f, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));
	}

	average_speed_f = 0.0f;
#endif

	acc_data_cnt = 0;
}

void sample_acc_data()
{
	uint16_t i=0;
#ifndef ONE_SPEED_ONE_SEC
	float speed_sum = 0.0f;
#endif

	if(acc_data_cnt == ACC_DATA_LENGTH)
		return;

	printf("sample_acc_data-------acc_data_cnt=%d\n", acc_data_cnt);

//	memset(acc_data_f, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));

	//acc
	acc_ptr = fopen("/media/pw_data/acc_data.dat","r");

	fseek(acc_ptr, acc_data_cnt*ACC_SAMPLE_FRE_HZ*sizeof(float), SEEK_SET);

	fread(acc_data_f, ACC_SAMPLE_FRE_HZ*sizeof(float), 1, acc_ptr);

	if(acc_data_cnt == 0 || acc_data_cnt == 1)
	{
		for(i=0;i<10;i++)
		{
			printf("acc_data_f1[%d] = %f\n", i, acc_data_f[i]);
		}

		for(i=0;i<10;i++)
		{
			printf("acc_data_f1[%d] = %f\n", ACC_SAMPLE_FRE_HZ-(10-i), acc_data_f[ACC_SAMPLE_FRE_HZ-(10-i)]);
		}
	}

//	memset(acc_data_f, 0, ACC_SAMPLE_FRE_HZ*sizeof(float));

	//speed
	speed_ptr = fopen("/media/pw_data/speed_data.dat","r");

#ifdef ONE_SPEED_ONE_SEC
	fseek(speed_ptr, acc_data_cnt * sizeof(float), SEEK_SET);

	fread(&speed_data_f, sizeof(float), 1, speed_ptr);

	if(acc_data_cnt == 0 || acc_data_cnt == 1)
	{
		printf("sec:%ds, speed_data_f:%f\n", acc_data_cnt, speed_data_f);
	}
#else
	fseek(speed_ptr, acc_data_cnt*ACC_SAMPLE_FRE_HZ*sizeof(float), SEEK_SET);

	fread(speed_data_f, ACC_SAMPLE_FRE_HZ*sizeof(float), 1, speed_ptr);

	for(i=0;i<ACC_SAMPLE_FRE_HZ;i++)
	{
		speed_sum += speed_data_f[i];
	}

	average_speed_f = speed_sum/ACC_SAMPLE_FRE_HZ;

	if(acc_data_cnt == 0 || acc_data_cnt == 1)
	{
		printf("sec:%ds, average_speed_f:%f\n", acc_data_cnt, average_speed_f);
	}
#endif

	acc_data_cnt++;

	fclose(acc_ptr);
	acc_ptr = NULL;

	fclose(speed_ptr);
	speed_ptr = NULL;
//	sleep(1);
}
#endif

#ifdef DATA_CONVERT_TXT_TO_BIN
void data_test_thread_entry()
{

//	sem_init(&data_test_sem,0,0);
	init_test_data();

	while(1)
	{
#if (FILE_CONVERT_TXT_TO_BIN==1)
	#ifdef FILE_CONVERT_ON//转换二进制文件
		write_acc_data();

		sleep(1);
	#else//验证
		read_acc_data();

		sleep(1);
	#endif
#else
//		sem_wait(&data_test_sem);

		printf("data_test_thread_entry\n");

		sample_acc_data();

		sleep(1);
#endif

	}
}

int init_data_test_thread()
{
	pthread_t data_test_thread_id;
	int ret = -1;
	ret=pthread_create(&data_test_thread_id,NULL,(void *)data_test_thread_entry,NULL);
	if(ret!=0)
	 {
	 	DEBUG ("Create data_test_thread error!\n");
	 }
	 	return ret;
}
#endif

