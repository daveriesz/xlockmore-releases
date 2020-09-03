/* -*- Mode: C; tab-width: 4 -*- */
/* pentominoes --- Shows attempts to place pentominoes into a rectangle */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)pentominoes.c	5.00 2000/11/01 xlockmore";

#endif

/*
 * Copyright (c) 2000 by Stephen Montgomery-Smith <stephen@math.missouri.edu>
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 * 27-Nov-2000: Adapted from euler2d.c Copyright (c) 2000 by Stephen
 *              Montgomery-Smith
 * 05-Jun-2000: Adapted from flow.c Copyright (c) 1996 by Tim Auckland
 * 18-Jul-1996: Adapted from swarm.c Copyright (c) 1991 by Patrick J. Naughton.
 * 31-Aug-1990: Adapted from xswarm by Jeff Butterworth. (butterwo@ncsc.org)
 */

/*
A pentomino is a shape made up of five squares placed adjacent to each other
in some configuration.  There are twelve different possible pentominoes
(up to rotations, reflections and translations).

This program attempts to find all the ways of placing all twelve pentominoes
into a rectangle whose size is 20x3, 15x4, 12x5 or 10x6.
*/

#ifdef STANDALONE
#define PROGCLASS "Pentominoes"
#define HACK_INIT init_pentominoes
#define HACK_DRAW draw_pentominoes
#define pentominoes_opts xlockmore_opts
#define DEFAULTS "*delay: 1000 \n" \
"*count: 1024 \n" \
"*cycles: 3000 \n" \
"*ncolors: 200 \n"
#define SMOOTH_COLORS
#include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */

#ifdef MODE_pentominoes
#if 0
static XrmOptionDescRec opts[] =
{
};
static argtype vars[] =
{
};
static OptionStruct desc[] =
{
};

ModeSpecOpt pentominoes_opts =
{sizeof opts / sizeof opts[0], opts,
 sizeof vars / sizeof vars[0], vars, desc};
#else
/* SunC does not like size 0 arrays */
ModeSpecOpt pentominoes_opts =
{0, NULL, 0, NULL, NULL};
#endif

#ifdef USE_MODULES
ModStruct   pentominoes_description = {
  "pentominoes", "init_pentominoes", "draw_pentominoes", "release_pentominoes",
  "refresh_pentominoes", "init_pentominoes", NULL, &pentominoes_opts,
  1000, 1, 1, 1, 64, 1.0, "",
  "Shows attempts to place pentominoes into a rectangle", 0, NULL
};

#endif

/* List of coordinates for the twelve pentominoes.
*/
static struct pent_struct {int x,y;} init_pent_list[12][5] = 
{{{0,0}, {1,0}, {2,0}, {0,1}, {1,1}}, /* []' */
 {{0,0}, {1,0}, {2,0}, {3,0}, {4,0}}, /* ---  */
 {{0,0}, {1,0}, {2,0}, {3,0}, {3,1}}, /* --, */
 {{0,0}, {1,0}, {2,0}, {2,1}, {2,2}}, /* 7 */
 {{0,0}, {1,0}, {2,0}, {1,1}, {1,2}}, /* T */
 {{1,0}, {0,1}, {1,1}, {2,1}, {1,2}}, /* + */
 {{1,0}, {0,1}, {1,1}, {2,1}, {2,2}}, /* -|, */
 {{0,0}, {0,1}, {1,1}, {2,1}, {2,2}}, /* S */
 {{0,0}, {2,0}, {0,1}, {1,1}, {2,1}}, /* U */
 {{0,0}, {1,0}, {2,0}, {3,0}, {1,1}}, /* -,-- */
 {{1,0}, {2,0}, {3,0}, {0,1}, {1,1}}, /* _-- */
 {{0,0}, {0,1}, {1,1}, {1,2}, {2,2}}}; /* M */

/* List of transformations that need to be considered
   for each pentomino.  See the function transform
   below.
*/
static struct transform_struct {int len; int transform_no[8];} init_transform_list[12] = 
{{8, {0, 1, 2, 3, 4, 5, 6, 7}},
 {2, {0, 1,-1,-1,-1,-1,-1,-1}},
 {8, {0, 1, 2, 3, 4, 5, 6, 7}},
 {4, {0, 1, 2, 3,-1,-1,-1,-1}},
 {4, {0, 1, 2, 3,-1,-1,-1,-1}},
 {1, {0,-1,-1,-1,-1,-1,-1,-1}},
 {8, {0, 1, 2, 3, 4, 5, 6, 7}},
 {4, {0, 1, 4, 5,-1,-1,-1,-1}},
 {4, {0, 1, 2, 3,-1,-1,-1,-1}},
 {8, {0, 1, 2, 3, 4, 5, 6, 7}},
 {8, {0, 1, 2, 3, 4, 5, 6, 7}},
 {4, {0, 1, 2, 3,-1,-1,-1,-1}}};

