/*
 * * bitmap-binding.cpp
 **
 ** This file is part of mkxp.
 **
 ** Copyright (C) 2013 - 2021 Amaryllis Kulla <ancurio@mapleshrine.eu>
 **
 ** mkxp is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** mkxp is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "binding-types.h"
#include "binding-util.h"
#include "bitmap.h"
#include "disposable-binding.h"
#include "exception.h"
#include "font.h"
#include "sharedstate.h"
#include "graphics.h"

#if RAPI_FULL > 187
DEF_TYPE(Bitmap);
#else
DEF_ALLOCFUNC(Bitmap);
#endif

static const char *objAsStringPtr(VALUE obj) {
    VALUE str = rb_obj_as_string(obj);
    return RSTRING_PTR(str);
}

void bitmapInitProps(Bitmap *b, VALUE self) {
    /* Wrap properties */
    VALUE fontKlass = rb_const_get(rb_cObject, rb_intern("Font"));
    VALUE fontObj = rb_obj_alloc(fontKlass);
    rb_obj_call_init(fontObj, 0, 0);

    Font *font = getPrivateData<Font>(fontObj);

    rb_iv_set(self, "font", fontObj);

    // Leave property as default nil if hasHires() is false.
    GFX_GUARD_EXC(
        if (b->hasHires()) {
            b->assumeRubyGC();
            wrapProperty(self, b->getHires(), "hires", BitmapType);

            VALUE hiresFontObj = rb_obj_alloc(fontKlass);
            rb_obj_call_init(hiresFontObj, 0, 0);
            Font *hiresFont = getPrivateData<Font>(hiresFontObj);
            rb_iv_set(rb_iv_get(self, "hires"), "font", hiresFontObj);
            b->getHires()->setInitFont(hiresFont);
        }

        b->setInitFont(font);
    );
}

RB_METHOD_GUARD(bitmapInitialize) {
    Bitmap *b = 0;

    if (argc == 1) {
        char *filename;
        rb_get_args(argc, argv, "z", &filename RB_ARG_END);

        GFX_GUARD_EXC(b = new Bitmap(filename);)
    } else {
        int width, height;
        rb_get_args(argc, argv, "ii", &width, &height RB_ARG_END);

        GFX_GUARD_EXC(b = new Bitmap(width, height);)
    }

    setPrivateData(self, b);
    bitmapInitProps(b, self);

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapWidth) {
    RB_UNUSED_PARAM;

    Bitmap *b = getPrivateData<Bitmap>(self);

    int value = 0;
    value = b->width();

    return INT2FIX(value);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapHeight) {
    RB_UNUSED_PARAM;

    Bitmap *b = getPrivateData<Bitmap>(self);

    int value = 0;
    value = b->height();

    return INT2FIX(value);
}
RB_METHOD_GUARD_END

DEF_GFX_PROP_OBJ_REF(Bitmap, Bitmap, Hires, "hires")

RB_METHOD_GUARD(bitmapRect) {
    RB_UNUSED_PARAM;

    Bitmap *b = getPrivateData<Bitmap>(self);

    IntRect rect;
    rect = b->rect();

    Rect *r = new Rect(rect);

    return wrapObject(r, RectType);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapBlt) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int x, y;
    VALUE srcObj;
    VALUE srcRectObj;
    int opacity = 255;

    Bitmap *src;
    Rect *srcRect;

    rb_get_args(argc, argv, "iioo|i", &x, &y, &srcObj, &srcRectObj,
                &opacity RB_ARG_END);

    src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
    if (src) {
        srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);

        GFX_GUARD_EXC(b->blt(x, y, *src, srcRect->toIntRect(), opacity););
    }

    return self;
}
RB_METHOD_GUARD_END

