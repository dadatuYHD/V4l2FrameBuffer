#ifndef __JPEG_DECOMPRESS_H__
#define __JPEG_DECOMPRESS_H__

int jpeg_decompress(unsigned char rgb[], char *filepath, unsigned int *width, unsigned int *height);
void rgb24_to_bmp(unsigned char bmp[], unsigned char rgb24[],unsigned int width, unsigned int height);
#endif