/* If the rectangle is alternatively covered by white and black
   squares (chess board style), then this gives the list of
   the maximal possible whites covered by each pentomino.  It
   is used by the function whites_ok which makes sure that the
   array has a valid number of white or black remaining blanks left.
*/
int init_max_white_list[12] = {3,3,3,3,4,4,3,3,3,3,3,3};


typedef struct {
  int wait;

  unsigned long color[12], border_color;
  int mono;

/* Copies of the above data structures (in a randomized order). */
  struct pent_struct pent_list[12][5];
  struct transform_struct transform_list[12];
  int max_white_list[12];

/* A list of what pentominoes have been attached, and where
   they have been attached.  See the functions attach and detach
   below. */
  struct {int pent_no,point_no,x,y,transform_index;} attach_list[12];
  int nr_attached;

/* A boolean to indicate if a pentomino has already been attached. */
  int attached[12];

/* The array that tells where the pentominoes are attached. */
  int width, height, array[20][6], changed_array[20][6];

/* These specify the dimensions of how things appear on the screen. */
  int box, x_margin, y_margin;

/* These booleans decide in which way to try to attach the pentominoes. */
  int left_right, top_bottom;

/* Bitmaps for display purposes. */
  int use_bitmaps;
  XImage *bitmaps[256];

/* Structures used for display purposes if there is not enough memory
   to allocate bitmaps (or if the screen is small). */
  XSegment lines[60];
  XRectangle rectangles[60];

} pentominoesstruct;

#define ARR(x,y) (((x)<0||(x)>=sp->width||(y)<0||(y)>=sp->height)?-1:sp->array[x][y])

#define ROUND8(n) ((((n)+7)/8)*8)

/* Defines to index the bitmaps.  A set bit indicates that an edge or
   corner is required. */
#define LEFT (1<<0)
#define RIGHT (1<<1)
#define UP (1<<2)
#define DOWN (1<<3)
#define LEFT_UP (1<<4)
#define LEFT_DOWN (1<<5)
#define RIGHT_UP (1<<6)
#define RIGHT_DOWN (1<<7)
#define IS_LEFT(n) ((n) & LEFT)
#define IS_RIGHT(n) ((n) & RIGHT)
#define IS_UP(n) ((n) & UP)
#define IS_DOWN(n) ((n) & DOWN)
#define IS_LEFT_UP(n) ((n) & LEFT_UP)
#define IS_LEFT_DOWN(n) ((n) & LEFT_DOWN)
#define IS_RIGHT_UP(n) ((n) & RIGHT_UP)
#define IS_RIGHT_DOWN(n) ((n) & RIGHT_DOWN)

/* Defines to access the bitmaps. */
#define BITNO(x,y) ((x)+(y)*ROUND8(sp->box))
#define SETBIT(n,x,y) {data[BITNO(x,y)/8] |= 1<<(BITNO(x,y)%8);}
#define RESBIT(n,x,y) {data[BITNO(x,y)/8] &= ~(1<<(BITNO(x,y)%8));}
#define TWOTHIRDSBIT(n,x,y) {if ((x+y-1)%3) SETBIT(n,x,y) else RESBIT(n,x,y)}
#define HALFBIT(n,x,y) {if ((x-y)%2) SETBIT(n,x,y) else RESBIT(n,x,y)}
#define THIRDBIT(n,x,y) {if (!((x-y-1)%3)) SETBIT(n,x,y) else RESBIT(n,x,y)}
#define THREEQUARTERSBIT(n,x,y) \
  {if ((y%2)||((x+2+y/2+1)%2)) SETBIT(n,x,y) else RESBIT(n,x,y)}

/* Parameters for bitmaps. */
#define T G*2  /* Thickness of walls of pentominoes. */
#define G ((sp->box/45)+1)  /* 1/2 of gap between pentominoes. */
#define R G*6  /* Amount of rounding. */
#define RT G*3 /* Thickness of wall of rounded parts. Here 3 is an
                  approximation to 2 sqrt(2). */
#define RR 0   /* Roof ridge thickness */

/* A list of those bitmaps we need to create to display any pentomino. */
static int bitmaps_needed[] =
{
 LEFT_UP|LEFT_DOWN|RIGHT_UP|RIGHT_DOWN,

 LEFT|RIGHT_UP|RIGHT_DOWN,
 RIGHT|LEFT_UP|LEFT_DOWN,
 UP|LEFT_DOWN|RIGHT_DOWN,
 DOWN|LEFT_UP|RIGHT_UP,
 LEFT|RIGHT_UP,
 RIGHT|LEFT_UP,
 UP|LEFT_DOWN,
 DOWN|LEFT_UP,
 LEFT|RIGHT_DOWN,
 RIGHT|LEFT_DOWN,
 UP|RIGHT_DOWN,
 DOWN|RIGHT_UP,

 LEFT|UP|RIGHT_DOWN,
 LEFT|DOWN|RIGHT_UP,
 RIGHT|UP|LEFT_DOWN,
 RIGHT|DOWN|LEFT_UP,
 LEFT|UP,
 LEFT|DOWN,
 RIGHT|UP,
 RIGHT|DOWN,

 LEFT|RIGHT,
 UP|DOWN,

 RIGHT|UP|DOWN,
 LEFT|UP|DOWN,
 LEFT|RIGHT|DOWN,
 LEFT|RIGHT|UP,

 -1
};