// Helper function for 8-mode pixel blending
// Blend modes: 0=NORMAL, 1=ADD, 2=SUB, 3=MUL, 4=DODGE, 5=BURN, 6=SCREEN, 7=OVERLAY
static inline void blendPixelAllModes(int& r, int& g, int& b, int& a,
                                      int dr, int dg, int db, int da,
                                      int sr, int sg, int sb, int sa,
                                      int blend_type, int opacity)
{
    // Scale source alpha by opacity
    sa = (sa * opacity) / 255;

    if (sa == 0) {
        r = dr; g = dg; b = db; a = da;
        return;
    }

    auto clamp = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };

    switch (blend_type) {
        case 0: // NORMAL - Alpha compositing
        {
            int da_scaled = da * (255 - sa) / 255;
            if (da_scaled == 0) {
                r = sr; g = sg; b = sb; a = sa;
            } else {
                int aa = da_scaled + sa;
                r = (dr * da_scaled + sr * sa) / aa;
                g = (dg * da_scaled + sg * sa) / aa;
                b = (db * da_scaled + sb * sa) / aa;
                a = aa / 255;
            }
            break;
        }
        case 1: // ADD - Additive blending
        {
            if (da == sa) {
                r = dr + sr;
                g = dg + sg;
                b = db + sb;
                a = da;
            } else if (da > sa) {
                r = dr + (sr * sa / da);
                g = dg + (sg * sa / da);
                b = db + (sb * sa / da);
                a = da;
            } else {
                r = dr * da / sa + sr;
                g = dg * da / sa + sg;
                b = db * da / sa + sb;
                a = sa;
            }
            break;
        }
        case 2: // SUB - Subtractive blending (darken)
        {
            if (da == sa) {
                r = dr - sr;
                g = dg - sg;
                b = db - sb;
                a = da;
            } else if (da > sa) {
                r = dr - (sr * sa / da);
                g = dg - (sg * sa / da);
                b = db - (sb * sa / da);
                a = da;
            } else {
                r = dr * da / sa - sr;
                g = dg * da / sa - sg;
                b = db * da / sa - sb;
                a = sa;
            }
            break;
        }
        case 3: // MUL - Multiply blending
        {
            r = dr * sr / 255;
            g = dg * sg / 255;
            b = db * sb / 255;
            a = 255;
            break;
        }
        case 4: // DODGE - Color dodge (lighten, inverse multiply)
        {
            r = (sr == 255) ? 255 : (dr * 255 / (255 - sr));
            g = (sg == 255) ? 255 : (dg * 255 / (255 - sg));
            b = (sb == 255) ? 255 : (db * 255 / (255 - sb));
            a = 255;
            break;
        }
        case 5: // BURN - Color burn (darken)
        {
            r = (sr == 0) ? 0 : (255 - ((255 - dr) * 255 / sr));
            g = (sg == 0) ? 0 : (255 - ((255 - dg) * 255 / sg));
            b = (sb == 0) ? 0 : (255 - ((255 - db) * 255 / sb));
            a = 255;
            break;
        }
        case 6: // SCREEN - Screen blending (lighten)
        {
            r = 255 - ((255 - dr) * (255 - sr) / 255);
            g = 255 - ((255 - dg) * (255 - sg) / 255);
            b = 255 - ((255 - db) * (255 - sb) / 255);
            a = 255;
            break;
        }
        case 7: // OVERLAY - Overlay blending
        {
            if (dr < 128) {
                r = dr * sr * 2 / 255;
            } else {
                r = 2 * (dr + sr - dr * sr / 255) - 255;
            }
            if (dg < 128) {
                g = dg * sg * 2 / 255;
            } else {
                g = 2 * (dg + sg - dg * sg / 255) - 255;
            }
            if (db < 128) {
                b = db * sb * 2 / 255;
            } else {
                b = 2 * (db + sb - db * sb / 255) - 255;
            }
            a = 255;
            break;
        }
        default:
            r = sr; g = sg; b = sb; a = sa;
    }

    // Clamp all values
    r = clamp(r);
    g = clamp(g);
    b = clamp(b);
    a = clamp(a);
}

