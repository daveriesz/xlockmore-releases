/* ximage-loader.c --- converts image files or data to XImages or Pixmap.
 * xscreensaver, Copyright (c) 1998-2018 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_JWXYZ
# include "jwxyz.h"
#else
# include <X11/Xlib.h>
# include <X11/Xutil.h>
#endif

#include "ximage-loader.h"

#if defined(HAVE_GDK_PIXBUF) || defined(HAVE_COCOA) || defined(HAVE_ANDROID)
# undef HAVE_LIBPNG
#endif

#ifdef HAVE_COCOA
# include "grabscreen.h"  /* for osx_load_image_file() */
#endif

#ifdef HAVE_GDK_PIXBUF
# include <gdk-pixbuf/gdk-pixbuf.h>
# ifdef HAVE_GTK2
#  include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
# else  /* !HAVE_GTK2 */
#  include <gdk-pixbuf/gdk-pixbuf-xlib.h>
# endif /* !HAVE_GTK2 */
#endif /* HAVE_GDK_PIXBUF */

#ifdef HAVE_LIBPNG
# include <png.h>
#endif

#ifdef HAVE_ANDROID
 /* So that debug output shows up in logcat... */
extern void Log(const char *format, ...);
# undef  fprintf
# define fprintf(S, ...) Log(__VA_ARGS__)
#endif

extern char *progname;

static Bool
bigendian (void)
{
  union { int i; char c[sizeof(int)]; } u;
  u.i = 1;
  return !u.c[0];
}


#ifdef HAVE_GDK_PIXBUF

/* Loads the image to an XImage, RGBA -- GDK Pixbuf version.
 */
static XImage *
make_ximage (Display *dpy, Visual *visual, const char *filename,
             const unsigned char *image_data, unsigned long data_size)
{
  GdkPixbuf *pb;
  static int initted = 0;
# ifdef HAVE_GTK2
  GError *gerr = NULL;
# endif

  if (!initted)
    {
# ifdef HAVE_GTK2
#  if !GLIB_CHECK_VERSION(2, 36 ,0)
      g_type_init ();
#  endif
# endif
      gdk_pixbuf_xlib_init (dpy, DefaultScreen (dpy));
      xlib_rgb_init (dpy, DefaultScreenOfDisplay (dpy));
      initted = 1;
    }

  if (filename)
    {
# ifdef HAVE_GTK2
      pb = gdk_pixbuf_new_from_file (filename, &gerr);
      if (!pb)
        {
          fprintf (stderr, "%s: %s\n", progname, gerr->message);
          return 0;
        }
# else
      pb = gdk_pixbuf_new_from_file (filename);
      if (!pb)
        {
          fprintf (stderr, "%s: GDK unable to load %s: %s\n",
                   progname, filename, (gerr ? gerr->message : "?"));
          return 0;
        }
# endif /* HAVE_GTK2 */
    }
  else
    {
# ifdef HAVE_GTK2
      GInputStream *s =
        g_memory_input_stream_new_from_data (image_data, data_size, 0);
      pb = gdk_pixbuf_new_from_stream (s, 0, &gerr);

      g_input_stream_close (s, NULL, NULL);
      g_object_unref (s);

      if (! pb)
        {
          /* fprintf (stderr, "%s: GDK unable to parse image data: %s\n",
                   progname, (gerr ? gerr->message : "?")); */
          return 0;
        }
# else /* !HAVE_GTK2 */
      fprintf (stderr, "%s: image loading not supported with GTK 1.x\n",
               progname);
      return 0;
# endif /* !HAVE_GTK2 */
    }

  if (!pb) abort();

  {
    XImage *image;
    int w = gdk_pixbuf_get_width (pb);
    int h = gdk_pixbuf_get_height (pb);
    guchar *row = gdk_pixbuf_get_pixels (pb);
    int stride = gdk_pixbuf_get_rowstride (pb);
    int chan = gdk_pixbuf_get_n_channels (pb);
    int x, y;

    image = XCreateImage (dpy, visual, 32, ZPixmap, 0, 0, w, h, 32, 0);
    image->data = (char *) malloc(h * image->bytes_per_line);

    /* Set the bit order in the XImage structure to whatever the
       local host's native bit order is.
    */
    image->bitmap_bit_order =
      image->byte_order =
      (bigendian() ? MSBFirst : LSBFirst);

    if (!image->data)
      {
        fprintf (stderr, "%s: out of memory (%d x %d)\n", progname, w, h);
        return 0;
      }

    for (y = 0; y < h; y++)
      {
        guchar *i = row;
        for (x = 0; x < w; x++)
          {
            unsigned long rgba = 0;
            switch (chan) {
            case 1:
              rgba = ((0xFF << 24) |
                      (*i   << 16) |
                      (*i   <<  8) |
                       *i);
              i++;
              break;
            case 3:
              rgba = ((0xFF << 24) |
                      (i[2] << 16) |
                      (i[1] <<  8) |
                      i[0]);
              i += 3;
              break;
            case 4:
              rgba = ((i[3] << 24) |
                      (i[2] << 16) |
                      (i[1] <<  8) |
                      i[0]);
              i += 4;
              break;
            default:
              abort();
              break;
            }
            XPutPixel (image, x, y, rgba);
          }
        row += stride;
      }

    g_object_unref (pb);
    return image;
  }
}