static pentominoesstruct *pentominoeses = NULL;

static void transform(int x, int y, int transform_no, int *out_x, int *out_y) {
  switch (transform_no) {
    case 0: *out_x=x;
            *out_y=y;
            break;
    case 1: *out_x=-y;
            *out_y=x;
            break;
    case 2: *out_x=-x;
            *out_y=-y;
            break;
    case 3: *out_x=y;
            *out_y=-x;
            break;
    case 4: *out_x=-x;
            *out_y=y;
            break;
    case 5: *out_x=y;
            *out_y=x;
            break;
    case 6: *out_x=x;
            *out_y=-y;
            break;
    case 7: *out_x=-y;
            *out_y=-x;
            break;
  }
}

static
void random_permutation(int n, int a[]) {
  int i,j;

  for (i=0;i<n;i++) {
    do {
      a[i] = NRAND(n);
      for (j=0;j<i && a[j]!=a[i];j++);
    } while (j<i);
  }
}

static
void randomize(pentominoesstruct *sp) {
  int perm[12], temp_transform_no[8], i, j;

  random_permutation(12,perm);
  for (i=0;i<12;i++) {
    memcpy(sp->pent_list[i],init_pent_list[perm[i]],sizeof(struct pent_struct)*5);
    memcpy(&sp->transform_list[i],&init_transform_list[perm[i]],sizeof(struct transform_struct));
    sp->max_white_list[i] = init_max_white_list[perm[i]];
  }

  for (i=0;i<12;i++) {
    memcpy(temp_transform_no,sp->transform_list[i].transform_no,sizeof(temp_transform_no));
    random_permutation(sp->transform_list[i].len,perm);
    for (j=0;j<sp->transform_list[i].len;j++)
      sp->transform_list[i].transform_no[j] = temp_transform_no[perm[j]];
  }
}

/* find_non_multiple_of_five_regions looks for connected regions of 
   blank spaces, and returns 1 if it finds a connected region containing
   a number of blanks that is not a multiple of five.
*/

static void count_adjacent_blanks(pentominoesstruct *sp, int *count, int x, int y) {

  if (sp->array[x][y] == -1) {
    (*count)++;
    sp->array[x][y] = -2;
    if (x>=1) count_adjacent_blanks(sp, count,x-1,y);
    if (x<sp->width-1) count_adjacent_blanks(sp, count,x+1,y);
    if (y>=1) count_adjacent_blanks(sp, count,x,y-1);
    if (y<sp->height-1) count_adjacent_blanks(sp, count,x,y+1);
  }
}

static int find_non_multiple_of_five_regions(pentominoesstruct *sp) {
  int x,y,count,found = 0;

  for (x=0;x<sp->width && !found;x++) for (y=0;y<sp->height && !found;y++) {
    count = 0;
    count_adjacent_blanks(sp, &count,x,y);
    found = count%5 != 0;
  }

  for (x=0;x<sp->width;x++) for (y=0;y<sp->height;y++)
    if (sp->array[x][y] == -2)
      sp->array[x][y] = -1;

  return found;
}

/* "Chess board" check - see init_max_whites_list above for more details.
*/

static int whites_ok(pentominoesstruct *sp) {
  int x,y,i,whites=0,blacks,max_white=0,min_white;

  for (x=0;x<sp->width;x++) for (y=0;y<sp->height;y++)
    if (sp->array[x][y] == -1 && (x+y)%2) whites++;
  blacks = (12-sp->nr_attached)*5-whites;
  for (i=0;i<12;i++) if (!sp->attached[i])
    max_white += sp->max_white_list[i];
  min_white = (12-sp->nr_attached)*5-max_white;
  return (min_white <= blacks && min_white <= whites
          && blacks <= max_white && whites <= max_white);
}

static void find_blank(pentominoesstruct *sp, int *x, int *y) {

  if (sp->left_right && sp->top_bottom) {
    *x=0;
    *y=0;
    while (sp->array[*x][*y] != -1) {
      (*y)++;
      if (*y==sp->height) {(*x)++;*y=0;}
    }
  }
  else if (!sp->left_right && sp->top_bottom) {
    *x=sp->width-1;
    *y=0;
    while (sp->array[*x][*y] != -1) {
      (*y)++;
      if (*y==sp->height) {(*x)--;*y=0;}
    }
  }
  else if (sp->left_right && !sp->top_bottom) {
    *x=0;
    *y=sp->height-1;
    while (sp->array[*x][*y] != -1) {
      (*y)--;
      if (*y==-1) {(*x)++;*y=sp->height-1;}
    }
  }
  else if (!sp->left_right && !sp->top_bottom) {
    *x=sp->width-1;
    *y=sp->height-1;
    while (sp->array[*x][*y] != -1) {
      (*y)--;
      if (*y==-1) {(*x)--;*y=sp->height-1;}
    }
  }
}