RB_METHOD_GUARD(bitmapBlendBlt) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int x, y;
    VALUE srcObj;
    VALUE srcRectObj;
    int blend_type = 0;
    int opacity = 255;

    Bitmap *src;
    Rect *srcRect;

    rb_get_args(argc, argv, "iioo|ii", &x, &y, &srcObj, &srcRectObj, &blend_type, &opacity RB_ARG_END);

    src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
    if (!src) {
        return self;
    }

    srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);
    if (!srcRect) {
        raiseDisposedAccess(srcRectObj);
        return self;
    }

    if (opacity <= 0) {
        return self;
    }

    GFX_GUARD_EXC(
        int dst_width = b->width();
        int dst_height = b->height();
        int src_width = src->width();
        int src_height = src->height();

        IntRect srcIntRect = srcRect->toIntRect();
        int drx = x;
        int dry = y;
        int drw = srcIntRect.w;
        int drh = srcIntRect.h;
        int srx = srcIntRect.x;
        int sry = srcIntRect.y;

        // Quick reject: destination completely out of bounds
        if (drx >= dst_width || dry >= dst_height) {
            return self;
        }

        // Clip source rect (left/top) - negative source coordinates
        if (srx < 0) {
            drw += srx;  // srx is negative, so drw shrinks
            srx = 0;
        }
        if (sry < 0) {
            drh += sry;
            sry = 0;
        }

        // Clip to source bounds (right/bottom)
        drw = (drw > src_width - srx) ? (src_width - srx) : drw;
        drh = (drh > src_height - sry) ? (src_height - sry) : drh;

        // Clip destination left/top (negative destination coordinates)
        if (drx < 0) {
            srx -= drx;  // adjust source to compensate
            drw += drx;  // drx is negative, so drw shrinks
            drx = 0;
        }
        if (dry < 0) {
            sry -= dry;
            drh += dry;
            dry = 0;
        }

        // Clip to destination bounds (right/bottom)
        drw = (drw > dst_width - drx) ? (dst_width - drx) : drw;
        drh = (drh > dst_height - dry) ? (dst_height - dry) : drh;

        // Final validation
        if (drw <= 0 || drh <= 0) {
            return self;
        }

        // Clamp blend_type to valid range
        if (blend_type < 0 || blend_type > 7) {
            blend_type = 0;
        }

        // Pixel-by-pixel blending
        for (int yy = 0; yy < drh; yy++) {
            for (int xx = 0; xx < drw; xx++) {
                int dst_x = drx + xx;
                int dst_y = dry + yy;
                int src_x = srx + xx;
                int src_y = sry + yy;

                Color dst_col = b->getPixel(dst_x, dst_y);
                Color src_col = src->getPixel(src_x, src_y);

                int r, g, b_val, a;
                blendPixelAllModes(
                    r, g, b_val, a,
                    dst_col.red, dst_col.green, dst_col.blue, dst_col.alpha,
                    src_col.red, src_col.green, src_col.blue, src_col.alpha,
                    blend_type, opacity
                );

                b->setPixel(dst_x, dst_y, Color(r, g, b_val, a));
            }
        }
    );

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapStretchBlt) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    VALUE destRectObj;
    VALUE srcObj;
    VALUE srcRectObj;
    int opacity = 255;

    Bitmap *src;
    Rect *destRect, *srcRect;

    rb_get_args(argc, argv, "ooo|i", &destRectObj, &srcObj, &srcRectObj,
                &opacity RB_ARG_END);

    src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
    if (src) {
        destRect = getPrivateDataCheck<Rect>(destRectObj, RectType);
        srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);

        GFX_GUARD_EXC(b->stretchBlt(destRect->toIntRect(), *src, srcRect->toIntRect(),
                                    opacity););
    }

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapFillRect) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    VALUE colorObj;
    Color *color;

    if (argc == 2) {
        VALUE rectObj;
        Rect *rect;

        rb_get_args(argc, argv, "oo", &rectObj, &colorObj RB_ARG_END);

        rect = getPrivateDataCheck<Rect>(rectObj, RectType);
        color = getPrivateDataCheck<Color>(colorObj, ColorType);

        GFX_GUARD_EXC(b->fillRect(rect->toIntRect(), color->norm););
    } else {
        int x, y, width, height;

        rb_get_args(argc, argv, "iiiio", &x, &y, &width, &height,
                    &colorObj RB_ARG_END);

        color = getPrivateDataCheck<Color>(colorObj, ColorType);

        GFX_GUARD_EXC(b->fillRect(x, y, width, height, color->norm););
    }

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapClear) {
    RB_UNUSED_PARAM;

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->clear();)

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetPixel) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int x, y;

    rb_get_args(argc, argv, "ii", &x, &y RB_ARG_END);

    Color value;
    if (b->surface() || b->megaSurface())
        value = b->getPixel(x, y);
    else
        GFX_GUARD_EXC(value = b->getPixel(x, y););

    Color *color = new Color(value);

    return wrapObject(color, ColorType);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapSetPixel) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int x, y;
    VALUE colorObj;

    Color *color;

    rb_get_args(argc, argv, "iio", &x, &y, &colorObj RB_ARG_END);

    color = getPrivateDataCheck<Color>(colorObj, ColorType);

    GFX_GUARD_EXC(b->setPixel(x, y, *color););

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapHueChange) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int hue;

    rb_get_args(argc, argv, "i", &hue RB_ARG_END);

    GFX_GUARD_EXC(b->hueChange(hue););

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapDrawText) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    const char *str;
    int align = Bitmap::Left;

    if (argc == 2 || argc == 3) {
        VALUE rectObj;
        Rect *rect;

        if (rgssVer >= 2) {
            VALUE strObj;
            rb_get_args(argc, argv, "oo|i", &rectObj, &strObj, &align RB_ARG_END);

            str = objAsStringPtr(strObj);
        } else {
            rb_get_args(argc, argv, "oz|i", &rectObj, &str, &align RB_ARG_END);
        }

        rect = getPrivateDataCheck<Rect>(rectObj, RectType);

        GFX_GUARD_EXC(b->drawText(rect->toIntRect(), str, align););
    } else {
        int x, y, width, height;

        if (rgssVer >= 2) {
            VALUE strObj;
            rb_get_args(argc, argv, "iiiio|i", &x, &y, &width, &height, &strObj,
                        &align RB_ARG_END);

            str = objAsStringPtr(strObj);
        } else {
            rb_get_args(argc, argv, "iiiiz|i", &x, &y, &width, &height, &str,
                        &align RB_ARG_END);
        }

        GFX_GUARD_EXC(b->drawText(x, y, width, height, str, align););
    }

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapTextSize) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    const char *str;

    if (rgssVer >= 2) {
        VALUE strObj;
        rb_get_args(argc, argv, "o", &strObj RB_ARG_END);

        str = objAsStringPtr(strObj);
    } else {
        rb_get_args(argc, argv, "z", &str RB_ARG_END);
    }

    IntRect value;
    value = b->textSize(str);

    Rect *rect = new Rect(value);

    return wrapObject(rect, RectType);
}
RB_METHOD_GUARD_END

