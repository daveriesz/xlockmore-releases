#ifndef lint
static char sccsid[] = "@(#)geometry.c	2.9 95/04/24 xlockmore";
#endif
/*-
 * geometry.c - low cpu screen design for xlock, the X Window System lockscreen.
 *
 * This screen design has n (2 <= n <= MAXPTS) number of points that
 * randomly move around the screen.  Each point is connected to every
 * other point by a line.  Gives the sensation of abstract 3D morphing.
 *
 * Copyright (c) 1994 by Darrick Brown.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 06-Mar-95: Cleaned up code.
 * 11-Jul-94: Written.
 */

#include "xlock.h"
#include <math.h>

#define MAXPTS 10 /* remember: the number of lines to be drawn is 1+2+3+...+PTS */
#define MINPTS 2 

typedef struct{
  float sx[MAXPTS];
  float sy[MAXPTS];
  float ex[MAXPTS];
  float ey[MAXPTS];
  float dx[MAXPTS];
  float dy[MAXPTS];
  float px[MAXPTS];
  float py[MAXPTS];
  float oldx[MAXPTS];
  float oldy[MAXPTS];
  float colors;
  int width;
  int height;
  float top,bottom,left,right;
  int num;
}movepoint;

static movepoint pts[MAXSCREENS];

static void drawlines();
static void eraselines();
static float sqr();

static int tfx(x)
   float x;
{
      
   int a;
   movepoint *pt = &pts[screen];
   
   a = (int)((x/pt->right)*(float)pt->width);
   
   return(a);
}


static int tfy(y)
  float y;
{
      
  int a;
  movepoint *pt = &pts[screen];
   
  a = pt->height - (int)((y/pt->top)*(float)pt->height);
   
  return(a);

}


static void eraselines(win)
  Window win;
{
   
  int i,j;

  movepoint *pt = &pts[screen];

  XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
  for(i=0; i<pt->num-1; i++){
    for(j=i+1; j<pt->num; j++){
      XDrawLine (dsp, win, Scr[screen].gc,tfx(pt->oldx[i]),tfy(pt->oldy[i]),
		                          tfx(pt->oldx[j]),tfy(pt->oldy[j]));
    }
  }

}


static void drawlines(win)
  Window win;
{
   
  int i,j;

  movepoint *pt = &pts[screen];

  for(i=0; i<pt->num-1; i++){
    for(j=i+1; j<pt->num; j++){
      XDrawLine (dsp, win, Scr[screen].gc,tfx(pt->px[i]),tfy(pt->py[i]),
		                          tfx(pt->px[j]),tfy(pt->py[j]));
    }
  }

}


static float sqr(v)
  float v;
{
  return(v*v);
}


/*DrawGeometry*/
void drawgeometry(win)
    Window      win;

{
    int i;
    float dist,speed;
    movepoint *pt = &pts[screen];

    for(i=0; i<pt->num; i++){
      pt->oldx[i]=pt->px[i];
      pt->oldy[i]=pt->py[i];
      pt->px[i]+=pt->dx[i];
      pt->py[i]+=pt->dy[i];
    }

    eraselines(win);

    pt->colors+=(float)(Scr[screen].npixels)/200.0;
    if(pt->colors>=Scr[screen].npixels){
      pt->colors=0.0;
    }
    if (!mono && Scr[screen].npixels > 2)
      XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[(int)pt->colors]);
    else
      XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
    drawlines(win);
 
    for(i=0; i<pt->num; i++){
      if(sqrt(sqr(pt->px[i]-pt->ex[i])+sqr(pt->py[i]-pt->ey[i]))<=50.0){
	
	pt->sx[i]=pt->px[i]; /* Set it to the current point */
        pt->sy[i]=pt->py[i]; 
        pt->ex[i]=(float)(LRAND()%(int)pt->right);
        pt->ey[i]=(float)(LRAND()%10000);
        pt->dx[i]=pt->ex[i]-pt->sx[i];
        pt->dy[i]=pt->ey[i]-pt->sy[i];
        dist=sqrt((pt->dx[i]*pt->dx[i])+(pt->dy[i]*pt->dy[i]));
        pt->dx[i]/=dist;
        pt->dy[i]/=dist;
        speed=(float)(LRAND()%15+36); /* a float 1.5-3.6 */
        pt->dx[i]*=speed;
        pt->dy[i]*=speed;
      }
    }
}


void initgeometry(win)
    Window      win;
{
    int i;
    float dist,speed;
    movepoint *pt = &pts[screen];
    XWindowAttributes xgwa;

    (void) XGetWindowAttributes(dsp, win, &xgwa);
    
    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp,win,Scr[screen].gc, 0, 0, xgwa.width,xgwa.height);
    
    /* keep the window parameters proportional */
    pt->top = 10000.0;
    pt->bottom = 0;
    pt->right = (float)(xgwa.width)/(float)(xgwa.height)*(10000.0);
    pt->left = 0;

    pt->width = xgwa.width;
    pt->height = xgwa.height;

    if(batchcount > MAXPTS)
      pt->num = LRAND()%(MAXPTS-MINPTS+1)+MINPTS;
    else
      pt->num = batchcount;
    /* Absolute minimum */
    if(pt->num < MINPTS)
      pt->num = MINPTS;

    for(i=0; i<pt->num; i++){
      pt->sx[i]=(float)(LRAND()%(int)pt->right);
      pt->sy[i]=(float)(LRAND()%10000);
      pt->px[i]=pt->sx[i];
      pt->py[i]=pt->sy[i];
      pt->ex[i]=(float)(LRAND()%(int)pt->right);
      pt->ey[i]=(float)(LRAND()%10000);
      pt->dx[i]=pt->ex[i]-pt->sx[i];
      pt->dy[i]=pt->ey[i]-pt->sy[i];
      dist=sqrt((pt->dx[i]*pt->dx[i])+(pt->dy[i]*pt->dy[i]));
      pt->dx[i]/=dist;
      pt->dy[i]/=dist;
      speed=(float)(LRAND()%15+21); /* a float 15.0-36.0 */
      pt->dx[i]*=speed;
      pt->dy[i]*=speed;
    }

    pt->colors=0.0;

}
