/*
 * crc_table.h
 *
 *  Created on: Sep 29, 2018
 *      Author: zc
 */
#ifndef _CRC_TABLE_H
#define _CRC_TABLE_H

#define  FCS_BIT  0xffffffff
unsigned int fcs32(const unsigned char *buf, unsigned int len, unsigned int fcs);
unsigned short check_sum16(const unsigned char *buf, unsigned int len);


#endif