RB_METHOD(BitmapGetFont) {
    RB_UNUSED_PARAM;
    checkDisposed<Bitmap>(self);
    return rb_iv_get(self, "font");
}
RB_METHOD_GUARD(BitmapSetFont) {
    rb_check_argc(argc, 1);
    Bitmap *b = getPrivateData<Bitmap>(self);
    VALUE propObj = *argv;

    Font *prop = getPrivateDataCheck<Font>(propObj, FontType);
    if (prop) {
        GFX_GUARD_EXC(b->setFont(*prop);)

        VALUE f = rb_iv_get(self, "font");
        if (f) {
            rb_iv_set(f, "name", rb_iv_get(propObj, "name"));
            rb_iv_set(f, "size", rb_iv_get(propObj, "size"));
            rb_iv_set(f, "bold", rb_iv_get(propObj, "bold"));
            rb_iv_set(f, "italic", rb_iv_get(propObj, "italic"));

            if (rgssVer >= 2) {
                rb_iv_set(f, "shadow", rb_iv_get(propObj, "shadow"));
            }

            if (rgssVer >= 3) {
                rb_iv_set(f, "outline", rb_iv_get(propObj, "outline"));
            }
        }
    }

    return propObj;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGradientFillRect) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    VALUE color1Obj, color2Obj;
    Color *color1, *color2;
    bool vertical = false;

    if (argc == 3 || argc == 4) {
        VALUE rectObj;
        Rect *rect;

        rb_get_args(argc, argv, "ooo|b", &rectObj, &color1Obj, &color2Obj,
                    &vertical RB_ARG_END);

        rect = getPrivateDataCheck<Rect>(rectObj, RectType);
        color1 = getPrivateDataCheck<Color>(color1Obj, ColorType);
        color2 = getPrivateDataCheck<Color>(color2Obj, ColorType);

        GFX_GUARD_EXC(b->gradientFillRect(rect->toIntRect(), color1->norm, color2->norm,
                                          vertical););
    } else {
        int x, y, width, height;

        rb_get_args(argc, argv, "iiiioo|b", &x, &y, &width, &height, &color1Obj,
                    &color2Obj, &vertical RB_ARG_END);

        color1 = getPrivateDataCheck<Color>(color1Obj, ColorType);
        color2 = getPrivateDataCheck<Color>(color2Obj, ColorType);

        GFX_GUARD_EXC(b->gradientFillRect(x, y, width, height, color1->norm,
                                          color2->norm, vertical););
    }

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapClearRect) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    if (argc == 1) {
        VALUE rectObj;
        Rect *rect;

        rb_get_args(argc, argv, "o", &rectObj RB_ARG_END);

        rect = getPrivateDataCheck<Rect>(rectObj, RectType);

        GFX_GUARD_EXC(b->clearRect(rect->toIntRect()););
    } else {
        int x, y, width, height;

        rb_get_args(argc, argv, "iiii", &x, &y, &width, &height RB_ARG_END);

        GFX_GUARD_EXC(b->clearRect(x, y, width, height););
    }

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapBlur) {
    RB_UNUSED_PARAM;

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC( b->blur(); );

    return Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapRadialBlur) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int angle, divisions;
    rb_get_args(argc, argv, "ii", &angle, &divisions RB_ARG_END);

    GFX_GUARD_EXC( b->radialBlur(angle, divisions); );

    return Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetRawData) {
    RB_UNUSED_PARAM;

    Bitmap *b = getPrivateData<Bitmap>(self);
    int size = b->width() * b->height() * 4;
    VALUE ret = rb_str_new(0, size);

    GFX_GUARD_EXC(b->getRaw(RSTRING_PTR(ret), size););

    return ret;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapSetRawData) {
    RB_UNUSED_PARAM;

    VALUE str;
    rb_scan_args(argc, argv, "1", &str);
    SafeStringValue(str);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->replaceRaw(RSTRING_PTR(str), RSTRING_LEN(str)););

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapSaveToFile) {
    RB_UNUSED_PARAM;

    VALUE str;
    rb_scan_args(argc, argv, "1", &str);
    SafeStringValue(str);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->saveToFile(RSTRING_PTR(str)););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetMega){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    VALUE ret;

    GFX_GUARD_EXC(ret = rb_bool_new(b->isMega()););

    return ret;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetAnimated){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    VALUE ret;

    GFX_GUARD_EXC(ret = rb_bool_new(b->isAnimated()););

    return ret;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetPlaying){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    VALUE ret;

    GFX_GUARD_EXC(ret = rb_bool_new(b->isPlaying()););

    return ret;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapSetPlaying){
    RB_UNUSED_PARAM;

    bool play;

    rb_get_args(argc, argv, "b", &play RB_ARG_END);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC((play) ? b->play() : b->stop(););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapPlay){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);
    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->play(););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapStop){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);
    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->stop(););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGotoStop){
    RB_UNUSED_PARAM;

    int frame;

    rb_get_args(argc, argv, "i", &frame RB_ARG_END);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->gotoAndStop(frame););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGotoPlay){
    RB_UNUSED_PARAM;

    int frame;

    rb_get_args(argc, argv, "i", &frame RB_ARG_END);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->gotoAndPlay(frame););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapFrames){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    int ret;

    GFX_GUARD_EXC(ret = b->numFrames(););

    return INT2NUM(ret);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapCurrentFrame){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    int ret;

    GFX_GUARD_EXC(ret = b->currentFrameI(););

    return INT2NUM(ret);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapAddFrame){
    RB_UNUSED_PARAM;

    VALUE srcBitmap;
    VALUE position;

    rb_scan_args(argc, argv, "11", &srcBitmap, &position);

    Bitmap *src = getPrivateDataCheck<Bitmap>(srcBitmap, BitmapType);
    if (!src)
        raiseDisposedAccess(srcBitmap);

    Bitmap *b = getPrivateData<Bitmap>(self);

    int pos = -1;
    if (position != Qnil) {
        pos = NUM2INT(position);
        if (pos < 0) pos = 0;
    }

    int ret;

    GFX_GUARD_EXC(ret = b->addFrame(*src, pos););

    return INT2NUM(ret);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapRemoveFrame){
    RB_UNUSED_PARAM;

    VALUE position;

    rb_scan_args(argc, argv, "01", &position);

    Bitmap *b = getPrivateData<Bitmap>(self);

    int pos = -1;
    if (position != Qnil) {
        pos = NUM2INT(position);
        if (pos < 0) pos = 0;
    }

    GFX_GUARD_EXC(b->removeFrame(pos););

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapNextFrame){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->nextFrame(););

    int ret;

    GFX_GUARD_EXC(ret = b->currentFrameI(););

    return INT2NUM(ret);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapPreviousFrame){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->previousFrame(););

    int ret;

    GFX_GUARD_EXC(ret = b->currentFrameI(););

    return INT2NUM(ret);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapSetFPS){
    RB_UNUSED_PARAM;

    VALUE fps;
    rb_scan_args(argc, argv, "1", &fps);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(
        if (RB_TYPE_P(fps, RUBY_T_FLOAT)) {
            b->setAnimationFPS(RFLOAT_VALUE(fps));
        }
        else {
            b->setAnimationFPS(NUM2INT(fps));
        }
    );

    return RUBY_Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetFPS){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    float ret;

    ret = b->getAnimationFPS();

    return rb_float_new(ret);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapSetLooping){
    RB_UNUSED_PARAM;

    bool loop;
    rb_get_args(argc, argv, "b", &loop RB_ARG_END);

    Bitmap *b = getPrivateData<Bitmap>(self);

    GFX_GUARD_EXC(b->setLooping(loop););

    return rb_bool_new(loop);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapGetLooping){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    bool ret;
    ret = b->getLooping();
    return rb_bool_new(ret);
}
RB_METHOD_GUARD_END