#elif defined(HAVE_JWXYZ) /* MacOS, iOS or Android */

/* Loads the image to an XImage, RGBA -- MacOS, iOS or Android version.
 */
static XImage *
make_ximage (Display *dpy, Visual *visual, const char *filename,
             const unsigned char *image_data, unsigned long data_size)
{
  XImage *ximage = 0;

  if (filename)
    {
# ifdef HAVE_COCOA  /* MacOS */
      XRectangle geom;
      Screen *screen = DefaultScreenOfDisplay (dpy);
      Window window = RootWindowOfScreen (screen);
      XWindowAttributes xgwa;
      XGetWindowAttributes (dpy, window, &xgwa);
      Pixmap pixmap =
        XCreatePixmap (dpy, window, xgwa.width, xgwa.height, xgwa.depth);
      int x, y;

      if (! osx_load_image_file (screen, window, pixmap, filename, &geom))
        {
          fprintf (stderr, "%s: %s failed\n", progname, filename);
          return 0;
        }

      ximage = XGetImage (dpy, pixmap, geom.x, geom.y,
                          geom.width, geom.height,
                          ~0L, ZPixmap);
      if (!ximage) abort();

      /* Have to convert ABGR to RGBA */
      for (y = 0; y < ximage->height; y++)
        for (x = 0; x < ximage->width; x++)
          {
            unsigned long p = XGetPixel (ximage, x, y);
            unsigned long a = (p >> 24) & 0xFF;
            unsigned long b = (p >> 16) & 0xFF;
            unsigned long g = (p >>  8) & 0xFF;
            unsigned long r = (p >>  0) & 0xFF;
            p = (r << 24) | (g << 16) | (b << 8) | (a << 0);
            XPutPixel (ximage, x, y, p);
          }

      XFreePixmap (dpy, pixmap);

# else   /* !HAVE_COCOA -- iOS or Android. */
      fprintf (stderr, "%s: image file loading not supported\n", progname);
      return 0;
# endif  /* !HAVE_COCOA */
    }
  else
    {
      ximage = jwxyz_png_to_ximage (dpy, visual, image_data, data_size);
    }

  return ximage;
}

#elif defined(HAVE_LIBPNG)

typedef struct {
  const unsigned char *buf;
  png_size_t siz, ptr;
} png_read_closure;

static void
png_reader_fn (png_structp png_ptr, png_bytep buf, png_size_t siz)
{
  png_read_closure *r = png_get_io_ptr (png_ptr);
  if (siz > r->siz - r->ptr)
    png_error (png_ptr, "PNG internal read error");
  memcpy (buf, r->buf + r->ptr, siz);
  r->ptr += siz;
}


