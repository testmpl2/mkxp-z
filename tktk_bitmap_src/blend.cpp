#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "tktk_error.h"
#include "TktkBitmap.h"

int getDatas(DWORD, int*, int*t, PixelBGRA**);

int blendAlpha(PixelBGRA* p_dPix, PixelBGRA* p_sPix, int opacity)
{
	int aa, r, g, b;
	int sa, da;
	sa = (int)(p_sPix->a) * opacity;
	da = (int)(p_dPix->a) * (255 - sa / 255);
	if (sa < 255){return 0;}
	if (da == 0){
		p_dPix->r = p_sPix->r;
		p_dPix->g = p_sPix->g;
		p_dPix->b = p_sPix->b;
		p_dPix->a = p_sPix->a;
		return 0;
	}
	aa = da + sa;
	r = ( (int)(p_dPix->r) * da + (int)(p_sPix->r) * sa ) / aa;
	g = ( (int)(p_dPix->g) * da + (int)(p_sPix->g) * sa ) / aa;
	b = ( (int)(p_dPix->b) * da + (int)(p_sPix->b) * sa ) / aa;
	if (r > 255){r=255;}
	if (g > 255){g=255;}
	if (b > 255){b=255;}
	p_dPix->r = (BYTE)r;
	p_dPix->g = (BYTE)g;
	p_dPix->b = (BYTE)b;
	p_dPix->a = (BYTE)(aa / 255);
	return 0;
}

int blendAdd(PixelBGRA* p_dPix, PixelBGRA* p_sPix, int opacity)
{
	int sa, r, g, b;
	sa  = (int)(p_sPix->a) * opacity / 255;
	if (sa == 0){return 0;}
	if (p_dPix->a == sa ){
		r = p_dPix->r + p_sPix->r;
		g = p_dPix->g + p_sPix->g;
		b = p_dPix->b + p_sPix->b;
	} else if (p_dPix->a > sa){
		r = p_dPix->r + p_sPix->r * sa / (p_dPix->a);
		g = p_dPix->g + p_sPix->g * sa / (p_dPix->a);
		b = p_dPix->b + p_sPix->b * sa / (p_dPix->a);
	} else {
		r = p_dPix->r * (p_dPix->a) / sa + p_sPix->r;
		g = p_dPix->g * (p_dPix->a) / sa + p_sPix->g;
		b = p_dPix->b * (p_dPix->a) / sa + p_sPix->b;
		p_dPix->a = (BYTE)sa;
	}
	if (r > 255){r=255;}
	if (g > 255){g=255;}
	if (b > 255){b=255;}
	p_dPix->r = (BYTE)r;
	p_dPix->g = (BYTE)g;
	p_dPix->b = (BYTE)b;
	return 0;
}

int blendSub(PixelBGRA* p_dPix, PixelBGRA* p_sPix, int opacity)
{
	int sa, r, g, b;
	sa  = (int)(p_sPix->a) * opacity / 255;
	if (sa == 0){return 0;}
	if (p_dPix->a == sa ){
		r = p_dPix->r - p_sPix->r;
		g = p_dPix->g - p_sPix->g;
		b = p_dPix->b - p_sPix->b;
	} else if (p_dPix->a > sa){
		r = p_dPix->r - p_sPix->r * sa / (p_dPix->a);
		g = p_dPix->g - p_sPix->g * sa / (p_dPix->a);
		b = p_dPix->b - p_sPix->b * sa / (p_dPix->a);
	} else {
		r = p_dPix->r * (p_dPix->a) / sa - p_sPix->r;
		g = p_dPix->g * (p_dPix->a) / sa - p_sPix->g;
		b = p_dPix->b * (p_dPix->a) / sa - p_sPix->b;
		p_dPix->a = (BYTE)sa;
	}
	if (r < 0){r=0;}
	if (g < 0){g=0;}
	if (b < 0){b=0;}
	p_dPix->r = (BYTE)r;
	p_dPix->g = (BYTE)g;
	p_dPix->b = (BYTE)b;
	return 0;
}

