#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include "jpeg_decompress.h"
#include "fb.h"

#pragma pack(2)        //设置为2字节对齐
struct bmp_fileheader
{
    unsigned short    bfType;        
    unsigned long    bfSize;
    unsigned short    bfReverved1;
    unsigned short    bfReverved2;
    unsigned long    bfOffBits;
};
struct bmp_infoheader
{
    unsigned long    biSize;
    unsigned long    biWidth;
    unsigned long    biHeight;
    unsigned short    biPlanes;
    unsigned short    biBitCount;
    unsigned long    biCompression;
    unsigned long    biSizeImage;
    unsigned long    biXPelsPerMeter;
    unsigned long    biYpelsPerMeter;
    unsigned long    biClrUsed;
    unsigned long    biClrImportant;
};

/*************************************************
Function: jpeg_decompress
Description: 解压缩jpeg图片为rgb24格式的图片
Input: 
	rgb:指向存放rgb图片缓冲区的首地址
	filepath:待解压缩的jpeg图片的路径
Output: 将产生的rgb图片放在了rgb所指向的缓冲区
Return: 成功返回0，失败返回-1
Others: 
*************************************************/
int jpeg_decompress(unsigned char rgb24[], char *filepath,unsigned int *width, unsigned int *height)
{
	FILE * jpegfp = fopen(filepath, "rb");
	if(jpegfp == NULL)
		return -1;
	
	JSAMPARRAY buffer;  
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	cinfo.err=jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, jpegfp);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	unsigned char *rgb_p = NULL;
	rgb_p = rgb24;
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	unsigned short depth = cinfo.output_components;
	buffer = (*cinfo.mem->alloc_sarray)\
				((j_common_ptr)&cinfo,JPOOL_IMAGE,(*width)*depth,1);
				

	while(cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, buffer, 1);
		memcpy(rgb_p, *buffer, depth * (*width));
		rgb_p += depth * (*width);
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(jpegfp);
	return 0;
}

/*************************************************
Function: rgb24_to_bmp
Description: 将24位的rgb图片转换为bmp图片
Input: 
	bmp:存放bmp格式图片数据的缓冲区首地址
	rgb24:存放rgb格式图片数据的缓冲区首地址
	width:rgb图片的宽
	heigth：rgb图片的高
Output: 将bmp图片存入到bmp为首地址的缓冲区
Return: void
Others: 
*************************************************/

void rgb24_to_bmp(unsigned char bmp[], unsigned char rgb24[],unsigned int width, unsigned int height)
{
	struct bmp_fileheader bfh;
    struct bmp_infoheader bih;
	
    unsigned short depth = 3;
    unsigned long headersize = 54;
    unsigned long filesize = width * height * depth;
    
    memset(&bfh,0,sizeof(struct bmp_fileheader));
    memset(&bih,0,sizeof(struct bmp_infoheader));
	
	//填充bmp头信息   
    bfh.bfType=0x4D42;
    bfh.bfSize=filesize;
    bfh.bfOffBits=headersize;

    bih.biSize=40;
    bih.biWidth=width;
    bih.biHeight=height;
    bih.biPlanes=1;
    bih.biBitCount=(unsigned short)depth*8;
    bih.biSizeImage=width*height*depth;
	
	memcpy(bmp, &bfh, sizeof(struct bmp_fileheader));
	memcpy(bmp+ sizeof(struct bmp_fileheader), &bih, \
			sizeof(struct bmp_infoheader));
	
	unsigned char *line_buff;
    unsigned char *point;
	
    line_buff=(unsigned char *)malloc(width*depth);//申请一行rbg格式数据大小的缓冲区
    memset(line_buff,0,sizeof(unsigned char)*width*depth);

    point=rgb24+width*depth*(height-1);    //倒着写数据，bmp格式是倒的，jpg是正的
	unsigned int i, j;
   for (i=0;i<height;i++)
    {
        for (j=0;j<width * depth;j += depth)
        {
                line_buff[j+2]=point[j+0];
                line_buff[j+1]=point[j+1];
                line_buff[j+0]=point[j+2];
        }
        point-=width*depth;
      
		memcpy(bmp + 54 + i * width * depth, line_buff, width * depth);//以行的方式将数据存入bmp buff中去
   }
	free(line_buff);
}