/* Loads the image to an XImage, RGBA -- libpng version.
 */
static XImage *
make_ximage (Display *dpy, Visual *visual,
             const char *filename, const unsigned char *image_data,
             unsigned long data_size)
{
  XImage *image = 0;
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_info;
  png_uint_32 width, height, channels;
  int bit_depth, color_type, interlace_type;
  FILE *fp = 0;

  png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (!png_ptr) return 0;

  info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct (&png_ptr, 0, 0);
      return 0;
    }

  end_info = png_create_info_struct (png_ptr);
  if (!end_info)
    {
      png_destroy_read_struct (&png_ptr, &info_ptr, 0);
      return 0;
    }

  if (setjmp (png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
      return 0;
    }

  if (filename)
    {
      fp = fopen (filename, "r");
      if (! fp)
        {
          fprintf (stderr, "%s: unable to read %s\n", progname, filename);
          return 0;
        }
      png_init_io (png_ptr, fp);
    }
  else
    {
      png_read_closure closure;
      closure.buf = image_data;
      closure.siz = data_size;
      closure.ptr = 0;
      png_set_read_fn (png_ptr, (void *) &closure, png_reader_fn);
    }

  png_read_info (png_ptr, info_ptr);
  png_get_IHDR (png_ptr, info_ptr,
                &width, &height, &bit_depth, &color_type,
                &interlace_type, 0, 0);

  png_set_strip_16 (png_ptr);  /* Truncate 16 bits per component to 8 */
  png_set_packing (png_ptr);   /* Unpack to 1 pixel per byte */

# if 0
  if (color_type == PNG_COLOR_TYPE_PALETTE)  /* Colormap to RGB */
    png_set_palette_rgb (png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)  /* Mono to 8bit */
    png_set_gray_1_2_4_to_8 (png_ptr);
# endif

  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS)) /* Fix weird alpha */
    png_set_tRNS_to_alpha (png_ptr);

  /* At least 8 bits deep */
  if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
    png_set_expand (png_ptr);

   if (bit_depth == 8 &&          /* Convert RGB to RGBA */
           (color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_PALETTE))
     png_set_filler (png_ptr, 0xFF, PNG_FILLER_AFTER);

  /* Grayscale to color */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand (png_ptr);


  /* Convert graysale to color */
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png_ptr);

# if 0
  {
    png_color_16 *bg;
    if (png_get_bKGD (png_ptr, info_ptr, &bg))
      png_set_background (png_ptr, bg, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
  }
# endif

  /* Commit */
  png_read_update_info (png_ptr, info_ptr);

  channels = png_get_channels (png_ptr, info_ptr);

  {
    png_bytep *rows = png_malloc (png_ptr, height * sizeof(*rows));
    int x, y;
    for (y = 0; y < height; y++)
      rows[y] = png_malloc (png_ptr, png_get_rowbytes (png_ptr, info_ptr));
    png_read_image (png_ptr, rows);
    png_read_end (png_ptr, info_ptr);

    image = XCreateImage (dpy, visual, 32, ZPixmap, 0, 0,
                          width, height, 32, 0);
    image->data = (char *) malloc (height * image->bytes_per_line);

    /* Set the bit order in the XImage structure to whatever the
       local host's native bit order is.
     */
    image->bitmap_bit_order =
      image->byte_order =
        (bigendian() ? MSBFirst : LSBFirst);

    if (!image->data)
      {
        fprintf (stderr, "%s: out of memory (%lu x %lu)\n",
                 progname, (unsigned long)width, (unsigned long)height);
        return 0;
      }

    for (y = 0; y < height; y++)
      {
        png_bytep i = rows[y];
        for (x = 0; x < width; x++)
          {
            unsigned long rgba;
            switch (channels) {
            case 4:
              rgba = ((i[3] << 24) |
                      (i[2] << 16) |
                      (i[1] << 8)  |
                       i[0]);
              break;
            case 3:
              rgba = ((0xFF << 24) |
                      (i[2] << 16) |
                      (i[1] << 8)  |
                       i[0]);
              break;
            case 2:
              rgba = ((i[1] << 24) |
                      (i[0] << 16) |
                      (i[0] << 8)  |
                       i[0]);
              break;
            case 1:
              rgba = ((0xFF << 24) |
                      (i[0] << 16) |
                      (i[0] << 8)  |
                       i[0]);
              break;
            default:
              abort();
            }
            XPutPixel (image, x, y, rgba);
            i += channels;
          }
        png_free (png_ptr, rows[y]);
      }

    png_free (png_ptr, rows);
  }

  png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);  
  if (fp) fclose (fp);

  return image;
}