/* Attempts to attach a pentomino at point (x,y) at the 
   point_no-th point of that pentomino, using the transform
   transform_no.  Returns 1 if successful.
*/

static
int attach(pentominoesstruct *sp, int pent_no, int point_no, int transform_index) {
  int x, y, x_offset, y_offset, add_x, add_y, i;

  if (find_non_multiple_of_five_regions(sp))
    return 0;
  if (sp->attached[pent_no])
    return 0;
  if (!whites_ok(sp))
    return 0;

  find_blank(sp,&x,&y);
  x_offset = sp->pent_list[pent_no][point_no].x;
  y_offset = sp->pent_list[pent_no][point_no].y;

  for (i=0;i<5;i++) {
    transform(sp->pent_list[pent_no][i].x-x_offset, sp->pent_list[pent_no][i].y-y_offset,
              sp->transform_list[pent_no].transform_no[transform_index], &add_x, &add_y);
    if ( ! ((x+add_x>=0) && (x+add_x<sp->width) && (y+add_y>=0) && (y+add_y<sp->height)
              && (sp->array[x+add_x][y+add_y] == -1)))
      return 0;
  }

  for (i=0;i<5;i++) {
    transform(sp->pent_list[pent_no][i].x-x_offset, sp->pent_list[pent_no][i].y-y_offset,
              sp->transform_list[pent_no].transform_no[transform_index], &add_x, &add_y);
    sp->array[x+add_x][y+add_y] = pent_no;
    sp->changed_array[x+add_x][y+add_y] = 1;
  }

  sp->attached[pent_no] = 1;
  sp->attach_list[sp->nr_attached].pent_no = pent_no;
  sp->attach_list[sp->nr_attached].point_no = point_no;
  sp->attach_list[sp->nr_attached].x = x;
  sp->attach_list[sp->nr_attached].y = y;
  sp->attach_list[sp->nr_attached].transform_index = transform_index;
  sp->nr_attached++;

  return 1;
}

/* Detaches the most recently attached pentomino. */

static
void detach(pentominoesstruct *sp, int *pent_no, int *point_no, int *transform_index) {
  int x_offset, y_offset, add_x, add_y, i, x, y;

  if (sp->nr_attached == 0) return;
  sp->nr_attached--;
  *pent_no = sp->attach_list[sp->nr_attached].pent_no;
  *point_no = sp->attach_list[sp->nr_attached].point_no;
  *transform_index = sp->attach_list[sp->nr_attached].transform_index;
  x = sp->attach_list[sp->nr_attached].x;
  y = sp->attach_list[sp->nr_attached].y;
  x_offset = sp->pent_list[*pent_no][*point_no].x;
  y_offset = sp->pent_list[*pent_no][*point_no].y;
  for (i=0;i<5;i++) {
    transform(sp->pent_list[*pent_no][i].x-x_offset,
              sp->pent_list[*pent_no][i].y-y_offset,
              sp->transform_list[*pent_no].transform_no[*transform_index],
              &add_x, &add_y);
    sp->array[x+add_x][y+add_y] = -1;
    sp->changed_array[x+add_x][y+add_y] = 1;
  }

  sp->attached[*pent_no] = 0;
}

static
int next_attach_try(pentominoesstruct *sp, int *pent_no, int *point_no, int *transform_index) {

  (*transform_index)++;
  if (*transform_index>=sp->transform_list[*pent_no].len) {
    *transform_index = 0;
    (*point_no)++;
    if (*point_no>=5) {
      *point_no = 0;
      (*pent_no)++;
      if (*pent_no>=12) {
        *pent_no = 0;
        return 0;
      }
    }
  }
  return 1;
}

