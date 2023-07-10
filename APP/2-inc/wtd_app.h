#ifndef _WTD_APP_H
#define _WTD_APP_H

#include "global_macro.h"

#ifdef WTD_DATA_TEST
	#define __WTD_DEBUG__
#endif
#ifdef __WTD_DEBUG__
#define WTD_DEBUG(format,...)  printf(format,##__VA_ARGS__)//    printf//
#else
#define WTD_DEBUG(format,...)
#endif


void wtd_data_send_thread();
int init_wtd_thread();

#endif
