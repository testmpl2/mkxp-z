#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "tktk_error.h"
#include "png.h"
#include "TktkBitmap.h"

DWORD GetAddress(DWORD id)
{
	RData *rdata;
	rdata = reinterpret_cast<RData*>(id * 2);
	//if (rdata->basic.flags != 546){return ERROR_BITMAP_TYPE1;};
	if (rdata->data->bitmap == NULL){return ERROR_BITMAP_TYPE2;};
	if (rdata->data->bitmap->info == NULL){return ERROR_BITMAP_TYPE3;};
	//if (rdata->data->bitmap->info->flags != 40){return ERROR_BITMAP_TYPE4;};
	return (DWORD)(rdata->data->bitmap->data);
}

int getDatas(DWORD id, int *width, int *height, PixelBGRA **image)
{
	if (id == 0){return ERROR_BITMAP_NULL;};
	RData *rdata;
	rdata = reinterpret_cast<RData*>(id * 2);
	//if (rdata->basic.flags != 546){return ERROR_BITMAP_TYPE1;};
	if (rdata->data->bitmap == NULL){return ERROR_BITMAP_TYPE2;};
	if (rdata->data->bitmap->info == NULL){return ERROR_BITMAP_TYPE3;};
	//if (rdata->data->bitmap->info->flags != 40){return ERROR_BITMAP_TYPE4;};
	*width = rdata->data->bitmap->info->width;
	*height = rdata->data->bitmap->info->height;
	*image = (PixelBGRA*)(rdata->data->bitmap->data);
	return 0;
}


int PngSaveA(LPCSTR file_name, DWORD id, int compression_level, int filter_type)
{
	int width, height, result;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};

	png_bytep *row_pointers;
	FILE *fp;
	int	buflen;
	WCHAR wbuffer[1024];
	png_structp	png_ptr;
	png_infop info_ptr;

    buflen = MultiByteToWideChar(CP_UTF8, 0, file_name, -1, wbuffer, 0);
	MultiByteToWideChar(CP_UTF8, 0, file_name, -1, wbuffer, buflen);

	if ((compression_level > 9) || (compression_level < -1)){
		compression_level = -1;
	}

	if (_wfopen_s(&fp,wbuffer, L"wb") != 0) return 1;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fclose(fp);
		return 1;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		fclose(fp);
		return 1;
	}
	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_write_struct(&png_ptr,  &info_ptr);
		fclose(fp);
		return 1;
	}
	png_init_io(png_ptr, fp);
	png_set_filter(png_ptr, 0, filter_type);
	png_set_compression_level(png_ptr, compression_level);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	/* png_set_gAMA(png_ptr, info_ptr, 1.0); */

	png_write_info(png_ptr, info_ptr);

	row_pointers = (png_bytep *)png_malloc(png_ptr, height*png_sizeof(png_bytep));
	{
		int i;
		for (i = 0; i < height; i++){
			row_pointers[height - i - 1] = (png_bytep)(image + i * width);
		}
	}

	png_set_bgr(png_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	return 0;

}

int PngSave(LPCSTR file_name, DWORD id, int compression_level, int filter_type)
{
	return PngSaveA(file_name, id, compression_level, filter_type);
}

int ChangeTone(DWORD id, int red, int green, int blue, int simplify)
{
	int width, height, result;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};

	if (red < -256) {red = -256;}
    if (red > 256) {red = 256;}
	if (green < -256) {green = -256;}
    if (green > 256) {green = 256;}
	if (blue < -256) {blue = -256;}
    if (blue > 256) {blue = 256;}

	{
		int index;
		for (index = 0; index < width * height; index++){
			if (simplify != 0 && image[index].a == 0){
				continue;
			}
			if (blue > 0){
				image[index].b = image[index].b + ((blue * (255 - image[index].b)) >> 8);
			} else if (blue < 0){
				image[index].b = (image[index].b * (256 + blue)) >> 8;
			}
			if (green > 0){
				image[index].g = image[index].g + ((green * (255 - image[index].g)) >> 8);
			} else if (green < 0){
				image[index].g = (image[index].g * (256 + green)) >> 8;
			}
			if (red > 0){
				image[index].r = image[index].r + ((red * (255 - image[index].r)) >>8);
			} else if (red < 0){
				image[index].r = (image[index].r * (256 + red)) >> 8;
			}
		}
	}
	return 0;
}

int InvertColor(DWORD id)
{
	int width, height, result;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};
	{
		int index;
		for (index = 0; index < width * height; index++){
			image[index].r = 255 - image[index].r;
			image[index].g = 255 - image[index].g;
			image[index].b = 255 - image[index].b;
		}
	}
	return 0;
}

