#ifndef __TKTKBITMAP_H__
#define __TKTKBITMAP_H__

#define BLEND_NORMAL  0x00
#define BLEND_ADD     0x01
#define BLEND_SUB     0x02
#define BLEND_MUL     0x03
#define BLEND_DODGE   0x04
#define BLEND_BURN    0x05
#define BLEND_SCREEN  0x06
#define BLEND_OVERLAY 0x07

typedef struct{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
} PixelBGRA;

typedef struct{
	unsigned int r;
	unsigned int g;
	unsigned int b;
	unsigned int a;
	unsigned int w;
} PBuf;

typedef struct {
	DWORD flags;
	DWORD klass;
} RBasic;

typedef struct{
	int flags;
	int width;
	int height;
} RBmInfo;

typedef struct{
	RBasic  basic;
	RBmInfo *info;
	void    *params;
	void    *data;
} RBitmap;

typedef struct{
	RBasic  basic;
	RBitmap *bitmap;
} RObject;

typedef struct{
	RBasic basic;
	void *dmark;
	void *dfree;
	RObject *data;
} RData;


extern "C"{
	__declspec(dllexport) int Version(void);
	__declspec(dllexport) int PngSaveA(LPCSTR, DWORD, int, int);
	__declspec(dllexport) int PngSave(LPCSTR, DWORD, int, int);
	__declspec(dllexport) int ChangeTone(DWORD, int, int, int, int);
	__declspec(dllexport) int InvertColor(DWORD);
	__declspec(dllexport) int Mosaic(DWORD, int, int, int, int, int, int);
	__declspec(dllexport) int Blur(DWORD, int);
	__declspec(dllexport) int ClipMask(DWORD, DWORD, int, int, int);
	__declspec(dllexport) int BlendBlt(DWORD, int, int, DWORD, int, int, int, int, int, int);
	__declspec(dllexport) DWORD GetAddress(DWORD);
	__declspec(dllexport) int GetPixelData(DWORD, LPVOID, int);
	__declspec(dllexport) int SetPixelData(DWORD, LPVOID, int);
	__declspec(dllexport) int ChangeSize(DWORD, int, int);
}

#endif
