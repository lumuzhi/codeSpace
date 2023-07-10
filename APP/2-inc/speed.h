#ifndef _SPEED_H
#define _SPEED_H

#define FS_SPEED 20
#define FS_MIN_SPEED 15
#define SPEED_BUFF_SIZE 120*FS_MIN_SPEED
#define SPEED_SAVE_SIZE  (15*60)
/*
速度算法的结构体
该结构体只在轴承算法与多边形算法中使用
**/
struct SPEED_SAVE
{
	unsigned int speed_len;        				//该算法所对应速度的个数
	unsigned short speed_buff[SPEED_SAVE_SIZE]; //速度缓存
};

struct SPEED_PARA
{
    unsigned short speed_buff[SPEED_BUFF_SIZE];
    unsigned short speed_valid_flag;
    float  mean_speed;
};

/**
 * 获取轴承诊断的速度,4096个加速度所对应的速度
 */
float get_bearing_diag_speed(struct SPEED_SAVE *speed_save);
int get_polygon_diag_speed(float speed_buff[],struct SPEED_SAVE *speed_save);
void init_speed_para(void);
void write_speed_buff(unsigned short speed_buff[], int size);
float get_mean_speed(void);
void reset_ploy_speed_rb(void);
void reset_bear_speed_rb(void);
#endif