int Mosaic(DWORD id, int rx1, int ry1, int rw, int rh, int msw, int msh)
{
	int width, height, result;
	int rx2, ry2;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};
	rx2 = rx1 + rw; ry2 = ry1 + rh;
	if (rx1 < 0) {rx1 = 0;}
	if (ry1 < 0) {ry1 = 0;}
	if (rx2 > width) {rx2 = width;}
    if (ry2 > height) {ry2 = height;}
	if ( (rx2 <= rx1) | (ry2 <= ry1) ) {return 0;}
	if (msw < 1) {msw = 1;}
	if (msh < 1) {msh = 1;}

	{
		int x, y, index,cell_size,cw,ch;
		for (y = ry1; y < ry2; y += msh){
			if (ry2 - y < msh){
				ch = ry2 - y;
			}else{
				ch = msh;
			}
			for (x = rx1; x < rx2; x += msw){
				int cx,cy,cr,cg,cb,ca;
				if (rx2 - x < msw){
					cw = rx2 - x;
				}else{
					cw = msw;
				}
				cr=0;cg=0;cb=0;ca=0;
				cell_size = cw * ch;
				for (cx = 0; cx < cw; cx++){
					for (cy = 0; cy < ch; cy++){
						index = ((height - y - cy - 1) * width + x + cx);
						cb += image[index].b * image[index].a;
						cg += image[index].g * image[index].a;
						cr += image[index].r * image[index].a;
						ca += image[index].a;
					}
				}

				if (ca < cell_size) {
					cr=0;cg=0;cb=0;ca=0;
				}else{
					cr /= ca;
					cg /= ca;
					cb /= ca;
					ca /= cell_size;
				}
				if (cr > 255){cr = 255;}
				if (cg > 255){cg = 255;}
				if (cb > 255){cb = 255;}
				for (cx = 0; cx < cw; cx++){
					for (cy = 0; cy < ch; cy++){
						index = ((height - y - cy -1) * width + x + cx);
						image[index].b = cb;
						image[index].g = cg;
						image[index].r = cr;
						image[index].a = ca;
					}
				}
			}
		}

	}
	return 0;
}

int Blur(DWORD id, int r)
{
	int width, height, result;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};

	if (r <= 0) {return 0;};
    if (r > 1000) {r=1000;};
	{
		int x, y, index,ci,ci2,rw;
		PBuf  *buffer, *buffer2;

		buffer = new PBuf[width * height];
		buffer2 = new PBuf[width * height];
		rw = r * width;

		for (y = 0; y < height; y++){
			index = y * width;
			buffer[index].r = 0;
			buffer[index].g = 0;
			buffer[index].b = 0;
			buffer[index].a = 0;
			buffer[index].w = 0;

			x = 0;
			if (r < width){
				ci2 = index + r;
			}else{
				ci2 = index + width - 1;
			}

			for (ci = index; ci <= ci2; ci++){
				buffer[index].r += image[ci].r * image[ci].a;
				buffer[index].g += image[ci].g * image[ci].a;
				buffer[index].b += image[ci].b * image[ci].a;
				buffer[index].a += image[ci].a;
				buffer[index].w += 1;
			}
			for (x = 1; x < width; x++){
				buffer[index + 1] = buffer[index];

				if (buffer[index].a < buffer[index].w){
					buffer[index].r = 0;
					buffer[index].g = 0;
					buffer[index].b = 0;
					buffer[index].a = 0;
				}else{
					buffer[index].r /= buffer[index].a;
					buffer[index].g /= buffer[index].a;
					buffer[index].b /= buffer[index].a;
					buffer[index].a /= buffer[index].w;
				}
				index++;
				if (x > r) {
					buffer[index].r -= image[index - r - 1].r * image[index - r - 1].a;
					buffer[index].g -= image[index - r - 1].g * image[index - r - 1].a;
					buffer[index].b -= image[index - r - 1].b * image[index - r - 1].a;
					buffer[index].a -= image[index - r - 1].a;
					buffer[index].w -= 1;
				};
				if ((x + r) < width) {
					buffer[index].r += image[index + r].r * image[index + r].a;
					buffer[index].g += image[index + r].g * image[index + r].a;
					buffer[index].b += image[index + r].b * image[index + r].a;
					buffer[index].a += image[index + r].a;
					buffer[index].w += 1;
				};
			}

			if (buffer[index].a < buffer[index].w){
				buffer[index].r = 0;
				buffer[index].g = 0;
				buffer[index].b = 0;
				buffer[index].a = 0;
			}else{
				buffer[index].r /= buffer[index].a;
				buffer[index].g /= buffer[index].a;
				buffer[index].b /= buffer[index].a;
				buffer[index].a /= buffer[index].w;
			}
		}
		for (x = 0; x < width; x++){
			y = 0;
			index = x;
			buffer2[index].r = 0;
			buffer2[index].g = 0;
			buffer2[index].b = 0;
			buffer2[index].a = 0;
			buffer2[index].w = 0;
			if (r < height){
				ci2 = index + r * width;
			}else{
				ci2 = index + (height - 1) * width;
			}

			for (ci = index; ci <= ci2; ci += width){
				buffer2[index].r += buffer[ci].r * buffer[ci].a;
				buffer2[index].g += buffer[ci].g * buffer[ci].a;
				buffer2[index].b += buffer[ci].b * buffer[ci].a;
				buffer2[index].a += buffer[ci].a;
				buffer2[index].w += 1;
			}
			if (buffer2[index].a < buffer2[index].w){
				image[index].r = 0;
				image[index].g = 0;
				image[index].b = 0;
				image[index].a = 0;
			}else{
				image[index].r = buffer2[index].r / buffer2[index].a;
				image[index].g = buffer2[index].g / buffer2[index].a;
				image[index].b = buffer2[index].b / buffer2[index].a;
				image[index].a = buffer2[index].a / buffer2[index].w;
			}

			for (y = 1; y < height; y++){
				buffer2[index + width] = buffer2[index];

				index += width;
				if (y > r) {
					buffer2[index].r -= buffer[index - rw - width].r * buffer[index - rw - width].a;
					buffer2[index].g -= buffer[index - rw - width].g * buffer[index - rw - width].a;
					buffer2[index].b -= buffer[index - rw - width].b * buffer[index - rw - width].a;
					buffer2[index].a -= buffer[index - rw - width].a;
					buffer2[index].w -= 1;
				};
				if ((y + r) < height) {
					buffer2[index].r += buffer[index + rw].r * buffer[index + rw].a;
					buffer2[index].g += buffer[index + rw].g * buffer[index + rw].a;
					buffer2[index].b += buffer[index + rw].b * buffer[index + rw].a;
					buffer2[index].a += buffer[index + rw].a;
					buffer2[index].w += 1;
				};
				if (buffer2[index].a < buffer2[index].w){
					image[index].r = 0;
					image[index].g = 0;
					image[index].b = 0;
					image[index].a = 0;

				}else{
					image[index].r = buffer2[index].r / buffer2[index].a;
					image[index].g = buffer2[index].g / buffer2[index].a;
					image[index].b = buffer2[index].b / buffer2[index].a;
					image[index].a = buffer2[index].a / buffer2[index].w;
				}
			}
		}
		delete [] buffer2;
		delete [] buffer;
	}
	return 0;
}


