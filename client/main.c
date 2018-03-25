/********************************************************
*   Copyright (C) 2017 All rights reserved.
*
*   Filename:main.c
*   Author  :YHD
*   Date    :2017-01-13
*   Describe:
*
********************************************************/

#include "imag_pros.h"
/*全局变量定义*/
int imag_fd;
int control_fd;
pthread_mutex_t mutex;
unsigned int sem = 0;
ssize_t picture_size_ = 0;
/*图像采集服务器地址结构体*/
SA_I imag_col_addr;
/*控制服务器地址结构体*/
SA_I control_addr;

int main(int argc, char *argv[])
{
    /*图像采集服务器地址结构体初始化*/
    memset(&imag_col_addr, 0, sizeof(imag_col_addr));
    imag_col_addr.sin_family = AF_INET;
    imag_col_addr.sin_port = htons(imag_collect_PORT_NUM);
    if (-1 == inet_pton(AF_INET, IMAG_SERVER, &imag_col_addr.sin_addr))
    {
        perror("inet_pton");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        exit(-1);
    }
    /*控制服务器地址结构体初始化*/
    /*memset(&control_addr, 0, sizeof(control_addr));
    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(control_PORT_NUM);
    if (-1 == inet_pton(AF_INET, CONTROL_SERVER, &control_addr.sin_addr))
    {
        perror("inet_pton");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        exit(-1);
    }*/
    /*图像处理套接子*/
    imag_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == imag_fd)
    {
        perror("socket");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        exit(-1);
    }

    /*小车控制套接子*/
    /*control_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == control_fd)
    {
        perror("socket");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        exit(-1);
    }*/

    /*互斥锁初始化*/
    if (0 > pthread_mutex_init(&mutex, NULL))
    {
        perror("pthread_mutex_init");
        exit(-1);
    }
    /*开线程*/
    pthread_t thread1 = 0;
    pthread_t thread2 = 0;

    if (0 != pthread_create(&thread1, NULL, imag_pross, NULL))
    {
        perror("pthread_create");
        exit(-1);
    }
    if (0 != pthread_create(&thread2, NULL, frame_process, NULL))
    {
        perror("pthread_create");
        exit(-1);
    }
    /*等待线程退出
    void *retval1 = NULL;
    void *retval2 = NULL;
    if (0 != pthread_join(thread1, &retval1))
    {
        perror("pthread_join");
        exit(-1);
    }
    puts(retval1);
    if (0 !=pthread_join(thread2, &retval2))
    {
        perror("pthread_join");
        exit(-1);
    }
    puts(retval2);
    */

    /*发送小车控制命令*/
    //car_control();
    while (1)
    {
        sleep(10);
    }
    return 0;
}