// Captures the Bitmap's current frame data to a new Bitmap
RB_METHOD_GUARD(bitmapSnapToBitmap) {
    RB_UNUSED_PARAM;

    VALUE position;
    rb_scan_args(argc, argv, "01", &position);

    Bitmap *b = getPrivateData<Bitmap>(self);

    Bitmap *newbitmap = 0;
    int pos = (position == RUBY_Qnil) ? -1 : NUM2INT(position);

    GFX_GUARD_EXC(newbitmap = new Bitmap(*b, pos););

    VALUE ret = rb_obj_alloc(rb_class_of(self));

    bitmapInitProps(newbitmap, ret);
    setPrivateData(ret, newbitmap);

    return ret;
}
RB_METHOD_GUARD_END

RB_METHOD(bitmapGetMaxSize){
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    return INT2NUM(Bitmap::maxSize());
}

RB_METHOD_GUARD(bitmapInitializeCopy) {
    rb_check_argc(argc, 1);
    VALUE origObj = argv[0];

    if (!OBJ_INIT_COPY(self, origObj))
        return self;

    Bitmap *orig = getPrivateData<Bitmap>(origObj);
    Bitmap *b = 0;

    GFX_GUARD_EXC(b = new Bitmap(*orig););

    bitmapInitProps(b, self);
    b->setFont(orig->getFont());
    setPrivateData(self, b);

    return self;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapKglInvert) {
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    b->kglInvert();
    return Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapKglCompressAlpha) {
    RB_UNUSED_PARAM;

    rb_check_argc(argc, 0);

    Bitmap *b = getPrivateData<Bitmap>(self);

    b->kglCompressAlpha();
    return Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapKglSubtractRect) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int x, y;
    VALUE srcObj;
    VALUE srcRectObj;
    int opacity = 255;

    Bitmap *src;
    Rect *srcRect;

    rb_get_args(argc, argv, "iioo|i", &x, &y, &srcObj, &srcRectObj,
                &opacity RB_ARG_END);

    src = getPrivateDataCheck<Bitmap>(srcObj, BitmapType);
    if (src) {
        srcRect = getPrivateDataCheck<Rect>(srcRectObj, RectType);
        IntRect srcIntRect = srcRect->toIntRect();
        GFX_GUARD_EXC(b->stretchBlt(IntRect(x, y, abs(srcIntRect.w), abs(srcIntRect.h)), *src, srcIntRect, opacity, false, Bitmap::KGL_SUBTRACT););
    }

    return Qnil;
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapKglShadowShaderH) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int x1, x2, y;
    bool soft;

    rb_get_args(argc, argv, "iiib", &x1, &x2, &y, &soft RB_ARG_END);

    return RB_INT2FIX(b->kglShadowShaderH(x1, x2, y, soft));
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(bitmapKglShadowShaderV) {
    Bitmap *b = getPrivateData<Bitmap>(self);

    int y1, y2, x;
    bool wall, soft;

    rb_get_args(argc, argv, "iiibb", &y1, &y2, &x, &wall, &soft RB_ARG_END);

    return RB_INT2FIX(b->kglShadowShaderV(y1, y2, x, wall, soft));
}
RB_METHOD_GUARD_END

