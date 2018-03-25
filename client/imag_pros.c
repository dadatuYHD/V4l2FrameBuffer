/********************************************************
*   Copyright (C) 2017 All rights reserved.
*
*   Filename:imag_pros.c
*   Author  :YHD
*   Date    :2017-01-13
*   Describe:
*
********************************************************/

#include "imag_pros.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**************函数功能***************
把从服务器收到图像数据，写入本地保存
 ************************************/
void write_imag(char * picture, char * copy_pic_name, unsigned int picture_size)
{
	int fd = open(copy_pic_name, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	ssize_t ret = 0;
	ssize_t total = 0;
	while (total < picture_size)
	{
		ret = write(fd, picture + total, picture_size - total);
		if (-1 == ret)
		{
			perror("write");
			printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
			exit(-1);
		}
		else if (0 == ret)
		{
			perror("write");
			continue;
		}
		else
		{
			total += ret;
		}
	}
#ifdef DE_BUG
	printf("write picture_data to bendi %d byte\n", total);
#endif
	close(fd);
}

/**************函数功能***************
 JPEG图像处理，并做帧缓存在显示屏上**
 ************************************/
void *frame_process(void * arg)
{
	sleep(2);
	int ret;
	unsigned char rgb24[1024 * 1024 *2];
	unsigned int rgb24_width = 0;
	unsigned int rgb24_height = 0;

	/*帧缓存到屏幕上显示*/
	int fbfd = open("/dev/fb0", O_RDWR);
	if (-1 == fbfd)
	{
		perror("open");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		exit(-1);
	}

	/*获取屏幕信息*/
	struct fb_var_screeninfo  vinfo;
	ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
	if(ret < 0)
	{
		perror("FBIOGET_VSCREENINFO failed");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		exit(-1);
	}
#if 1
	printf("宽：%u\n", vinfo.xres);
	printf("高：%u\n", vinfo.yres);
	printf("虚拟宽：%u\n", vinfo.xres_virtual);
	printf("虚拟高：%u\n", vinfo.yres_virtual);
	printf("位深度：%d\n", vinfo.bits_per_pixel);
#endif

    /*设置屏幕信息*/
	vinfo.xoffset = 0;
    vinfo.yoffset = 0;
	ret = ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo);
	if(ret < 0)
	{
		perror("FBIOOUT_VSCREENINFO failed");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		exit(-1);
	}

	/*获取屏幕宽度，单位为byte*/
	struct fb_fix_screeninfo finfo;
	ret = ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);
	if(ret < 0)
	{
		perror("FBIOGET_FSCREENINFO failed");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		exit(-1);
	}

#if DE_BUG
	printf("一行所占空间：%u\n", finfo.line_length);
#endif

	/*屏幕加虚拟部分总的大小单位字节*/
	unsigned fbsize0 = finfo.line_length * vinfo.yres_virtual;
#ifdef DE_BUG
	printf("fbsize0 = %u\n", fbsize0);
#endif

	unsigned char * fbp = (unsigned char *)mmap(NULL, fbsize0, PROT_WRITE|PROT_READ, MAP_SHARED, fbfd, 0);
	if (fbp == MAP_FAILED)
	{
		perror("mmap");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		close(fbfd);
		exit(-1);
	}

	while (1)
	{
#ifdef	DE_BUG
		printf("pthread_frame lock!\n");
#endif
		/*线程上锁*/
		if (pthread_mutex_lock(&mutex))
		{
			perror("pthread_mutex_lock");
			break;
		}
		if (sem == 1)
		{
		    //printf("start jpg convert to rgb24\n");
			/*获得jpg转换后的rgb24图像数据*/
			memset(rgb24, 0, sizeof(rgb24));
			ret = jpeg_decompress(rgb24, "./pipa1.jpg", &rgb24_width, &rgb24_height);
			if (-1 == ret)
			{
				printf("jpeg imag convert to rgb_24 failed\n");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
			//printf("jpg convert to rgb24 success!\n");
            sem = 0;

			unsigned char bmp_[1024 * 1024 * 2];
			memset(bmp_, 0, sizeof(bmp_));
	        /*将rgb24图像数据转换成bmp图像数据*/
	        rgb24_to_bmp(bmp_, rgb24, rgb24_width, rgb24_height);

			struct rgb_32 * fbp_32 = (struct rgb_32 *)fbp;
			char * bmp = bmp_ + 54;
			struct rgb_24 * bmp_24 = (struct rgb_24 *)bmp_;

			/*输出图像数据到显示屏幕,小车上运行*/
			unsigned int col, row;
			for (row = 0; row < 480; row++)
			{
				for (col = 0; col < 640; col++)
				{
					(fbp_32 + row * 2048 + col)->r = (bmp_24 + row * 640 + col)->b;
					(fbp_32 + row * 2048 + col)->g = (bmp_24 + row * 640 + col)->g;
					(fbp_32 + row * 2048 + col)->b = (bmp_24 + row * 640 + col)->r;
				}
			}
			/*unsigned int col, row;
			  for (row = 0; row < 400; row++)
			  {
			  for (col = 0; col < 800; col++)
			  {
			  (fbp_32 + row * 2048 + col)->r = (bmp_24 + row * 848 + col)->b;
			  (fbp_32 + row * 2048 + col)->g = (bmp_24 + row * 848 + col)->g;
			  (fbp_32 + row * 2048 + col)->b = (bmp_24 + row * 848 + col)->r;
			  }
			  }*/
#ifdef	DE_BUG
			printf("imag display success!\n");
#endif
		}
		/*线程解锁*/
		if (pthread_mutex_unlock(&mutex)) {
			perror("pthread_mutex_unlock");
			break;
		}
#ifdef	DE_BUG
		printf("pthread_frame unlock!\n");
#endif
		usleep(20000);
	}

	ret = munmap(fbp, fbsize0);
	if (-1 == ret)
	{
		perror("munmap");
		exit(-1);
	}
	ret =  pthread_exit(NULL);
	if (-1 == ret)
	{
		perror("pthread");
		exit(-1);
	}
}

