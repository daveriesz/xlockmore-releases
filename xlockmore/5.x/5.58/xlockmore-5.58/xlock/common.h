#ifndef __XLOCK_COMMON_H__
#define __XLOCK_COMMON_H__

#if !defined( lint ) && !defined( SABER )
/* #ident	"@(#)common.h	5.34 2011/05/23 xlockmore" */

#endif

#if HAVE_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_TWO_ARGS
#ifdef SunCplusplus
#define GETTIMEOFDAY(t) (void)gettimeofday(t,(struct timezone *) NULL);
#else
#define GETTIMEOFDAY(t) (void)gettimeofday(t,NULL);
#endif
#else
#define GETTIMEOFDAY(t) (void)gettimeofday(t);
#endif
#endif

/* There is some overlap so it can be made more efficient */
#define ERASE_IMAGE(d,w,g,x,y,xl,yl,xs,ys) \
if (yl<y) \
(y<(yl+ys))?XFillRectangle(d,w,g,xl,yl,xs,y-(yl)): \
XFillRectangle(d,w,g,xl,yl,xs,ys); \
else if (yl>y) \
(y>(yl-(ys)))?XFillRectangle(d,w,g,xl,y+ys,xs,yl-(y)): \
XFillRectangle(d,w,g,xl,yl,xs,ys); \
if (xl<x) \
(x<(xl+xs))?XFillRectangle(d,w,g,xl,yl,x-(xl),ys): \
XFillRectangle(d,w,g,xl,yl,xs,ys); \
else if (xl>x) \
(x>(xl-(xs)))?XFillRectangle(d,w,g,x+xs,yl,xl-(x),ys): \
XFillRectangle(d,w,g,xl,yl,xs,ys)


#define MI_CLEARWINDOWCOLORMAP(mi, gc, pixel) \
{ \
 XSetForeground(MI_DISPLAY(mi), gc, pixel); \
 XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), gc, \
   0, 0, (unsigned int) MI_WIDTH(mi), (unsigned int) MI_HEIGHT(mi)); \
}
#define MI_CLEARWINDOWCOLORMAPFAST(mi, gc, pixel) \
 MI_CLEARWINDOWCOLORMAP(mi, gc, pixel)
#define MI_CLEARWINDOWCOLOR(mi, pixel) \
 MI_CLEARWINDOWCOLORMAP(mi, MI_GC(mi), pixel)

/* #define MI_CLEARWINDOW(mi) XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi)) */
/*#define MI_CLEARWINDOW(mi) MI_CLEARWINDOWCOLOR(mi, MI_BLACK_PIXEL(mi))*/



#endif /* __XLOCK_COMMON_H__ */