void bitmapBindingInit() {
    VALUE klass = rb_define_class("Bitmap", rb_cObject);
    #if RAPI_FULL > 187
    rb_define_alloc_func(klass, classAllocate<&BitmapType>);
    #else
    rb_define_alloc_func(klass, BitmapAllocate);
    #endif

    disposableBindingInit<Bitmap>(klass);

    _rb_define_method(klass, "initialize", bitmapInitialize);
    _rb_define_method(klass, "initialize_copy", bitmapInitializeCopy);

    _rb_define_method(klass, "width", bitmapWidth);
    _rb_define_method(klass, "height", bitmapHeight);

    INIT_PROP_BIND(Bitmap, Hires, "hires");

    _rb_define_method(klass, "rect", bitmapRect);
    _rb_define_method(klass, "blt", bitmapBlt);
    _rb_define_method(klass, "_blend_blt", bitmapBlendBlt);
    _rb_define_method(klass, "stretch_blt", bitmapStretchBlt);
    _rb_define_method(klass, "fill_rect", bitmapFillRect);
    _rb_define_method(klass, "clear", bitmapClear);
    _rb_define_method(klass, "get_pixel", bitmapGetPixel);
    _rb_define_method(klass, "set_pixel", bitmapSetPixel);
    _rb_define_method(klass, "hue_change", bitmapHueChange);
    _rb_define_method(klass, "draw_text", bitmapDrawText);
    _rb_define_method(klass, "text_size", bitmapTextSize);

    _rb_define_method(klass, "raw_data", bitmapGetRawData);
    _rb_define_method(klass, "raw_data=", bitmapSetRawData);
    _rb_define_method(klass, "to_file", bitmapSaveToFile);

    _rb_define_method(klass, "gradient_fill_rect", bitmapGradientFillRect);
    _rb_define_method(klass, "clear_rect", bitmapClearRect);
    _rb_define_method(klass, "blur", bitmapBlur);
    _rb_define_method(klass, "radial_blur", bitmapRadialBlur);

    _rb_define_method(klass, "mega?", bitmapGetMega);
    rb_define_singleton_method(klass, "max_size", RUBY_METHOD_FUNC(bitmapGetMaxSize), -1);

    _rb_define_method(klass, "animated?", bitmapGetAnimated);
    _rb_define_method(klass, "playing", bitmapGetPlaying);
    _rb_define_method(klass, "playing=", bitmapSetPlaying);
    _rb_define_method(klass, "play", bitmapPlay);
    _rb_define_method(klass, "stop", bitmapStop);
    _rb_define_method(klass, "goto_and_stop", bitmapGotoStop);
    _rb_define_method(klass, "goto_and_play", bitmapGotoPlay);
    _rb_define_method(klass, "frame_count", bitmapFrames);
    _rb_define_method(klass, "current_frame", bitmapCurrentFrame);
    _rb_define_method(klass, "add_frame", bitmapAddFrame);
    _rb_define_method(klass, "remove_frame", bitmapRemoveFrame);
    _rb_define_method(klass, "next_frame", bitmapNextFrame);
    _rb_define_method(klass, "previous_frame", bitmapPreviousFrame);
    _rb_define_method(klass, "frame_rate", bitmapGetFPS);
    _rb_define_method(klass, "frame_rate=", bitmapSetFPS);
    _rb_define_method(klass, "looping", bitmapGetLooping);
    _rb_define_method(klass, "looping=", bitmapSetLooping);
    _rb_define_method(klass, "snap_to_bitmap", bitmapSnapToBitmap);

    INIT_PROP_BIND(Bitmap, Font, "font");

    _rb_define_method(klass, "_kgl_invert", bitmapKglInvert);
    _rb_define_method(klass, "_kgl_compress_alpha", bitmapKglCompressAlpha);
    _rb_define_method(klass, "_kgl_subtract_rect", bitmapKglSubtractRect);
    _rb_define_method(klass, "_kgl_shadow_shader_h", bitmapKglShadowShaderH);
    _rb_define_method(klass, "_kgl_shadow_shader_v", bitmapKglShadowShaderV);
}