#else /* No image loaders! */

static XImage *
make_ximage (Display *dpy, Visual *visual,
             const char *filename, const unsigned char *image_data,
             unsigned long data_size)
{
  fprintf (stderr, "%s: no image loading support!\n", progname);
  return 0;
}

#endif /* no loaders */


/* Given a bitmask, returns the position and width of the field.
 */
static void
decode_mask (unsigned long mask, unsigned long *pos_ret,
             unsigned long *size_ret)
{
  int i;
  for (i = 0; i < 32; i++)
    if (mask & (1L << i))
      {
        int j = 0;
        *pos_ret = i;
        for (; i < 32; i++, j++)
          if (! (mask & (1L << i)))
            break;
        *size_ret = j;
        return;
      }
}


/* Loads the image to a Pixmap and optional 1-bit mask.
 */
static Pixmap
make_pixmap (Display *dpy, Window window,
             const char *filename,
             const unsigned char *image_data, unsigned long data_size,
             int *width_ret, int *height_ret, Pixmap *mask_ret)
{
  XWindowAttributes xgwa;
  XImage *in, *out, *mask = 0;
  Pixmap pixmap;
  XGCValues gcv;
  GC gc;
  int x, y;

  unsigned long crpos=0, cgpos=0, cbpos=0, capos=0; /* bitfield positions */
  unsigned long srpos=0, sgpos=0, sbpos=0;
  unsigned long srmsk=0, sgmsk=0, sbmsk=0;
  unsigned long srsiz=0, sgsiz=0, sbsiz=0;

# ifdef HAVE_JWXYZ
  // BlackPixel has alpha: 0xFF000000.
  unsigned long black = BlackPixelOfScreen (DefaultScreenOfDisplay (dpy));
#else
  unsigned long black = 0;
# endif

  XGetWindowAttributes (dpy, window, &xgwa);

  in = make_ximage (dpy, xgwa.visual, filename, image_data, data_size);
  if (!in) return 0;

  /* Create a new image in the depth and bit-order of the server. */
  out = XCreateImage (dpy, xgwa.visual, xgwa.depth, ZPixmap, 0, 0,
                      in->width, in->height, 8, 0);

  out->bitmap_bit_order = in->bitmap_bit_order;
  out->byte_order = in->byte_order;

  out->bitmap_bit_order = BitmapBitOrder (dpy);
  out->byte_order = ImageByteOrder (dpy);

  out->data = (char *) malloc (out->height * out->bytes_per_line);
  if (!out->data) abort();

  if (mask_ret)
    {
      mask = XCreateImage (dpy, xgwa.visual, 1, XYPixmap, 0, 0,
                           in->width, in->height, 8, 0);
      mask->byte_order = in->byte_order;
      mask->data = (char *) malloc (mask->height * mask->bytes_per_line);
    }

  /* Find the server's color masks.
   */
  srmsk = out->red_mask;
  sgmsk = out->green_mask;
  sbmsk = out->blue_mask;

  if (!(srmsk && sgmsk && sbmsk)) abort();  /* No server color masks? */

  decode_mask (srmsk, &srpos, &srsiz);
  decode_mask (sgmsk, &sgpos, &sgsiz);
  decode_mask (sbmsk, &sbpos, &sbsiz);

  /* 'in' is RGBA in client endianness.  Convert to what the server wants. */
  if (bigendian())
    crpos = 24, cgpos = 16, cbpos =  8, capos =  0;
  else
    crpos =  0, cgpos =  8, cbpos = 16, capos = 24;

  for (y = 0; y < in->height; y++)
    for (x = 0; x < in->width; x++)
      {
        unsigned long p = XGetPixel (in, x, y);
        unsigned char a = (p >> capos) & 0xFF;
        unsigned char b = (p >> cbpos) & 0xFF;
        unsigned char g = (p >> cgpos) & 0xFF;
        unsigned char r = (p >> crpos) & 0xFF;
        XPutPixel (out, x, y, ((r << srpos) |
                               (g << sgpos) |
                               (b << sbpos) |
                               black));
        if (mask)
          XPutPixel (mask, x, y, (a ? 1 : 0));
      }

  XDestroyImage (in);
  in = 0;

  pixmap = XCreatePixmap (dpy, window, out->width, out->height, xgwa.depth);
  gc = XCreateGC (dpy, pixmap, 0, &gcv);
  XPutImage (dpy, pixmap, gc, out, 0, 0, 0, 0, out->width, out->height);
  XFreeGC (dpy, gc);

  if (mask)
    {
      Pixmap p2 = XCreatePixmap (dpy, window, mask->width, mask->height, 1);
      gcv.foreground = 1;
      gcv.background = 0;
      gc = XCreateGC (dpy, p2, GCForeground|GCBackground, &gcv);
      XPutImage (dpy, p2, gc, mask, 0, 0, 0, 0, mask->width, mask->height);
      XFreeGC (dpy, gc);
      XDestroyImage (mask);
      mask = 0;
      *mask_ret = p2;
    }

  if (width_ret)  *width_ret  = out->width;
  if (height_ret) *height_ret = out->height;

  XDestroyImage (out);

  return pixmap;
}


