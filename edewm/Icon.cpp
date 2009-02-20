#include "Icon.h"
#include "debug.h"

// Default icon
#include "tux.xpm"

#define I(i) ((Fl_Image*)i)

static Fl_Image default_icon(tux_xpm);

uint8 *cvt1to32(XImage *xim, int ow, int oh)
{
    int pixel;
    int pitch = Fl_Renderer::calc_pitch(4, ow);
    uint8 *data = new uint8[oh*pitch];
    uint32 *ptr;
    int x,y;
    for(y = 0; y < oh; y++)
    {
        ptr = (uint32*) (data + (pitch*y));
        for(x = 0; x < ow; x++) {
            pixel = XGetPixel(xim, x, y);
            if(pixel) *ptr++ = 0x00000000;
            else *ptr++ = 0xFFFFFFFF;
        }
    }
    return data;
}

extern uint8 *ximage_to_data(XImage *im, Fl_PixelFormat *desired);

Icon::Icon(XWMHints *wm_hints)
{
    // Max size 128x128
    Fl_Rect r(0, 0, 128, 128);
    XImage *xim;

    image=0;
    mask=0;

    // ICON
    if(wm_hints && wm_hints->flags & IconPixmapHint && wm_hints->icon_pixmap)
    {
        xim = Fl_Renderer::ximage_from_pixmap(wm_hints->icon_pixmap, r);
        if(xim)
        {
            Fl_PixelFormat fmt;

            DBG("Icon format: %dx%d %d\n",xim->width, xim->height, xim->depth);
            DBG("depth/padding: %d/%d; r/g/b mask: %lx/%lx/%lx\n",
                xim->depth, xim->bitmap_pad,
                xim->red_mask, xim->green_mask, xim->blue_mask);

            uint8 *data=0;
            if(xim->depth==1) {
                data = cvt1to32(xim, xim->width, xim->height);
                fmt.realloc(32,0xFF0000,0x00FF00,0x0000FF,0);
            } else {
                data = ximage_to_data(xim, Fl_Renderer::system_format());
                fmt.copy(Fl_Renderer::system_format());
            }

            // Create Fl_Image, masks are calculated automaticly
            image = new Fl_Image(xim->width, xim->height, &fmt, data);
            image->mask_type(FL_MASK_NONE);
            XDestroyImage(xim);
        }
    }

    // MASK
    if(wm_hints && image && wm_hints->flags & IconMaskHint && wm_hints->icon_mask)
    {
        xim = Fl_Renderer::ximage_from_pixmap(wm_hints->icon_mask, r);
        if(xim) {
            uint8 *data = cvt1to32(xim, xim->width, xim->height);
            mask = new Fl_Image(xim->width, xim->height, 32, data, 0,0,0,0);
            mask->no_screen(true);
            XDestroyImage(xim);
        }
    }

    // If no icon, set default
    if(!image) {
        image = &default_icon;
        mask = 0;
    }
}

Icon::~Icon()
{
    if(image && image!=&default_icon) { delete image; image=0; }
    if(mask) { delete mask; mask=0; }

    for(ImageMap::Iterator it(images); it.current(); it++) {
        Fl_Image *i = I(it.value());
        if(i && i!=&default_icon) {
            delete i;
        }
    }
}

Fl_Image *Icon::get_icon(int W, int H)
{
    if (!image) return 0;

    Fl_Image *scaled=0, *cached=0;

    Fl_String key;
    key += Fl_String(W);
    key += 'x';
    key += Fl_String(H);

    if(image==&default_icon)
    {
        if(W!=image->width() || H!=image->height())
        {
            cached = I(images.get_value(key));
            if(cached) {
                //printf("1. Cached\n");
                return cached;
            }

            scaled = image->scale(W, H);
            scaled->set_mask(default_icon.create_scaled_bitmap_mask(W, H), true);
            images.insert(key, scaled);
            return scaled;

        } else
            return image;
    }

    cached = I(images.get_value(key));
    if(cached) {
        //printf("2. Cached\n");
        return cached;
    }

    // Check for default size image
    if(W==image->width() && H==image->height()) {
        if(mask && mask->get_mask()==0) {
            // Init mask
            mask->mask_type(MASK_COLORKEY);
            mask->colorkey(0xFFFFFFFF);
            Pixmap m = mask->create_mask(W, H);
            mask->set_mask(m, true);
            image->set_mask(mask->get_mask());
        }
        return image;
    }

    // Create NEW
    scaled = image->scale(W,H);
    if(mask) {
        Fl_Image *smask  = mask->scale(W, H);
        smask->mask_type(MASK_COLORKEY);
        smask->colorkey(0xFFFFFFFF);
        Pixmap m = smask->create_mask(W, H);
        delete smask;
        scaled->set_mask(m, true);
    }

    images.insert(key, scaled);
    return scaled;
}