void draw_without_bitmaps(ModeInfo * mi, pentominoesstruct *sp) {
  Display *display = MI_DISPLAY(mi);
  Window  window = MI_WINDOW(mi);
  GC gc = MI_GC(mi);
  int x,y,pent_no,nr_lines,nr_rectangles;

  XSetLineAttributes(display,gc,sp->box/10+1,LineSolid,CapRound,JoinRound);

  for (pent_no=-1;pent_no<12;pent_no++) {
    nr_rectangles = 0;
    for (x=0;x<sp->width;x++) for (y=0;y<sp->height;y++)
    if (sp->changed_array[x][y] && sp->array[x][y] == pent_no) {
      sp->rectangles[nr_rectangles].x = sp->x_margin + sp->box*x;
      sp->rectangles[nr_rectangles].y = sp->y_margin + sp->box*y;
      sp->rectangles[nr_rectangles].width = sp->box;
      sp->rectangles[nr_rectangles].height = sp->box;
      nr_rectangles++;
    }
    if (pent_no == -1)
      XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
    else
      XSetForeground(display, gc, sp->color[pent_no]);
    XFillRectangles(display, window, gc, sp->rectangles, nr_rectangles);
  }

  XSetForeground(display, gc, MI_BLACK_PIXEL(mi));

  nr_lines = 0;
  for (x=0;x<sp->width-1;x++) for (y=0;y<sp->height;y++) {
    if (sp->array[x][y] == -1 && sp->array[x+1][y] == -1
        && (sp->changed_array[x][y] || sp->changed_array[x+1][y])) {
      sp->lines[nr_lines].x1 = sp->x_margin + sp->box*(x+1);
      sp->lines[nr_lines].y1 = sp->y_margin + sp->box*y;
      sp->lines[nr_lines].x2 = sp->x_margin + sp->box*(x+1);
      sp->lines[nr_lines].y2 = sp->y_margin + sp->box*(y+1);
      nr_lines++;
    }
  }
  XDrawSegments(display, window, gc, sp->lines, nr_lines);

  nr_lines = 0;
  for (x=0;x<sp->width;x++) for (y=0;y<sp->height-1;y++) {
    if (sp->array[x][y] == -1 && sp->array[x][y+1] == -1
        && (sp->changed_array[x][y] || sp->changed_array[x][y+1])) {
      sp->lines[nr_lines].x1 = sp->x_margin + sp->box*x;
      sp->lines[nr_lines].y1 = sp->y_margin + sp->box*(y+1);
      sp->lines[nr_lines].x2 = sp->x_margin + sp->box*(x+1);
      sp->lines[nr_lines].y2 = sp->y_margin + sp->box*(y+1);
      nr_lines++;
    }
  }
  XDrawSegments(display, window, gc, sp->lines, nr_lines);

  XSetForeground(display, gc, MI_WHITE_PIXEL(mi));
  XDrawRectangle(display, window, gc, sp->x_margin, sp->y_margin, sp->box*sp->width, sp->box*sp->height);

  XSetForeground(display, gc, MI_WHITE_PIXEL(mi));

  nr_lines = 0;
  for (x=0;x<sp->width-1;x++) for (y=0;y<sp->height;y++) {
    if (sp->array[x+1][y] != sp->array[x][y]) {
      sp->lines[nr_lines].x1 = sp->x_margin + sp->box*(x+1);
      sp->lines[nr_lines].y1 = sp->y_margin + sp->box*y;
      sp->lines[nr_lines].x2 = sp->x_margin + sp->box*(x+1);
      sp->lines[nr_lines].y2 = sp->y_margin + sp->box*(y+1);
      nr_lines++;
    }
  }
  XDrawSegments(display, window, gc, sp->lines, nr_lines);

  nr_lines = 0;
  for (x=0;x<sp->width;x++) for (y=0;y<sp->height-1;y++) {
    if (sp->array[x][y+1] != sp->array[x][y]) {
      sp->lines[nr_lines].x1 = sp->x_margin + sp->box*x;
      sp->lines[nr_lines].y1 = sp->y_margin + sp->box*(y+1);
      sp->lines[nr_lines].x2 = sp->x_margin + sp->box*(x+1);
      sp->lines[nr_lines].y2 = sp->y_margin + sp->box*(y+1);
      nr_lines++;
    }
  }
  XDrawSegments(display, window, gc, sp->lines, nr_lines);
}

static int bitmap_needed(int n) {
  int i;

  for (i=0;bitmaps_needed[i]!=-1;i++)
    if (n == bitmaps_needed[i])
      return 1;
  return 0;
}