/* Textures are upside down, so invert XImages before returning them.
 */
static void
flip_ximage (XImage *ximage)
{
  char *data2, *in, *out;
  int y;

  if (!ximage) return;
  data2 = malloc (ximage->bytes_per_line * ximage->height);
  if (!data2) abort();
  in = ximage->data;
  out = data2 + ximage->bytes_per_line * (ximage->height - 1);
  for (y = 0; y < ximage->height; y++)
    {
      memcpy (out, in, ximage->bytes_per_line);
      in  += ximage->bytes_per_line;
      out -= ximage->bytes_per_line;
    }
  free (ximage->data);
  ximage->data = data2;
}


Pixmap
image_data_to_pixmap (Display *dpy, Window window, 
                      const unsigned char *image_data, unsigned long data_size,
                      int *width_ret, int *height_ret,
                      Pixmap *mask_ret)
{
  return make_pixmap (dpy, window, 0, image_data, data_size,
                      width_ret, height_ret, mask_ret);
}

Pixmap
file_to_pixmap (Display *dpy, Window window, const char *filename,
                int *width_ret, int *height_ret,
                Pixmap *mask_ret)
{
  return make_pixmap (dpy, window, filename, 0, 0,
                      width_ret, height_ret, mask_ret);
}


/* This XImage has RGBA data, which is what OpenGL code typically expects.
   Also it is upside down: the origin is at the bottom left of the image.
   X11 typically expects 0RGB as it has no notion of alpha, only 1-bit masks.
   With X11 code, you should probably use the _pixmap routines instead.
 */
XImage *
image_data_to_ximage (Display *dpy, Visual *visual,
                      const unsigned char *image_data,
                      unsigned long data_size)
{
  XImage *ximage = make_ximage (dpy, visual, 0, image_data, data_size);
  flip_ximage (ximage);
  return ximage;
}

XImage *
file_to_ximage (Display *dpy, Visual *visual, const char *filename)
{
  XImage *ximage = make_ximage (dpy, visual, filename, 0, 0);
  flip_ximage (ximage);
  return ximage;
}
