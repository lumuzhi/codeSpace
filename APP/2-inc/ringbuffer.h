#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "assert.h"

//#define FS_SPEED 20
//#define FS_MIN_SPEED 10
//#define SPEED_BUFF_SIZE 120
typedef unsigned char rt_uint8_t;
typedef unsigned short rt_uint16_t;
typedef unsigned int rt_size_t;
#define RT_ASSERT  assert
#define RT_NULL    NULL
/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE	8
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

enum rt_ringbuffer_state
{
    RT_RINGBUFFER_EMPTY,
    RT_RINGBUFFER_FULL,
    /* half full is neither full nor empty */
    RT_RINGBUFFER_HALFFULL,
};

/* ring buffer */
struct rt_ringbuffer
{
    rt_uint8_t *buffer_ptr;
    /* use the msb of the {read,write}_index as mirror bit. You can see this as
     * if the buffer adds a virtual mirror and the pointers point either to the
     * normal or to the mirrored buffer. If the write_index has the same value
     * with the read_index, but in a different mirror, the buffer is full.
     * While if the write_index and the read_index are the same and within the
     * same mirror, the buffer is empty. The ASCII art of the ringbuffer is:
     *
     *          mirror = 0                    mirror = 1
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Full
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     *  read_idx-^                   write_idx-^
     *
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Empty
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * read_idx-^ ^-write_idx
     *
     * The tradeoff is we could only use 32KiB of buffer for 16 bit of index.
     * But it should be enough for most of the cases.
     *
     * Ref: http://en.wikipedia.org/wiki/Circular_buffer#Mirroring */
    rt_uint16_t read_mirror : 1;
    rt_uint16_t read_index : 15;
    rt_uint16_t write_mirror : 1;
    rt_uint16_t write_index : 15;
    /* as we use msb of index as mirror bit, the size should be signed and
     * could only be positive. */
    rt_uint16_t buffer_size;
};

void rt_ringbuffer_init(struct rt_ringbuffer *rb,
                        rt_uint8_t           *pool,
                        rt_uint16_t            size);



rt_uint16_t rt_ringbuffer_data_len(struct rt_ringbuffer *rb);

rt_size_t rt_ringbuffer_put(struct rt_ringbuffer *rb,
                            const rt_uint8_t     *ptr,
                            rt_uint16_t           length);

rt_size_t rt_ringbuffer_get(struct rt_ringbuffer *rb,
                            rt_uint8_t           *ptr,
                            rt_uint16_t           length);
enum rt_ringbuffer_state rt_ringbuffer_status(struct rt_ringbuffer *rb);
void rt_ringbuffer_reset(struct rt_ringbuffer *rb);

//struct SPEED_PARA
//{
//    unsigned short speed_buff[SPEED_BUFF_SIZE];
//    int speed_valid_flag;
//    unsigned short w_index;
//    unsigned short r_index;
//};
//
///**
// * 获取轴承诊断的速度,4096个加速度所对应的速度
// */
//float get_bearing_diag_speed(void);
//int get_polygon_diag_speed(float speed_buff[]);
//void init_speed_para(void);

#endif
