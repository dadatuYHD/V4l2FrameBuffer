#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "camera.h"
#include <signal.h>
pthread_mutex_t mutex;
struct v4l2_requestbuffers reqbufs;
struct img bufs[REQBUFS_COUNT];

int camera_init(char *devpath, unsigned int *width, unsigned int *height, unsigned int *size, unsigned int *ismjpeg)
{
    int i;
    int fd;
    int ret;
    struct v4l2_format format;         //�趨����ͷ��Ƶ��׽ģʽ
    struct v4l2_capability capability; //��ѯ�豸����

    fd = open(devpath, O_RDWR);
    if (fd == -1) {
        perror("camera->init");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        return -1;
    }

    /*��ѯ�豸����*/
    ret = ioctl(fd, VIDIOC_QUERYCAP, &capability);
    if (ret == -1) {
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        perror("camera->init");
        return -1;
    }

    if(!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "camera->init: device can not support V4L2_CAP_VIDEO_CAPTURE\n");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        close(fd);
        return -1;
    }

    if(!(capability.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "camera->init: device can not support V4L2_CAP_STREAMING\n");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        close(fd);
        return -1;
    }

    /*��ȡ��ǰ����ͷ��֧�ֵ�������Ƶ�����ʽ*/
    struct v4l2_fmtdesc fmt1;
    memset(&fmt1, 0, sizeof(fmt1));
    fmt1.index = 0;            //��ʼ��Ϊ0
    fmt1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //��ȡ֧�ֵĸ�ʽ
    while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt1)) == 0)
    {
        fmt1.index++;
        printf("{ pixelformat = '%c%c%c%c', description = '%s' }\n",fmt1.pixelformat & 0xFF,
                (fmt1.pixelformat >> 8) & 0xFF,(fmt1.pixelformat >> 16) & 0xFF,
                (fmt1.pixelformat >> 24) & 0xFF,fmt1.description);
    }

    //�趨����ͷ�����ʽ
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    ret = ioctl(fd, VIDIOC_S_FMT, &format);
    if(ret == -1)
    {
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        perror("camera_init");
    }
    else {
        fprintf(stdout, "camera->init: picture format is mjpeg\n");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        *ismjpeg = 1;
        goto get_fmt;
    }

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.width = 0;
    format.fmt.pix.height = 0;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    ret = ioctl(fd, VIDIOC_S_FMT, &format);
    if(ret == -1) {
        perror("camera init");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        return -1;
    } else {
        *ismjpeg = 0;
        fprintf(stdout, "camera->init: picture format is yuyv\n");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
    }

    /*�鿴��ǰ��Ƶ���õĲ���ģʽ*/
get_fmt:
    ret = ioctl(fd, VIDIOC_G_FMT, &format);
    if (ret == -1) {
        perror("camera init");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        return -1;
    }

    if (format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
    {
        fprintf(stdout, "camera->init: picture format is mjpeg\n");
    }

    if (format.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
        fprintf(stdout, "camera->init: picture format is yuyv\n");
    }

    /*��ͼ�����ݷ���������ں˵������ڴ�ռ�*/
    memset(&reqbufs, 0, sizeof(struct v4l2_requestbuffers));
    reqbufs.count   = REQBUFS_COUNT;
    reqbufs.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory  = V4L2_MEMORY_MMAP;
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    if (ret == -1) {
        perror("camera init");
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        close(fd);
        return -1;
    }

    /*���ں˵��ڴ�ӳ�䵽�û��ռ䣬�����û�����*/
    struct v4l2_buffer vbuf;
    for (i = 0; i < reqbufs.count; i++)
    {
        /*����ں˿ռ仺��ĵ�ַ*/
        memset(&vbuf, 0, sizeof(struct v4l2_buffer));
        vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vbuf.memory = V4L2_MEMORY_MMAP;
        vbuf.index = i;
        ret = ioctl(fd, VIDIOC_QUERYBUF, &vbuf);
        if (ret == -1) {
            perror("camera init");
            printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
            close(fd);
            return -1;
        }

        /*�ں˿ռ�ӳ�䵽�û��ռ䣬��������bufs��*/
        bufs[i].length = vbuf.length;
        bufs[i].start = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, vbuf.m.offset);
        if (bufs[i].start == MAP_FAILED)
        {
            perror("camera init");
            printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
            close(fd);
            return -1;
        }

        /*�Ż��ں˿ռ仺���ַ��һ���´����»�ȡ������е���һ�黺���ַ*/
        vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vbuf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(fd, VIDIOC_QBUF, &vbuf);
        if (ret == -1) {
            perror("camera init");
            printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
            close(fd);
            return -1;
        }
    }

    *width = format.fmt.pix.width;
    *height = format.fmt.pix.height;
    *size = bufs[0].length;
    printf("camera init success\n");
    return fd;
}
//start
int camera_start(int fd)
{
    int ret;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret == -1) {
        perror("camera->start");
        return -1;
    }
    fprintf(stdout, "camera->start: start capture\n");

    return 0;
}
//
int camera_dqbuf(int fd, void **buf, unsigned int *size, unsigned int *index)
{
    int ret;
    fd_set fds;
    struct timeval timeout;
    struct v4l2_buffer vbuf;

    while (1) {
        FD_ZERO(&fds);
        //printf("fd = %d\n", fd);
        FD_SET(fd, &fds);
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        ret = select(fd + 1, &fds, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("camera->dqbuf");
            if (errno == EINTR)
                continue;
            else
                return -1;
        } else if (ret == 0) {
            fprintf(stderr, "camera->dqbuf: dequeue buffer timeout\n");
            continue;
        } else {
            vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            vbuf.memory = V4L2_MEMORY_MMAP;
            ret = ioctl(fd, VIDIOC_DQBUF, &vbuf);
            if (ret == -1) {
                perror("camera->dqbuf");
                return -1;
            }
            *buf = bufs[vbuf.index].start;
            *size = vbuf.bytesused;
            *index = vbuf.index;

            return 0;
        }
    }
}