int blendPixel(PixelBGRA* p_dPix, PixelBGRA* p_sPix, int blend_type, int opacity)
{
	int r,g,b;
	int dr,dg,db,sr,sg,sb;
	if (p_dPix->a == 255) {
		dr = (int)(p_dPix->r);
		dg = (int)(p_dPix->g);
		db = (int)(p_dPix->b);
	}else if (p_dPix->a == 0) {
		dr = 0;
		dg = 0;
		db = 0;
	}else{
		dr = (int)(p_dPix->r) * (p_dPix->a) / 255;
		dg = (int)(p_dPix->g) * (p_dPix->a) / 255;
		db = (int)(p_dPix->b) * (p_dPix->a) / 255;
	}
	if (p_sPix->a == 255) {
		sr = (int)(p_sPix->r);
		sg = (int)(p_sPix->g);
		sb = (int)(p_sPix->b);
	}else if (p_sPix->a == 0) {
		sr = 0;
		sg = 0;
		sb = 0;
	}else{
		sr = (int)(p_sPix->r) * (p_sPix->a) / 255;
		sg = (int)(p_sPix->g) * (p_sPix->a) / 255;
		sb = (int)(p_sPix->b) * (p_sPix->a) / 255;
	}
	switch(blend_type){
		case BLEND_NORMAL:
			blendAlpha(p_dPix, p_sPix, opacity);
			break;
		case BLEND_ADD:
			blendAdd(p_dPix, p_sPix, opacity);
			break;
		case BLEND_SUB:
			blendSub(p_dPix, p_sPix, opacity);
			break;
		case BLEND_MUL:
			r = dr * sr / 255;
			g = dg * sg / 255;
			b = db * sb / 255;
			p_dPix->r = (BYTE)r;
			p_dPix->g = (BYTE)g;
			p_dPix->b = (BYTE)b;
			p_dPix->a = 255;
			break;
		case BLEND_DODGE:
			if (sr == 255){
				r = 255;
			}else{
				r = dr * 255 / (255 - sr);
			}
			if (sg == 255){
				g = 255;
			}else{
				g = dg * 255 / (255 - sg);
			}
			if (sb == 255){
				b = 255;
			}else{
				b = db * 255 / (255 - sb);
			}
			if (r > 255){r=255;}
			if (g > 255){g=255;}
			if (b > 255){b=255;}
			p_dPix->r = (BYTE)r;
			p_dPix->g = (BYTE)g;
			p_dPix->b = (BYTE)b;
			p_dPix->a = 255;
			break;
		case BLEND_BURN:
			if (sr == 0){
				r = 0;
			}else{
				r = 255 - ( (255 - dr) * 255 / sr);
			}
			if (sg == 0){
				g = 0;
			}else{
				g = 255 - ( (255 - dg) * 255 / sg);
			}
			if (sb == 0){
				b = 0;
			}else{
				b = 255 - ( (255 - db) * 255 / sb);
			}
			if (r < 0){r=0;}
			if (g < 0){g=0;}
			if (b < 0){b=0;}
			p_dPix->r = (BYTE)r;
			p_dPix->g = (BYTE)g;
			p_dPix->b = (BYTE)b;
			p_dPix->a = 255;
			break;
		case BLEND_SCREEN:
			r = 255 - ( (255 - dr) * (255 - sr) ) / 255;
			g = 255 - ( (255 - dg) * (255 - sg) ) / 255;
			b = 255 - ( (255 - db) * (255 - sb) ) / 255;
			p_dPix->r = (BYTE)r;
			p_dPix->g = (BYTE)g;
			p_dPix->b = (BYTE)b;
			p_dPix->a = 255;
			break;
		case BLEND_OVERLAY:
			if (dr < 128){
				r = dr * sr * 2 / 255;
			}else{
				r = 2 * (dr + sr - dr * sr / 255) - 255;
			}
			if (dg < 128){
				g = dg * sg * 2 / 255;
			}else{
				g = 2 * (dg + sg - dg * sg / 255) - 255;
			}
			if (db < 128){
				b = db * sb * 2 / 255;
			}else{
				b = 2 * (db + sb - db * sb / 255) - 255;
			}
			if (r > 255){r=255;}
			if (g > 255){g=255;}
			if (b > 255){b=255;}
			p_dPix->r = (BYTE)r;
			p_dPix->g = (BYTE)g;
			p_dPix->b = (BYTE)b;
			p_dPix->a = 255;
			break;
		default:
			break;
	}
	return 0;
}

int BlendBlt(DWORD d_id, int x, int y, DWORD s_id, int r_x, int r_y, int r_width, int r_height, int blend_type, int opacity)
{
	int d_width, d_height, s_width, s_height, result;
	int drx, dry, drw, drh, srx, sry;
	int d_index, s_index;

	PixelBGRA *pDImage, *pSImage;

	if ( (result = getDatas(d_id, &d_width, &d_height, &pDImage) ) != 0 ){return result;};
	if ( (result = getDatas(s_id, &s_width, &s_height, &pSImage) ) != 0 ){return result - 100;};

	// 矩形が範囲外の場合は何もしない
	if (x >= d_width || y >= d_height){return 0;}

	drx = x;
	dry = y;
	drw = r_width;
	drh = r_height;

	srx = r_x;
	sry = r_y;


	// ソース画像と矩形のはみ出しチェック
	if (r_x < 0){
		srx = 0;
		drw -= r_x;
	}
	if (r_y < 0){
		sry = 0;
		drh -= r_y;
	}

	if (srx + drw > s_width){
		drw = s_width - srx;
	}
	if (sry + drh > s_height){
		drh = s_height - sry;
	}

	// コピー先と矩形のはみ出しチェック
	if (x < 0){
		drx = 0;
		srx -= x;
		drw += x;
	}
	if (y < 0){
		dry = 0;
		sry -= y;
		drh += y;
	}

	if (drx + drw > d_width){
		drw = d_width - drx;
	}
	if (dry + drh > d_height){
		drh = d_height - dry;
	}


	// 矩形が範囲外の場合は何もしない
	if ( drw <= 0 || drh <= 0){return 0;}

	{
		for (x = 0; x < drw; x++){
			for (y = 0; y < drh; y++){
				d_index = (d_height - y - dry - 1) * d_width + x + drx;
				s_index = (s_height - y - sry - 1) * s_width + x + srx;
				blendPixel(&pDImage[d_index], &pSImage[s_index], blend_type, opacity);
			}
		}

	}
	return 0;
}