/**************函数功能**************
*收来自图像服务器的数据，并写入本地**
 ************************************/
void * imag_pross(void * arg)
{
	int connect_fd;
	//char server_pic_name[50];
	char copy_pic_name[50];
	ssize_t ret;
	char picture_size[10];
	char *picture = NULL;
	connect_fd == connect(imag_fd, (SA *)&imag_col_addr, sizeof(imag_col_addr));
	if (-1 == connect_fd)
	{
		perror("connect");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		exit(-1);
	}
	printf("connect server is success!\n");

	char get_imag[32];
	while (1)
	{
		memset(get_imag, 0, sizeof(get_imag));
		strcpy(get_imag, "request video");
		ret = send(imag_fd, get_imag, sizeof(get_imag), 0);
		if (-1 == ret)
		{
			perror("send");
			printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
			exit(-1);
		}
		else
		{
#ifdef	DE_BUG
			printf("send commond %d byte\n", ret);
#endif
		}

		/*获取图像文件大小*/
		//ssize_t picture_size_ = 0;
		memset(picture_size, 0, sizeof(picture_size));
		ret = recv(imag_fd, picture_size, sizeof(picture_size), 0);
		if (-1 == ret)
		{
			perror("recv");
			printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
			exit(-1);
		}
		else if (ret == 0)
		{
			printf("server is shutdown!\n");
		}
		else
		{
			picture_size_ = atoi(picture_size);
#ifdef	DE_BUG
			printf("picture size is %d\n", picture_size_);
#endif
		}

		/*开辟图像文件大小的内存空间*/
		ssize_t total = 0;
		picture = (char *)malloc(picture_size_);
		memset(picture, 0, sizeof(picture));
		/*把图像数据存入堆*/
#ifdef	DE_BUG
		printf("wait recv imag content!\n");
#endif
		while (total < picture_size_)
		{
			ret = recv(imag_fd, picture + total, picture_size_ - total, 0);
			if (-1 == ret)
			{
				perror("recv");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
			else if (0 < ret)
			{
				total += ret;
			}
			else
			{
				printf("ser is shutdown\n");
				break;
			}
		}
#ifdef	DE_BUG
		printf("recv picture content size is %d byte\n", total);
		printf("pthread_imag lock!\n");
#endif
		/*线程上锁*/
		if (pthread_mutex_lock(&mutex)) {
			perror("pthread_mutex_lock");
			break;
		}

		if (0 == sem)
		{
			/*把图像写入本地*/
			write_imag(picture, "./pipa1.jpg", picture_size_);
#ifdef	DE_BUG
			printf("imag write bendi success!\n");
#endif
			sem = 1;
		}
		/*线程解锁*/
		if (pthread_mutex_unlock(&mutex))
		{
			perror("pthread_mutex_unlock");
			break;
		}
#ifdef	DE_BUG
		printf("pthread_imag unlock!\n");
#endif
		free(picture);
		usleep(25000);
	}

	ret =  pthread_exit(NULL);
	if (-1 == ret)
	{
		perror("pthread");
		exit(-1);
	}
	close(imag_fd);
}

/*小车控制*/
void car_control(void)
{
	int connect_fd;
	char control[10];
	int ret;
    connect_fd = connect(control_fd, (SA *)&control_addr, sizeof(control_addr));
	if (-1 == connect_fd)
	{
		perror("connect");
		printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
		exit(-1);
	}

	while (1)
	{
		memset(control, 0, sizeof(control));
		printf("please input xiaoche control commond:\n");
		/*标准输入读取控制命令*/
		fgets(control, sizeof(control), stdin);

		/*前进*/
		if (strncmp(control, "W", 1) == 0)
		{
			ret = send(control_fd, CMD_ROLL_FORWARD, sizeof(CMD_ROLL_FORWARD), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}

		/*后退*/
		if (strncmp(control, "S", 1) == 0)
		{
			ret = send(control_fd, CMD_ROLL_BACKWARD, sizeof(CMD_ROLL_BACKWARD), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}

		/*左转*/
		if (strncmp(control, "L", 1) == 0)
		{
			ret = send(control_fd, CMD_ROLL_LEFT, sizeof(CMD_ROLL_LEFT), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}

		/*右转*/
		if (strncmp(control, "R", 1) == 0)
		{
			ret = send(control_fd, CMD_ROLL_RIGHT, sizeof(CMD_ROLL_RIGHT), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}

		/*停止*/
		if (strncmp(control, "P", 1) == 0)
		{
			ret = send(control_fd, CMD_ROLL_STOP, sizeof(CMD_ROLL_STOP), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}

		/*舵机水平转动*/
		if (strncmp(control, "DS", 2) == 0)
		{
			ret = send(control_fd, CMD_CAMERA_V, sizeof(CMD_CAMERA_V), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}

		/*舵机俯仰转动*/
		if (strncmp(control, "DF", 2) == 0)
		{
			ret = send(control_fd, CMD_CAMERA_H, sizeof(CMD_CAMERA_H), 0);
			if (-1 == ret)
			{
				perror("send");
				printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
				exit(-1);
			}
		}
	}



}