int camera_eqbuf(int fd, unsigned int index)
{
    int ret;
    struct v4l2_buffer vbuf;

    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    vbuf.index = index;
    ret = ioctl(fd, VIDIOC_QBUF, &vbuf);
    if (ret == -1) {
        perror("camera->eqbuf");
        return -1;
    }

    return 0;
}
//stop
int camera_stop(int fd)
{
    int ret;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret == -1) {
        perror("camera->stop");
        return -1;
    }
    fprintf(stdout, "camera->stop: stop capture\n");

    return 0;
}
//exit
int camera_exit(int fd)
{
    int i;
    int ret;
    struct v4l2_buffer vbuf;

    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;

    for (i = 0; i < reqbufs.count; i++) {
        ret = ioctl(fd, VIDIOC_DQBUF, &vbuf);
        if (ret == -1)
            break;
    }

    for (i = 0; i < reqbufs.count; i++)
        munmap(bufs[i].start, bufs[i].length);

    fprintf(stdout, "camera->exit: camera exit\n");

    return close(fd);

}






unsigned int size;
static char  video_p[1024 * 128];
int camera_fd;

void * new_thread(void *arg)
{
    char *dev_name="/dev/video0";
    unsigned int width;
    unsigned int height;
    unsigned int ismjpeg;
    unsigned int index;
    camera_fd=camera_init(dev_name,&width,&height,&size,&ismjpeg);
    if (-1 == camera_fd)
    {
        printf("%s, %d, %s\n", __FUNCTION__, __LINE__, __FILE__);
        printf("camera init is failed!\n");
        exit(-1);
    }

    /*��ʼ����ͼ������*/
    printf("camera_fd = %d\n", camera_fd);
    camera_start(camera_fd);
    printf("camera_fd = %d\n", camera_fd);

    while(1)
    {
        char *jpegbuf = NULL;
        unsigned int jpegsize = 0;

        /*��ͼ�����ݴ�ŵ��û�����ռ�*/
        camera_dqbuf(camera_fd,(void **)&jpegbuf,&jpegsize,&index);
        pthread_mutex_lock(&mutex);
        size = jpegsize;
        printf("camera data is %d\n", size);
        memcpy(video_p, jpegbuf, size);
        pthread_mutex_unlock(&mutex);
        usleep(20000);
        camera_eqbuf(camera_fd,index);
    }
}

int main(int argv,char ** argc)
{
    signal(SIGPIPE, SIG_IGN);
    pthread_t video_thread = 0;
    pthread_create(&video_thread,NULL,new_thread, NULL);

    if (0 > pthread_mutex_init(&mutex, NULL))
    {
        perror("pthread_mutex_init");
        exit(-1);
    }

    sleep(2);
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in serveraddr;
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_port=htons(4096);
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serveraddr.sin_family=AF_INET;
    int opt = 0;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(ret < 0)
        perror("set sock option error");

    ret=bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
    if(ret<0)
    {
        perror("bind failed");
        return -1;
    }

    ret=listen(sockfd,5);
    if(ret<0)
    {
        perror("listen failed");
        return -1;
    }


    struct sockaddr_in client_address;
    socklen_t client_socket_lenth = sizeof(struct sockaddr_in);
    while(1)
    {
        int client_fd=accept(sockfd,(struct sockaddr*)&client_address,&client_socket_lenth);
        char* client_ip_address=inet_ntoa(client_address.sin_addr);
        short client_port=ntohs(client_address.sin_port);
        printf("%s \n  %d\n",client_ip_address,client_port);

        while(1)
        {
            char client_buf[256]={0};
            int recvbytes;
            /*���ܿͻ��˵�ͼ�����ݻ�ȡ����*/
            recvbytes=recv(client_fd,client_buf,sizeof(client_buf),0);
            if(recvbytes==0)
            {
                printf("client exit");
                break;
            }else if(recvbytes<0){
                perror("recv failed");
                close(client_fd);
                break;
            }
            if(strcmp(client_buf,"request video")==0)
            {
                printf("client request video\n");
                /*
                   FILE * fp = fopen("t.jpg", "w+");
                   fwrite(video_p, size, 1, fp);
                   fclose(fp);
                 */
                char size_buf[10] = {0};
                pthread_mutex_lock(&mutex);
                sprintf(size_buf,"%d",size);
                printf("%s\n",size_buf);
                unsigned int send_ret=0;
                /*����ͼ�����ݵĴ�С���ͻ���*/
                int ret = send(client_fd,size_buf,sizeof(size_buf),0);
                if(ret < 0)
                {
                    perror("send picture to server failed");
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                /*����ͼ�����ݸ��ͻ���*/
                ret = send_ret = send(client_fd,video_p, size, 0);
                if(ret < 0)
                {
                    perror("send picture to server failed");
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                printf("send %d bytes\n", send_ret);
                pthread_mutex_unlock(&mutex);
            }
        }
        close(client_fd);
    }
    close(sockfd);
    camera_stop(camera_fd);
    camera_exit(camera_fd);

    return 0;
}











