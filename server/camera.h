#ifndef __CAM_H__
#define __CAM_H__
#define DEBUG   1

#define MAX_BACKLOG 32
#define REQ_BUF_LEN 32
#define HDR_BUF_LEN 20

#define REQBUFS_COUNT   4
struct img {
    void * start;
    unsigned int length;
};

int camera_init(char *devpath, unsigned int *width, unsigned int *height, unsigned int *size, unsigned int *ismjpeg);
int camera_start(int fd);
int camera_dqbuf(int fd, void **buf, unsigned int *size, unsigned int *index);
int camera_eqbuf(int fd, unsigned int index);
int camera_stop(int fd);
int camera_exit(int fd);
#endif