void create_bitmaps(ModeInfo * mi, pentominoesstruct *sp) {
  int x,y,n;
  char *data;

  for (n=0;n<256;n++) {

/* Avoid duplication of identical bitmaps. */
    if (IS_LEFT_UP(n) && (IS_LEFT(n) || IS_UP(n)))
      sp->bitmaps[n] = sp->bitmaps[n & ~LEFT_UP];
    else if (IS_LEFT_DOWN(n) && (IS_LEFT(n) || IS_DOWN(n)))
      sp->bitmaps[n] = sp->bitmaps[n & ~LEFT_DOWN];
    else if (IS_RIGHT_UP(n) && (IS_RIGHT(n) || IS_UP(n)))
      sp->bitmaps[n] = sp->bitmaps[n & ~RIGHT_UP];
    else if (IS_RIGHT_DOWN(n) && (IS_RIGHT(n) || IS_DOWN(n)))
      sp->bitmaps[n] = sp->bitmaps[n & ~RIGHT_DOWN];

    else if (bitmap_needed(n)) {
      data = malloc(sizeof(char)*(sp->box*ROUND8(sp->box)/8));
      if (data == NULL) {
        sp->use_bitmaps = 0;
        return;
      }

      for (y=0;y<sp->box;y++) for (x=0;x<sp->box;x++) {
        if ((x>=y && x<=sp->box-y-1 && IS_UP(n))
            || (x<=y && x<=sp->box-y-1 && y<sp->box/2 && !IS_LEFT(n))
            || (x>=y && x>=sp->box-y-1 && y<sp->box/2 && !IS_RIGHT(n)))
          SETBIT(n,x,y)
        else if ((x<=y && x<=sp->box-y-1 && IS_LEFT(n))
            || (x>=y && x<=sp->box-y-1 && x<sp->box/2 && !IS_UP(n))
            || (x<=y && x>=sp->box-y-1 && x<sp->box/2 && !IS_DOWN(n)))
          TWOTHIRDSBIT(n,x,y)
        else if ((x>=y && x>=sp->box-y-1 && IS_RIGHT(n))
            || (x>=y && x<=sp->box-y-1 && x>=sp->box/2 && !IS_UP(n))
            || (x<=y && x>=sp->box-y-1 && x>=sp->box/2 && !IS_DOWN(n)))
          HALFBIT(n,x,y)
        else if ((x<=y && x>=sp->box-y-1 && IS_DOWN(n))
            || (x<=y && x<=sp->box-y-1 && y>=sp->box/2 && !IS_LEFT(n))
            || (x>=y && x>=sp->box-y-1 && y>=sp->box/2 && !IS_RIGHT(n)))
          THIRDBIT(n,x,y)
      }

      if (IS_LEFT(n)) 
        for (y=0;y<sp->box;y++) for (x=G;x<G+T;x++)
          SETBIT(n,x,y)
      if (IS_RIGHT(n)) 
        for (y=0;y<sp->box;y++) for (x=G;x<G+T;x++)
          SETBIT(n,sp->box-1-x,y)
      if (IS_UP(n))
        for (x=0;x<sp->box;x++) for (y=G;y<G+T;y++)
          SETBIT(n,x,y)
      if (IS_DOWN(n))
        for (x=0;x<sp->box;x++) for (y=G;y<G+T;y++)
          SETBIT(n,x,sp->box-1-y)
      if (IS_LEFT(n)) 
        for (y=0;y<sp->box;y++) for (x=0;x<G;x++)
          RESBIT(n,x,y)
      if (IS_RIGHT(n)) 
        for (y=0;y<sp->box;y++) for (x=0;x<G;x++)
          RESBIT(n,sp->box-1-x,y)
      if (IS_UP(n))
        for (x=0;x<sp->box;x++) for (y=0;y<G;y++)
          RESBIT(n,x,y)
      if (IS_DOWN(n))
        for (x=0;x<sp->box;x++) for (y=0;y<G;y++)
          RESBIT(n,x,sp->box-1-y)

      if (IS_LEFT(n) && IS_UP(n))
        for (x=G;x<=G+R;x++)
          for (y=G;y<=R+2*G-x;y++) {
            if (x+y>R+2*G-RT)
              SETBIT(n,x,y)
            else
              RESBIT(n,x,y)
          }
      if (IS_LEFT(n) && IS_DOWN(n))
        for (x=G;x<=G+R;x++)
          for (y=G;y<=R+2*G-x;y++) {
            if (x+y>R+2*G-RT)
              SETBIT(n,x,sp->box-1-y)
            else
              RESBIT(n,x,sp->box-1-y)
          }
      if (IS_RIGHT(n) && IS_UP(n))
        for (x=G;x<=G+R;x++)
          for (y=G;y<=R+2*G-x;y++) {
            if (x+y>R+2*G-RT)
              SETBIT(n,sp->box-1-x,y)
            else
              RESBIT(n,sp->box-1-x,y)
          }
      if (IS_RIGHT(n) && IS_DOWN(n))
        for (x=G;x<=G+R;x++)
          for (y=G;y<=R+2*G-x;y++) {
            if (x+y>R+2*G-RT)
              SETBIT(n,sp->box-1-x,sp->box-1-y)
            else
              RESBIT(n,sp->box-1-x,sp->box-1-y)
          }

      if (!IS_LEFT(n) && !IS_UP(n) && IS_LEFT_UP(n)) {
        for (x=0;x<G;x++) for(y=0;y<G;y++)
          RESBIT(n,x,y)
        for (x=G;x<G+T;x++) for(y=0;y<G;y++)
          SETBIT(n,x,y)
        for (x=0;x<G+T;x++) for(y=G;y<G+T;y++)
          SETBIT(n,x,y)
      }
      if (!IS_LEFT(n) && !IS_DOWN(n) && IS_LEFT_DOWN(n)) {
        for (x=0;x<G;x++) for(y=0;y<G;y++)
          RESBIT(n,x,sp->box-1-y)
        for (x=G;x<G+T;x++) for(y=0;y<G;y++)
          SETBIT(n,x,sp->box-1-y)
        for (x=0;x<G+T;x++) for(y=G;y<G+T;y++)
          SETBIT(n,x,sp->box-1-y)
      }
      if (!IS_RIGHT(n) && !IS_UP(n) && IS_RIGHT_UP(n)) {
        for (x=0;x<G;x++) for(y=0;y<G;y++)
          RESBIT(n,sp->box-1-x,y)
        for (x=G;x<G+T;x++) for(y=0;y<G;y++)
          SETBIT(n,sp->box-1-x,y)
        for (x=0;x<G+T;x++) for(y=G;y<G+T;y++)
          SETBIT(n,sp->box-1-x,y)
      }
      if (!IS_RIGHT(n) && !IS_DOWN(n) && IS_RIGHT_DOWN(n)) {
        for (x=0;x<G;x++) for(y=0;y<G;y++)
          RESBIT(n,sp->box-1-x,sp->box-1-y)
        for (x=G;x<G+T;x++) for(y=0;y<G;y++)
          SETBIT(n,sp->box-1-x,sp->box-1-y)
        for (x=0;x<G+T;x++) for(y=G;y<G+T;y++)
          SETBIT(n,sp->box-1-x,sp->box-1-y)
      }

      if (!IS_LEFT(n) && !IS_UP(n) && !IS_LEFT_UP(n))
        for (x=0;x<sp->box/2-RR;x++) for(y=0;y<sp->box/2-RR;y++)
          THREEQUARTERSBIT(n,x,y)
      if (!IS_LEFT(n) && !IS_DOWN(n) && !IS_LEFT_DOWN(n))
        for (x=0;x<sp->box/2-RR;x++) for(y=sp->box/2+RR;y<sp->box;y++)
          THREEQUARTERSBIT(n,x,y)
      if (!IS_RIGHT(n) && !IS_UP(n) && !IS_RIGHT_UP(n))
        for (x=sp->box/2+RR;x<sp->box;x++) for(y=0;y<sp->box/2-RR;y++)
          THREEQUARTERSBIT(n,x,y)
      if (!IS_RIGHT(n) && !IS_DOWN(n) && !IS_RIGHT_DOWN(n))
        for (x=sp->box/2+RR;x<sp->box;x++) for(y=sp->box/2+RR;y<sp->box;y++)
          THREEQUARTERSBIT(n,x,y)

      sp->bitmaps[n] = XCreateImage(MI_DISPLAY(mi), MI_VISUAL(mi), 1, XYBitmap,
                                    0, data, sp->box, sp->box, 8, 0);
      if (sp->bitmaps[n] == None) {
        free(data);
        sp->use_bitmaps = 0;
        return;
      }
      sp->bitmaps[n]->byte_order = MSBFirst;
      sp->bitmaps[n]->bitmap_unit = 8;
      sp->bitmaps[n]->bitmap_bit_order = LSBFirst;
    }
  }

  sp->use_bitmaps = 1;
}

