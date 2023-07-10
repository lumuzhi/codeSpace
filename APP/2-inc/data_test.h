/*
 * data_test.h
 *
 *  Created on: Nov 18, 2021
 *      Author: linux-ls
 */

#ifndef _DATA_TEST_H_
#define _DATA_TEST_H_

#include "global_macro.h"


#ifdef DATA_CONVERT_TXT_TO_BIN
	#define FILE_CONVERT_TXT_TO_BIN   1
	#define FILE_CONVERT_ON
#endif
#define ONE_SPEED_ONE_SEC

//#define ACC_DATA_LENGTH  (768000*2) //1500*512  因每个float后会插一个float
#define ACC_DATA_LENGTH  (30720/512)//(25600/512)//50秒数

#define ACC_SAMPLE_FRE_HZ   512

void write_acc_data();
void read_acc_data();

void init_test_data();
void sample_acc_data();

#ifdef DATA_CONVERT_TXT_TO_BIN
void data_test_thread_entry();
int init_data_test_thread();
#endif

#endif /* 2_INC_DATA_TEST_H_ */
