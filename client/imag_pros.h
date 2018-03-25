/********************************************************
*   Copyright (C) 2017 All rights reserved.
*   
*   Filename:imag_pros.h
*   Author  :YHD
*   Date    :2017-01-13
*   Describe:
*
********************************************************/
#ifndef _IMAG_PROS_H
#define _IMAG_PROS_H

#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include "cmd.h"
#include <jpeglib.h>
#include "fb.h"
#include "jpeg_decompress.h"
#include <sys/mman.h>
#include <linux/fb.h>
#include <fcntl.h>

typedef struct sockaddr SA;
typedef struct sockaddr_in SA_I;
#define imag_collect_PORT_NUM 4096
#define control_PORT_NUM 2001
#define CONTROL_SERVER "192.168.1.1"
#define IMAG_SERVER "10.202.3.183"
//#define DE_BUG 

/*rgb32 数据结构声明*/
struct rgb_32 {
	unsigned char b;
	unsigned char g;
	unsigned char r;
	unsigned char alpha;
};
/*rgb24 数据结构声明*/
struct rgb_24{
	unsigned char b;
	unsigned char g;
	unsigned char r;
};
/*外部变量声明*/
extern int imag_fd;
extern int control_fd;
extern	SA_I imag_col_addr;
extern 	SA_I control_addr;
extern struct rgb_32 rgb32;
extern struct rgb_24 rgb24;
extern pthread_mutex_t mutex;
extern unsigned int sem;
extern ssize_t picture_size_;

/*图像写入本地*/
void write_imag(char * picture, char * copy_pic_name, unsigned int picture_size);
/*图像写入本地的所有过程*/
void *imag_pross(void *arg);
/*帧处理过程*/
void *frame_process(void * arg);
/*小车控制*/
void car_control(void);
/*yuv 转换为 rgb24*/
int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width,unsigned int height); 
int convert_yuv_to_rgb_pixel(int y, int u, int v);    /*yuv格式转换为rgb格式*/  
void yuv422_to_rgb24(unsigned char *yuv422,unsigned char *rgb24, int width, int height);
#endif