void draw_with_bitmaps(ModeInfo * mi, pentominoesstruct *sp) {
  Display *display = MI_DISPLAY(mi);
  Window  window = MI_WINDOW(mi);
  GC gc = MI_GC(mi);
  int x,y,t,bitmap_index;

  for (x=0;x<sp->width;x++) for (y=0;y<sp->height;y++) {
    if (sp->array[x][y] == -1) {
      XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
      XFillRectangle(display,window,gc,
                     sp->x_margin + sp->box*x,
                     sp->y_margin + sp->box*y,
                     sp->box,sp->box);
    }
    else {
      XSetForeground(display, gc, sp->color[sp->array[x][y]]);
      bitmap_index = 0;
      if (ARR(x,y) != ARR(x-1,y))   bitmap_index |= LEFT;
      if (ARR(x,y) != ARR(x+1,y))   bitmap_index |= RIGHT;
      if (ARR(x,y) != ARR(x,y-1))   bitmap_index |= UP;
      if (ARR(x,y) != ARR(x,y+1))   bitmap_index |= DOWN;
      if (ARR(x,y) != ARR(x-1,y-1)) bitmap_index |= LEFT_UP;
      if (ARR(x,y) != ARR(x-1,y+1)) bitmap_index |= LEFT_DOWN;
      if (ARR(x,y) != ARR(x+1,y-1)) bitmap_index |= RIGHT_UP;
      if (ARR(x,y) != ARR(x+1,y+1)) bitmap_index |= RIGHT_DOWN;
      XPutImage(display,window,gc,
                sp->bitmaps[bitmap_index],
                0,0,
                sp->x_margin + sp->box*x,
                sp->y_margin + sp->box*y,
                sp->box,sp->box);
    }
  }

  XSetForeground(display, gc, sp->border_color);
  for (t=G;t<G+T;t++)
    XDrawRectangle(display,window,gc,
                   sp->x_margin-t-1,sp->y_margin-t-1,
                   sp->box*sp->width+1+2*t, sp->box*sp->height+1+2*t);
}

static void free_pentominoes(pentominoesstruct *sp) {
  int n;
  
  for (n=0;n<256;n++)
/* Don't bother to free duplicates */
    if (IS_LEFT_UP(n) && (IS_LEFT(n) || IS_UP(n)))
      sp->bitmaps[n] = None;
    else if (IS_LEFT_DOWN(n) && (IS_LEFT(n) || IS_DOWN(n)))
      sp->bitmaps[n] = None;
    else if (IS_RIGHT_UP(n) && (IS_RIGHT(n) || IS_UP(n)))
      sp->bitmaps[n] = None;
    else if (IS_RIGHT_DOWN(n) && (IS_RIGHT(n) || IS_DOWN(n)))
      sp->bitmaps[n] = None;

    else if (sp->bitmaps[n] != None) {
        XDestroyImage(sp->bitmaps[n]);
        sp->bitmaps[n] = None;
    }
}