int ClipMask(DWORD g_id, DWORD m_id, int m_x, int m_y, int outer)
{
	int g_width, g_height, m_width, m_height,result;
	int x, y, ma, gindex, mindex;

	PixelBGRA *image;
	PixelBGRA *mask;

	if( (result = getDatas(g_id, &g_width, &g_height, &image) ) != 0 ){return result;};
	if( (result = getDatas(m_id, &m_width, &m_height, &mask) ) != 0 ){return result - 100;};
	{
		for (x = 0; x < g_width; x++){
			for (y = 0; y < g_height; y++){
				gindex = (g_height - y - 1) * g_width + x;
				if ( (m_x > x) | ( (m_x + m_width) <= x) | (m_y > y) | ( (m_y + m_height) <= y )){
					ma = outer;
				}else{
					mindex = (m_height - y + m_y - 1) * m_width + x - m_x;
					ma = (int) mask[mindex].b;
				}
				image[gindex].a = (unsigned char)(ma * (int)(image[gindex].a) / 255);
			}
		}

	}
	return 0;
}

int GetPixelData(DWORD id, void *buffer, int bufferSize)
{
	int width, height, result;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};
	if (bufferSize != width*height*4) {return -100;};
	memcpy_s(buffer, bufferSize, image, width*height*4);
	return 0;
}

int SetPixelData(DWORD id, void *buffer, int bufferSize)
{
	int width, height, result;
	PixelBGRA *image;

	if( (result = getDatas(id, &width, &height, &image) ) != 0 ){return result;};
	if (bufferSize != width*height*4) {return -100;};
	memcpy_s(image, width*height*4, buffer, bufferSize);
	return 0;
}

int ChangeSize(DWORD id, int newWidth, int newHeight)
{
	RData *rdata;
	size_t newSize;
	void* newAddress;
	void* image;
    // return Error if wrong size
	if ( (newWidth <= 0) | (newHeight <= 0) ){
		return ERROR_SIZE_INVALID;
	}
	newSize = newWidth * newHeight * 4;

	if (id == 0){return ERROR_BITMAP_NULL;};
	rdata = reinterpret_cast<RData*>(id * 2);
	if (rdata->basic.flags != 546){return ERROR_BITMAP_TYPE1;};
	if (rdata->data->bitmap == NULL){return ERROR_BITMAP_TYPE2;};
	if (rdata->data->bitmap->info == NULL){return ERROR_BITMAP_TYPE3;};
	if (rdata->data->bitmap->info->flags != 40){return ERROR_BITMAP_TYPE4;};
	image = rdata->data->bitmap->data;

	newAddress = malloc(newSize);
	if (newAddress != NULL){
		free(image);
		ZeroMemory(newAddress, newSize);
		rdata->data->bitmap->info->width = newWidth;
		rdata->data->bitmap->info->height = newHeight;
		rdata->data->bitmap->data = newAddress;
	}else{
		return ERROR_ALLOCATE_FAILED;
	}
	return 0;
}