void
init_pentominoes(ModeInfo * mi) {
  pentominoesstruct *sp;
  int i,x,y, start;
  int box1, box2;

  if (pentominoeses == NULL) {
    if ((pentominoeses
         = (pentominoesstruct *) calloc(MI_NUM_SCREENS(mi),sizeof (pentominoesstruct))) 
        == NULL)
      return;
  }
  sp = &pentominoeses[MI_SCREEN(mi)];

  switch (NRAND(4)) {
    case 0:
      sp->width = 20;
      sp->height = 3;
      break;
    case 1:
      sp->width = 15;
      sp->height = 4;
      break;
    case 2:
      sp->width = 12;
      sp->height = 5;
      break;
    case 3:
      sp->width = 10;
      sp->height = 6;
      break;
  }

  box1 = MI_WIDTH(mi)/(sp->width+2);
  box2 = MI_HEIGHT(mi)/(sp->height+2);
  if (box1<box2)
    sp->box = box1;
  else
    sp->box = box2;

  randomize(sp);
  for (x=0;x<sp->width;x++) for (y=0;y<sp->height;y++) sp->array[x][y] = -1;
  for (i=0;i<12;i++) sp->attached[i] = 0;
  sp->nr_attached = 0;

  sp->left_right = NRAND(2);
  sp->top_bottom = NRAND(2);

  free_pentominoes(sp);
  if (sp->box >= 24) {
    sp->box = (sp->box/12)*12;
    create_bitmaps(mi,sp);
    if (!sp->use_bitmaps)
      free_pentominoes(sp);
   }
   else
     sp->use_bitmaps = 0;

  sp->mono = MI_NPIXELS(mi) < 12;
  start = NRAND(MI_NPIXELS(mi));
  for (i=0;i<12;i++)
    if (!sp->mono)
      sp->color[i] = MI_PIXEL(mi,(i*MI_NPIXELS(mi) / 12 + start) % MI_NPIXELS(mi));
    else
      if(sp->use_bitmaps)
        sp->color[i] = MI_WHITE_PIXEL(mi);
      else
        sp->color[i] = MI_BLACK_PIXEL(mi);

  if (sp->use_bitmaps) {
    if (sp->mono)
      sp->border_color = MI_WHITE_PIXEL(mi);
    else
      sp->border_color = MI_PIXEL(mi,NRAND(MI_NPIXELS(mi)));
  }

  sp->x_margin = (MI_WIDTH(mi)-sp->box*sp->width)/2;
  sp->y_margin = (MI_HEIGHT(mi)-sp->box*sp->height)/2;

  sp->wait = 0;

  /* Clear the background. */
  MI_CLEARWINDOW(mi);
  
}

void
draw_pentominoes(ModeInfo * mi) {
  pentominoesstruct *sp;
  int pent_no,point_no,transform_index,done,another_attachment_try;

  MI_IS_DRAWN(mi) = True;

  if (pentominoeses == NULL)
    return;
  sp = &pentominoeses[MI_SCREEN(mi)];

  sp->wait--;
  if (sp->wait>0) return;

  memset(sp->changed_array,0,sizeof(sp->changed_array));

  pent_no = 0;
  point_no = 0;
  transform_index = 0;
  done = 0;
  another_attachment_try = 1;
  while(!done) {
    if (sp->nr_attached < 12) {
      while (!done && another_attachment_try) {
        done = attach(sp,pent_no,point_no,transform_index);
        if (!done)
          another_attachment_try = next_attach_try(sp,&pent_no,&point_no,&transform_index);
      }
    }

    if (!done) {
      if (sp->nr_attached == 0)
        done = 1;
      else {
        detach(sp,&pent_no,&point_no,&transform_index);
        another_attachment_try = next_attach_try(sp,&pent_no,&point_no,&transform_index);
      }
    }
  }

  if (sp->use_bitmaps)
    draw_with_bitmaps(mi,sp);
  else
    draw_without_bitmaps(mi,sp);

  if (sp->nr_attached == 12)
    sp->wait = 600;
  else
    sp->wait = 6;

}

void
release_pentominoes(ModeInfo * mi) {
  int screen;
  
  if (pentominoeses != NULL) {
    for (screen=0;screen<MI_NUM_SCREENS(mi); screen++)
      free_pentominoes(&pentominoeses[screen]);
    (void) free((void *) pentominoeses);
    pentominoeses = NULL;
  }
}

void
refresh_pentominoes(ModeInfo * mi) {
  MI_CLEARWINDOW(mi);
}

#endif /* MODE_pentominoes */
