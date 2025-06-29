/* -*- Mode: C; tab-width: 4 -*- */
/* ant3d --- Extension to Langton's generalized turing machine ants */

#if 0
static const char sccsid[] = "@(#)ant3d.c	5.13 2004/07/23 xlockmore";

#endif
/*-
 * Copyright (c) 1999 by David Bagley.
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
 *   28-Jun-2004: A few more features added, need better "eyes" on leading
 *                cube (instead of a small black face)
 *   08-Jun-2004: Resurrected code after seeing Heiko Hamann's "Definition
 *                Behavior of Langton's Ant in Three Dimensions", Complex
 *                Systems Volume 14, Issue 3, (2003) pp 264-268
 *                heiko.hamann AT gmx.de
 *   12-Aug-1999: Coded from life3d.c and ant.c probably will not work but...
 */

#ifdef STANDALONE
# define MODE_ant3d
# define DEFAULTS	"*delay: 5000 \n" \
			"*count: -3 \n" \
			"*cycles: 10000 \n" \
			"*ncolors: 200 \n" \
			"*wireframe: False \n" \
			"*fullrandom: False \n" \
			"*verbose: False \n"\

# define reshape_ant3d 0
# define ant3d_handle_event 0
# include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
# include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */
#define DO_STIPPLE
#include "automata.h"

#ifdef MODE_ant3d

#define DEF_LABEL "True"
#define FONT_HEIGHT 19
#define FONT_WIDTH 15

/*-
 * neighbors of 0 randomizes it for 6, 12
 */
#define DEF_NEIGHBORS  "0"      /* choose random value */
#define DEF_RULE "A" /* All rules */
#define DEF_EYES  "False"

static Bool label;
static int  neighbors;
static char *rule;
static Bool eyes;

static XrmOptionDescRec opts[] =
{
	{(char *) "-label", (char *) ".ant3d.label", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+label", (char *) ".ant3d.label", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-neighbors", (char *) ".ant3d.neighbors", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-rule", (char *) ".ant3d.rule", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-eyes", (char *) ".ant3d.eyes", XrmoptionNoArg, (caddr_t) "on"},
        {(char *) "+eyes", (char *) ".ant3d.eyes", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(void *) & label, (char *) "label", (char *) "Label", (char *) DEF_LABEL, t_Bool},
	{(void *) & neighbors, (char *) "neighbors", (char *) "Neighbors", (char *) DEF_NEIGHBORS, t_Int},
	{(void *) & rule, (char *) "rule", (char *) "Rule", (char *) DEF_RULE, t_String},
	{(void *) & eyes, (char *) "eyes", (char *) "Eyes", (char *) DEF_EYES, t_Bool}
};
static OptionStruct desc[] =
{
	{(char *) "-/+label", (char *) "turn on/off rule labeling"},
	{(char *) "-neighbors num", (char *) "cubes 6 or rhombic dodecahedron 12"},
	{(char *) "-rule string", (char *) "base 4 string for Turk's Ant"},
	{(char *) "-/+eyes", (char *) "turn on/off eyes"}
};

ENTRYPOINT ModeSpecOpt ant3d_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
ModStruct   ant3d_description =
{"ant3d", "init_ant3d", "draw_ant3d", "release_ant3d",
 "refresh_ant3d", "init_ant3d", "free_ant3d", &ant3d_opts,
 5000, -3, 10000, 1, 64, 1.0, "",
 "Shows 3D ants", 0, NULL};

#endif

#define ANT3DBITS(n,w,h)\
  if ((ap->pixmaps[ap->init_bits]=\
  XCreatePixmapFromBitmapData(display,window,(char *)n,w,h,1,0,1))==None){\
  free_ant3d_screen(display,ap); return;} else {ap->init_bits++;}

/* If you change the table you may have to change the following 2 constants */
#define STATES 2
#define MINANTS 1
#define REDRAWSTEP 2000		/* How much tape to draw per cycle */

/* Don't change these numbers without changing the offset() macro below! */
#define MAXSTACKS 64
#define	MAXROWS 128
#define MAXCOLUMNS 128
#define BASESIZE ((MAXCOLUMNS*MAXROWS*MAXSTACKS)>>6)

#define RT_ANGLE 90
#define HALFRT_ANGLE 45

/* Store state of cell in top bit. Reserve low bits for count of living nbrs */
#define Set3D(x,y,z,c) SetMem(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)
#define Reset3D(x,y,z) SetMem(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,0)

#define SetList3D(x,y,z,c) if (!SetMem(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)) return False; \
if (!AddToList(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)) return False
#define InitSetList3D(x,y,z,c) if (!SetMem(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)) return; \
if (!AddToList(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)) return

#define ChangeList3D(i,x,y,z,c) if (!SetMem(ap,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)) return False; \
ChangeInList(ap,i,(unsigned int)x,(unsigned int)y,(unsigned int)z,c)

#define EyeToScreen 72.0	/* distance from eye to screen */
#define HalfScreenD 20.0	/* 1/2 the diameter of screen */
#define BUCKETSIZE 10
#define NBUCKETS ((MAXCOLUMNS+MAXROWS+MAXSTACKS)*BUCKETSIZE)
#define Distance(x1,y1,z1,x2,y2,z2) sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)+(z1-z2)*(z1-z2))

#define IP (M_PI / 180.0)
#define NCOLORS 14

typedef struct {
	unsigned char color;
	short   turn;
	unsigned char next;
} statestruct;

typedef struct _antstruct {
	int         col, row, stack;
	unsigned char color;
	char        visible;
	short       direction; /* Direction facing */
	short       gravity; /* Need to know which way is down */
	unsigned char state;
	short       dist; /* dist from cell to eye */
	struct _antstruct *next; /* pointer to next entry on linked list */
	struct _antstruct *prev;
	struct _antstruct *priority;
} antstruct;

typedef struct {
	Bool        painted;
	int         neighbors;
	int         ox, oy, oz;	/* origin */
	double      vx, vy, vz;	/* viewpoint */
	int         generation;
	int         ncols, nrows, nstacks;
	int         memstart;
	unsigned char *base[BASESIZE];
	double      A, B, C, F;
	int         width, height;
	unsigned char ncolors, nstates;
	int         n;
	double      azm;
	double      metaAlt, metaAzm, metaDist;
	antstruct  *ants;
	antstruct  **antList;
	antstruct  *ptrhead, *ptrend, eraserhead, eraserend;
	antstruct  *buckethead[NBUCKETS], *bucketend[NBUCKETS];
	Bool        wireframe, eyes;
	Pixmap      dbuf;
	statestruct machine[NCOLORS * STATES];
	unsigned char *tape;
	int         init_bits;
	unsigned long colors[NCOLORS - 1];
	GC          stippledGC;
	Pixmap      pixmaps[NCOLORS - 1];
	int         nInvisible;
	int         labelOffsetX, labelOffsetY;
	char        ruleString[50];
} antfarm3dstruct;

static char plots[] =
{6, 12};
#define NEIGHBORKINDS ((long) (sizeof plots / sizeof *plots))

/* Coordinate ant moves */
#define NU -1 /* Not used */
#define XN 0
#define XP 1
#define YN 2
#define YP 3
#define ZN 4
#define ZP 5
#define XNYN 0 /* Rhombic Dodecahedron */
#define XPYN 1
#define XNYP 2
#define XPYP 3
#define YNZN 4
#define YPZN 5
#define YNZP 6
#define YPZP 7
#define ZNXN 8
#define ZPXN 9
#define ZNXP 10 
#define ZPXP 11 

/* Relative ant moves */
#define TRS 0			/* Turn right, then step */
#define TLS 1			/* Turn left, then step */
#define TUS 2			/* Turn up, then step */
#define TDS 3			/* Turn down, then step */
#define FS  4			/* Step */
#define TBS 5			/* Turn back, then step */
/* Uhhh for completeness I guess  there are some yaw moves and a 180 TU */
#define STR 6			/* Step then turn right */
#define STL 7			/* Step then turn left */
#define STU 8			/* Step then turn up */
#define STD 9			/* Step then turn down */
#define SF  10			/* Step */
#define STB 11			/* Step then turn back */
/* Relative bee moves, leading R for Rhombic */
#define RTRS 0			/* Turn right, then step */
#define RTLS 1			/* Turn left, then step */
/* can also  include hard turns but did not yet */
#define RTURS 2			/* Turn up-right, then step */
#define RTDRS 3			/* Turn down-right, then step */
#define RTULS 4			/* Turn up-left, then step */
#define RTDLS 5			/* Turn down-left, then step */
#define RFS  6			/* Step */
#define RTBS 7			/* Turn back, then step */
/* Uhhh for completeness I guess  there are some yaw moves and a 180 TU */
#define RSTR 8			/* Step then turn right */
#define RSTL 9			/* Step then turn left */
#define RSTUR 10		/* Step then turn up-right */
#define RSTDR 11		/* Step then turn down-right */
#define RSTUL 12		/* Step then turn up-left */
#define RSTDL 13		/* Step then turn down-left */
#define RSF  14			/* Step */
/* turnDirection, oldGravity, oldDirection */
/*gravity XN, traveling along a direction YN, get TRS, now ZP */
static int newDirectionMap[4][6][6] = {
{ /* TRS */
	{NU, NU, ZP, ZN, YN, YP}, /* oldGravity = XN */
	{NU, NU, ZN, ZP, YP, YN},
	{ZN, ZP, NU, NU, XP, XN}, /* oldGravity = YN */
	{ZP, ZN, NU, NU, XN, XP},
	{YP, YN, XN, XP, NU, NU}, /* oldGravity = ZN */
	{YN, YP, XP, XN, NU, NU}
},
{ /* TLS */
	{NU, NU, ZN, ZP, YP, YN}, /* oldGravity = XN */
	{NU, NU, ZP, ZN, YN, YP},
	{ZP, ZN, NU, NU, XN, XP}, /* oldGravity = YN */
	{ZN, ZP, NU, NU, XP, XN},
	{YN, YP, XP, XN, NU, NU}, /* oldGravity = ZN */
	{YP, YN, XN, XP, NU, NU}
},
{ /* TUS */
	{NU, NU, XP, XP, XP, XP}, /* oldGravity = XN */
	{NU, NU, XN, XN, XN, XN},
	{YP, YP, NU, NU, YP, YP}, /* oldGravity = YN */
	{YN, YN, NU, NU, YN, YN},
	{ZP, ZP, ZP, ZP, NU, NU}, /* oldGravity = ZN */
	{ZN, ZN, ZN, ZN, NU, NU}
},
{ /* TDS */
	{NU, NU, XN, XN, XN, XN}, /* oldGravity = XN */
	{NU, NU, XP, XP, XP, XP},
	{YN, YN, NU, NU, YN, YN}, /* oldGravity = YN */
	{YP, YP, NU, NU, YP, YP},
	{ZN, ZN, ZN, ZN, NU, NU}, /* oldGravity = ZN */
	{ZP, ZP, ZP, ZP, NU, NU}
}
};
/* turnDirection, oldGravity, oldDirection */
static int newGravityMap[4][6][6] = {
{ /* TRS */
	{NU, NU, XN, XN, XN, XN}, /* oldGravity = XN */
	{NU, NU, XP, XP, XP, XP},
	{YN, YN, NU, NU, YN, YN}, /* oldGravity = YN */
	{YP, YP, NU, NU, YP, YP},
	{ZN, ZN, ZN, ZN, NU, NU}, /* oldGravity = ZN */
	{ZP, ZP, ZP, ZP, NU, NU}
},
{ /* TLS */
	{NU, NU, XN, XN, XN, XN}, /* oldGravity = XN */
	{NU, NU, XP, XP, XP, XP},
	{YN, YN, NU, NU, YN, YN}, /* oldGravity = YN */
	{YP, YP, NU, NU, YP, YP},
	{ZN, ZN, ZN, ZN, NU, NU}, /* oldGravity = ZN */
	{ZP, ZP, ZP, ZP, NU, NU}
},
{ /* TUS */
	{NU, NU, YN, YP, ZN, ZP}, /* oldGravity = XN */
	{NU, NU, YN, YP, ZN, ZP},
	{XN, XP, NU, NU, ZN, ZP}, /* oldGravity = YN */
	{XN, XP, NU, NU, ZN, ZP},
	{XN, XP, YN, YP, NU, NU}, /* oldGravity = ZN */
	{XN, XP, YN, YP, NU, NU}
},
{ /* TDS */
	{NU, NU, YP, YN, ZP, ZN}, /* oldGravity = XN */
	{NU, NU, YP, YN, ZP, ZN},
	{XP, XN, NU, NU, ZP, ZN}, /* oldGravity = YN */
	{XP, XN, NU, NU, ZP, ZN},
	{XP, XN, YP, YN, NU, NU}, /* oldGravity = ZN */
	{XP, XN, YP, YN, NU, NU}
}
};
/* turnDirection, oldGravity, oldDirection */
/* XNYN 0, XPYN 1, XNYP 2, XPYP 3
   YNZN 4, YPZN 5, YNZP 6, YPZP 7
   ZNXN 8, ZPXN 9, ZNXP 10,ZPXP 11 */
/*gravity XN, traveling along a direction YNZN, get RTRS, now YNZP */
static int newRDirectionMap[6][6][4] = {
{ /* RTRS:YNZN YPZN YNZP YPZP */
	{YPZN, YPZP, YNZN, YNZP}, /* YZ, oldGravity = XN */
	{YNZP, YNZN, YPZP, YPZN},
	{ZPXN, ZPXP, ZNXN, ZNXP}, /* ZX, oldGravity = YN */
	{ZNXP, ZNXN, ZPXP, ZPXN},
	{XPYN, XPYP, XNYN, XNYP}, /* XY, oldGravity = ZN */
	{XNYP, XNYN, XPYP, XPYN}
},
{ /* RTLS */
	{YNZP, YNZN, YPZP, YPZN}, /* oldGravity = XN */
	{YPZN, YPZP, YNZN, YNZP},
	{ZNXP, ZNXN, ZPXP, ZPXN}, /* oldGravity = YN */
	{ZPXN, ZPXP, ZNXN, ZNXP},
	{XNYP, XNYN, XPYP, XPYN}, /* oldGravity = ZN */
	{XPYN, XPYP, XNYN, XNYP}
},
{ /* RTURS */
	{ZNXN, XNYP, XNYN, ZPXN}, /* oldGravity = XN */
	{XPYN, ZNXP, ZPXP, XPYP},
	{XNYN, YNZP, YNZN, XPYN}, /* oldGravity = YN */
	{YPZN, XNYP, XPYP, YPZP},
	{YNZN, ZNXP, ZNXN, YPZN}, /* oldGravity = ZN */
	{ZPXN, YNZP, YPZP, ZPXP}
},
{ /* RTDRS */
	{ZNXP, XPYP, XPYN, ZPXP}, /* oldGravity = XN */
	{XNYN, ZNXN, ZPXN, XNYP},
	{XNYP, YPZP, YPZN, XPYP}, /* oldGravity = YN */
	{YNZN, XNYN, XPYN, YNZP},
	{YNZP, ZPXP, ZPXN, YPZP}, /* oldGravity = ZN */
	{ZNXN, YNZN, YPZN, ZNXP}
},
{ /* RTULS */
	{XNYN, ZNXN, ZPXN, XNYP}, /* oldGravity = XN */
	{ZNXP, XPYP, XPYN, ZPXP},
	{YNZN, XNYN, XPYN, YNZP}, /* oldGravity = YN */
	{XNYP, YPZP, YPZN, XPYP},
	{ZNXN, YNZN, YPZN, ZNXP}, /* oldGravity = ZN */
	{YNZP, ZPXP, ZPXN, YPZP}
},
{ /* RTDLS */
	{XPYN, ZNXP, ZPXP, XPYP}, /* oldGravity = XN */
	{ZNXN, XNYP, XNYN, ZPXN},
	{YPZN, XNYP, XPYP, YPZP}, /* oldGravity = YN */
	{XNYN, YNZP, YNZN, XPYN},
	{ZPXN, YNZP, YPZP, ZPXP}, /* oldGravity = ZN */
	{YNZN, ZNXP, ZNXN, YPZN}
}
};
/* turnDirection, oldGravity, oldDirection */
/* XNYN 0, XPYN 1, XNYP 2, XPYP 3
   YNZN 4, YPZN 5, YNZP 6, YPZP 7
   ZNXN 8, ZPXN 9, ZNXP 10,ZPXP 11 */
static int newRGravityMap[6][6][4] = {
{ /* RTRS:YNZN YPZN YNZP YPZP */
	{XN, XN, XN, XN}, /* oldGravity = XN */
	{XP, XP, XP, XP},
	{YN, YN, YN, YN}, /* oldGravity = YN */
	{YP, YP, YP, YP},
	{ZN, ZN, ZN, ZN}, /* oldGravity = ZN */
	{ZP, ZP, ZP, ZP}
},
{ /* RTLS */
	{XN, XN, XN, XN}, /* oldGravity = XN */
	{XP, XP, XP, XP},
	{YN, YN, YN, YN}, /* oldGravity = YN */
	{YP, YP, YP, YP},
	{ZN, ZN, ZN, ZN}, /* oldGravity = ZN */
	{ZP, ZP, ZP, ZP}
},
{ /* RTURS */
	{YP, ZP, ZN, YN}, /* oldGravity = XN */
	{ZP, YN, YP, ZN},
	{ZP, XP, XN, ZN}, /* oldGravity = YN */
	{XP, ZN, ZP, XN},
	{XP, YP, YN, XN}, /* oldGravity = ZN */
	{YP, XN, XP, YN}
},
{ /* RTDRS */
	{YN, ZN, ZP, YP}, /* oldGravity = XN */
	{ZN, YP, YN, ZP},
	{ZN, XN, XP, ZP}, /* oldGravity = YN */
	{XN, ZP, ZN, XP},
	{XN, YN, YP, XP}, /* oldGravity = ZN */
	{YN, XP, XN, YP}
},
{ /* RTULS */
	{ZP, YN, YP, ZN}, /* oldGravity = XN */
	{YP, ZP, ZN, YN},
	{XP, ZN, ZP, XN}, /* oldGravity = YN */
	{ZP, XP, XN, ZN},
	{YP, XN, XP, YN}, /* oldGravity = ZN */
	{XP, YP, YN, XN}
},
{ /* RTDLS */
	{ZN, YP, YN, ZP}, /* oldGravity = XN */
	{YN, ZN, ZP, YP},
	{XN, ZP, ZN, XP}, /* oldGravity = YN */
	{ZN, XN, XP, ZP},
	{YN, XP, XN, YP}, /* oldGravity = ZN */
	{XN, YN, YP, XP}
}
};
static antfarm3dstruct *antfarm3ds = NULL;

static unsigned char tables[][3 * NCOLORS * STATES + 2] =
{
#if 0
/* Generated */
	{		/* 0: RLU */
		3, 1,
		1, TRS, 0, 2, TLS, 0, 0, TUS, 0
	},
	{		/* 1: RUL */
		3, 1,
		1, TRS, 0, 2, TUS, 0, 0, TLS, 0
	},
	{		/* 2: RUD */
		3, 1,
		1, TRS, 0, 2, TUS, 0, 0, TDS, 0
	},
	{		/* 3: RLUU */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TUS, 0, 0, TUS, 0
	},
	{		/* 4: RRLU 32 */
		4, 1,
		1, TRS, 0, 2, TRS, 0, 3, TLS, 0, 0, TUS, 0
	},
	{		/* 5: RRUL ? */
		4, 1,
		1, TRS, 0, 2, TRS, 0, 3, TUS, 0, 0, TLS, 0
	},
	{		/* 6: RRUD 22 */
		4, 1,
		1, TRS, 0, 2, TRS, 0, 3, TUS, 0, 0, TDS, 0
	},
	{		/* 7: RLRU 188 */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TRS, 0, 0, TUS, 0
	},
	{		/* 8: RLLU ? */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TLS, 0, 0, TUS, 0
	},
	{		/* 9: RLUR 46 ! */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TUS, 0, 0, TRS, 0
	},
	{		/* 10: RLUL 32 */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TUS, 0, 0, TLS, 0
	},
	{		/* 11: RLUU 102 */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TUS, 0, 0, TUS, 0
	},
	{		/* 12: RLUD 28 */
		4, 1,
		1, TRS, 0, 2, TLS, 0, 3, TUS, 0, 0, TDS, 0
	},
	{		/* 13: RURL 30 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TRS, 0, 0, TLS, 0
	},
	{		/* 14: RURD ? */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TRS, 0, 0, TDS, 0
	},
	{		/* 15: RULL 22 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TLS, 0, 0, TLS, 0
	},
	{		/* 16: RULR 22 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TLS, 0, 0, TRS, 0
	},
	{		/* 17: RULU 22 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TLS, 0, 0, TUS, 0
	},
	{		/* 18: RUUL 26 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TUS, 0, 0, TLS, 0
	},
	{		/* 19: RULD 22 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TLS, 0, 0, TDS, 0
	},
	{		/* 20: RUUD 22 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TUS, 0, 0, TDS, 0
	},
	{		/* 21: RUDR 14 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TDS, 0, 0, TRS, 0
	},
	{		/* 22: RUDL ? */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TDS, 0, 0, TLS, 0
	},
	{		/* 23: RUDU 114 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TDS, 0, 0, TUS, 0
	},
	{		/* 24: RUDD 40 */
		4, 1,
		1, TRS, 0, 2, TUS, 0, 3, TDS, 0, 0, TDS, 0
	},
	{		/* 25: RRRULL ? */
		6, 1,
		1, TRS, 0, 2, TRS, 0, 3, TRS, 0, 4, TUS, 0,
		5, TLS, 0, 0, TLS, 0
	},
	{		/* 26: RLRUUUL 25436 */
		7, 1,
		1, TRS, 0, 2, TLS, 0, 3, TRS, 0, 4, TUS, 0,
		5, TUS, 0, 6, TUS, 0, 0, TLS, 0
	},
#endif
	{		/* 27: RRLDDDULRRLLLL * */
		14, 1,
		1, TRS, 0, 2, TRS, 0, 3, TLS, 0, 4, TDS, 0,
		5, TDS, 0, 6, TDS, 0, 7, TUS, 0, 8, TLS, 0,
		9, TRS, 0, 10, TRS, 0, 11, TLS, 0, 12, TLS, 0,
		13, TLS, 0, 0, TLS, 0
	}
};

#define NTABLES   (sizeof tables / sizeof tables[0])

#ifdef gDEBUG
static char * printDir(int dir, Bool omega)
{
	if (omega) {
		switch (dir) {
		case XNYN: return (char *) "XNYN";
		case XPYN: return (char *) "XPYN";
		case XNYP: return (char *) "XNYP";
		case XPYP: return (char *) "XPYP";
		case YNZN: return (char *) "YNZN";
		case YPZN: return (char *) "YPZN";
		case YNZP: return (char *) "YNZP";
		case YPZP: return (char *) "YPZP";
		case ZNXN: return (char *) "ZNXN";
		case ZPXN: return (char *) "ZPXN";
		case ZNXP: return (char *) "ZNXP";
		case ZPXP: return (char *) "ZPXP";
		default: return (char *) "NUNU";
		}
	} else {
		switch (dir) {
		case XN: return (char *) "XN";
		case XP: return (char *) "XP";
		case YN: return (char *) "YN";
		case YP: return (char *) "YP";
		case ZN: return (char *) "ZN";
		case ZP: return (char *) "ZP";
		default: return (char *) "NU";
		}
	}
}

static char * printTurn(antfarm3dstruct * ap, int dir)
{
	if (ap->neighbors == 12) {
		switch (dir) {
		case RTRS: return (char *) "RTRS";
		case RTLS: return (char *) "RTLS";
		case RTURS: return (char *) "RTURS";
		case RTDRS: return (char *) "RTDRS";
		case RTULS: return (char *) "RTULS";
		case RTDLS: return (char *) "RTDLS";
		default: return (char *) "RTXX";
		}
	} else {
		switch (dir) {
		case TRS: return (char *) "TRS";
		case TLS: return (char *) "TLS";
		case TUS: return (char *) "TUS";
		case TDS: return (char *) "TDS";
		default: return (char *) "TXX";
		}
	}
}
#endif

static void heading(antfarm3dstruct * ap, short * direction, short * gravity,
		short turn)
{
	short oldDirection = *direction;
	short oldGravity = *gravity;

	if (ap->neighbors == 12) {
		if (turn < 8) {
			if (turn == RTBS) {
				if (oldDirection & 1)
					*direction = oldDirection - 1;
				else
					*direction = oldDirection + 1;
			} else if (turn < 6) {
				*gravity = newRGravityMap
					[turn][oldGravity][oldDirection % 4];
				*direction = newRDirectionMap
					[turn][oldGravity][oldDirection % 4];
			}
		}
	} else {
		if (turn < 6) {
			if (turn == TBS) {
				if (oldDirection & 1)
					*direction = oldDirection - 1;
				else
					*direction = oldDirection + 1;
			} else if (turn < 4) {
				*gravity = newGravityMap
					[turn][oldGravity][oldDirection];
				*direction = newDirectionMap
					[turn][oldGravity][oldDirection];
			}
		}
	}
	if (*gravity == NU)
		(void) fprintf(stderr, "gravity corruption\n");
	if (*direction == NU)
		(void) fprintf(stderr, "direction corruption\n");
#ifdef gDEBUG
	(void) printf(
		"odirection %s, ogravity %s, turn %s, direction %s, gravity %s\n",
		printDir(oldDirection, (ap->neighbors == 12)),
		printDir(oldGravity, False),
		printTurn(ap, turn),
		printDir(*direction, (ap->neighbors == 12)),
		printDir(*gravity, False));
#endif
}

static void
positionOfNeighbor(antfarm3dstruct * ap, int dir,
		int *pcol, int *prow, int *pstack)
{
	int col = *pcol, row = *prow, stack = *pstack;

	if (ap->neighbors == 12) {
		switch (dir) {
		case XNYN:
			col = (!col) ? ap->ncols - 1 : col - 1;
			row = (!row) ? ap->nrows - 1 : row - 1;
			break;
		case XPYN:
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			row = (!row) ? ap->nrows - 1 : row - 1;
			break;
		case XNYP:
			col = (!col) ? ap->ncols - 1 : col - 1;
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		case XPYP:
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		case YNZN:
			row = (!row) ? ap->nrows - 1 : row - 1;
			stack = (!stack) ? ap->nstacks - 1 : stack - 1;
			break;
		case YPZN:
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			stack = (!stack) ? ap->nstacks - 1 : stack - 1;
			break;
		case YNZP:
			row = (!row) ? ap->nrows - 1 : row - 1;
			stack = (stack + 1 == ap->nstacks) ? 0 : stack + 1;
			break;
		case YPZP:
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			stack = (stack + 1 == ap->nstacks) ? 0 : stack + 1;
			break;
		case ZNXN:
			stack = (!stack) ? ap->nstacks - 1 : stack - 1;
			col = (!col) ? ap->ncols - 1 : col - 1;
			break;
		case ZPXN:
			stack = (stack + 1 == ap->nstacks) ? 0 : stack + 1;
			col = (!col) ? ap->ncols - 1 : col - 1;
			break;
		case ZNXP:
			stack = (!stack) ? ap->nstacks - 1 : stack - 1;
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			break;
		case ZPXP:
			stack = (stack + 1 == ap->nstacks) ? 0 : stack + 1;
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			break;
		default:
			(void) fprintf(stderr, "wrong direction %d\n", dir);
		}
	} else {
		switch (dir) {
		case XN:
			col = (!col) ? ap->ncols - 1 : col - 1;
			break;
		case XP:
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			break;
		case YN:
			row = (!row) ? ap->nrows - 1 : row - 1;
			break;
		case YP:
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		case ZN:
			stack = (!stack) ? ap->nstacks - 1 : stack - 1;
			break;
		case ZP:
			stack = (stack + 1 == ap->nstacks) ? 0 : stack + 1;
			break;
		default:
			(void) fprintf(stderr, "wrong direction %d\n", dir);
		}
	}
	*pcol = col;
	*prow = row;
	*pstack = stack;
}

/*--- list ---*/
/* initialise the state of all cells to OFF */
static void
Init3D(antfarm3dstruct * ap)
{
	ap->ptrhead = ap->ptrend = NULL;
	ap->eraserhead.next = &ap->eraserend;
	ap->eraserend.prev = &ap->eraserhead;
}

/*-
 * Function that adds the cell (assumed live) at (x,y,z) onto the search
 * list so that it is scanned in future generations
 */
static Bool
AddToList(antfarm3dstruct * ap,
		unsigned int x, unsigned int y, unsigned int z, unsigned int c)
{
	antstruct  *tmp;

	if ((tmp = (antstruct *) malloc(sizeof (antstruct))) == NULL)
		return False;
	tmp->col = x;
	tmp->row = y;
	tmp->stack = z;
	tmp->color = c;
	if (ap->ptrhead == NULL) {
		ap->ptrhead = ap->ptrend = tmp;
		tmp->prev = NULL;
	} else {
		ap->ptrend->next = tmp;
		tmp->prev = ap->ptrend;
		ap->ptrend = tmp;
	}
	ap->ptrend->next = NULL;
	return True;
}

static void
ChangeInList(antfarm3dstruct * ap, int i,
		unsigned int x, unsigned int y, unsigned int z, unsigned int c)
{
	antstruct  *ant = ap->antList[i];
	ant->col = x;
	ant->row = y;
	ant->stack = z;
	ant->color = c;
}

static void
AddToEraseList(antfarm3dstruct * ap, antstruct * cell)
{
	cell->next = &ap->eraserend;
	cell->prev = ap->eraserend.prev;
	ap->eraserend.prev->next = cell;
	ap->eraserend.prev = cell;
}

static void
DelFromList(antfarm3dstruct * ap, antstruct * cell)
{
	if (cell != ap->ptrhead) {
		cell->prev->next = cell->next;
	} else {
		ap->ptrhead = cell->next;
		if (ap->ptrhead != NULL)
			ap->ptrhead->prev = NULL;
	}

	if (cell != ap->ptrend) {
		cell->next->prev = cell->prev;
	} else {
		ap->ptrend = cell->prev;
		if (ap->ptrend != NULL)
			ap->ptrend->next = NULL;
	}
}

static void
EraseFromList(antfarm3dstruct * ap, antstruct * cell)
{
	DelFromList(ap, cell);
	AddToEraseList(ap, cell);
}

static void
DelFromEraseList(antstruct * cell)
{
	cell->next->prev = cell->prev;
	cell->prev->next = cell->next;
	free(cell);
}

/*--- memory ---*/
/*-
 * Simulate a large array by dynamically allocating 4x4x4 size cells when
 * needed.
 */
static void
MemInit(antfarm3dstruct * ap)
{
	int         i;

	for (i = 0; i < BASESIZE; ++i) {
		if (ap->base[i] != NULL) {
			free(ap->base[i]);
			ap->base[i] = (unsigned char *) NULL;
		}
	}
	ap->memstart = 0;
}

#define BASE_OFFSET(x,y,z,b,o) \
b = ((x & 0x7c) << 7) + ((y & 0x7c) << 2) + ((z & 0x7c) >> 2); \
o = (x & 3) + ((y & 3) << 2) + ((z & 3) << 4); \
if (ap->base[b] == NULL) {\
if ((ap->base[b] = (unsigned char *) calloc(64, sizeof (unsigned char))) == NULL) {return False;}}

#if 0
static Bool
GetMem(antfarm3dstruct * ap, unsigned int x, unsigned int y, unsigned int z,
		int *m)
{
	int         b, o;

	if (ap->memstart) {
		MemInit(ap);
	}
	BASE_OFFSET(x, y, z, b, o);
	*m = ap->base[b][o];
	return True;
}
#endif

static Bool
SetMem(antfarm3dstruct * ap,
	unsigned int x, unsigned int y, unsigned int z, unsigned int val)
{
	int         b, o;

	if (ap->memstart) {
		MemInit(ap);
	}
	BASE_OFFSET(x, y, z, b, o);
	ap->base[b][o] = val;
	return True;
}

#if 0
static Bool
ChangeMem(antfarm3dstruct * ap,
		unsigned int x, unsigned int y, unsigned int z,
		unsigned int val)
{
	int         b, o;

	if (ap->memstart) {
		MemInit(ap);
	}
	BASE_OFFSET(x, y, z, b, o);
	ap->base[b][o] += val;
	return True;
}
#endif

static void
End3D(antfarm3dstruct * ap)
{
	antstruct  *ptr;

	while (ap->ptrhead != NULL) {
		/* Reset3D(ap->ptrhead->col, ap->ptrhead->row,
			ap->ptrhead->stack); */
		EraseFromList(ap, ap->ptrhead);
	}
	ptr = ap->eraserhead.next;
	while (ptr != &ap->eraserend) {
		DelFromEraseList(ptr);
		ptr = ap->eraserhead.next;
	}
	MemInit(ap);
}

#if 0
/* This does not seem to be needed */
static void
condenseList(antfarm3dstruct * ap) {
	antstruct  *ptr1 = ap->ptrhead;
	antstruct  *ptr2 = ap->ptrhead;
	antstruct  *tmp = NULL;
	while (ptr2->next != NULL) {
		ptr2 = ptr2->next;
		while (ptr1 != ptr2) {
			if (ptr1->col == ptr2->col &&
					ptr1->row == ptr2->row &&
					ptr1->stack == ptr2->stack) {
				int found = 0;
				int i;

				for (i = 0; i < ap->n; i++) {
					if (ap->antList[i] == ptr1) {
						found = 1;
						break;
					}
				}
				if (found == 0)
					tmp = ptr1;
			}
			ptr1 = ptr1->next;
			if (tmp != NULL) {
				DelFromList(ap, tmp);
				free(tmp);
				tmp = NULL;
			}
		}
	}
}
#endif

#ifndef BLACK_CELLS
static void
eraseEmpty (antfarm3dstruct * ap) {
	antstruct  *ptr1 = ap->ptrhead;
	antstruct  *ptr2 = ap->ptrhead;
	antstruct  *tmp = NULL;
	int col, row, stack;
	while (ptr1->next != NULL) {
		if (ptr1->color == 0) {
			int found = 0;
			int i;

			for (i = 0; i < ap->n; i++) {
				if (ap->antList[i] == ptr1) {
					found = 1;
					break;
				}
			}
			if (found == 0)
				tmp = ptr1;
		}
		col = ptr1->col;
		row = ptr1->row;
		stack = ptr1->stack;
		ptr1 = ptr1->next;
		if (tmp != NULL) {
			DelFromList(ap, tmp);
			free(tmp);
			tmp = NULL;
			/* may have to delete all in that position */
			ptr2 = ap->ptrhead;
			while (ptr1 != ptr2) {
				if (ptr2->col == col &&
						ptr2->row == row &&
						ptr2->stack == stack) {
					int found = 0;
					int i;

					for (i = 0; i < ap->n; i++) {
						if (ap->antList[i] == ptr2) {
							found = 1;
							break;
						}
					}
					if (found == 0)
						tmp = ptr2;
				}
				ptr2 = ptr2->next;
				if (tmp != NULL) {
					DelFromList(ap, tmp);
					free(tmp);
					tmp = NULL;
				}
			}
		}
	}
}
#endif

static Bool
RunAnt3D(antfarm3dstruct * ap)
{
	antstruct  *anant;
	statestruct *status;
	int         i, state_pos, tape_pos;
	unsigned char color;

	if (ap->ants == NULL)
		return False;

	ap->painted = True;
	for (i = 0; i < ap->n; i++) {
		anant = &ap->ants[i];
		tape_pos = anant->col + anant->row * ap->ncols +
			anant->stack * ap->nrows * ap->ncols;
		color = ap->tape[tape_pos];     /* read tape */
		state_pos = color + anant->state * ap->ncolors;
		status = &(ap->machine[state_pos]);
#ifdef gDEBUG
		(void) printf("status: color %d, turn %s, state %d, colors %d\n",
			status->color, printTurn(ap, status->turn),
			anant->state, ap->ncolors);
#endif
		ChangeList3D(i, anant->col, anant->row, anant->stack,
			status->color);
		ap->tape[tape_pos] = status->color;     /* write on tape */
#ifdef gDEBUG
		(void) printf("gen %d: ", ap->generation);
#endif
		heading(ap, &(anant->direction), &(anant->gravity), status->turn);
		/* Find direction of Bees or Ants. */
		/* Translate relative direction to actual direction */
		anant->state = status->next;

		/* Allow step first then turn */
#ifdef gDEBUG
		(void) printf("state %d, col %d, row %d, stack %d",
			anant->state, anant->col, anant->row, anant->stack);
#endif
		positionOfNeighbor(ap, anant->direction,
			&(anant->col), &(anant->row), &(anant->stack));
		SetList3D(anant->col, anant->row, anant->stack,
			NCOLORS);
		ap->antList[i] = ap->ptrend;
	}
	/*condenseList(ap);*/
#ifndef BLACK_CELLS
	eraseEmpty(ap);
#endif
	return True;
}

#if 0
static int
CountCells3D(antfarm3dstruct * ap)
{
	antstruct  *ptr;
	int         count = 0;

	ptr = ap->ptrhead;
	while (ptr != NULL) {
		++count;
		ptr = ptr->next;
	}
	return count;
}

void
DisplayList(antfarm3dstruct * ap)
{
	antstruct  *ptr;
	int         count = 0;

	ptr = ap->ptrhead;
	while (ptr != NULL) {
		(void) printf("(%x)=[%d,%d,%d] ", (int) ptr,
			ptr->x, ptr->y, ptr->z);
		ptr = ptr->next;
		++count;
	}
	(void) printf("Living cells = %d\n", count);
}

#endif

static void
getTable(ModeInfo * mi, int i)
{
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	int         j, total;
	unsigned char *patptr;
	statestruct *status;

	patptr = &tables[i][0];
	ap->ncolors = *patptr++;
	ap->nstates = *patptr++;
	total = ap->ncolors * ap->nstates;
	if (MI_IS_VERBOSE(mi))
		(void) fprintf(stdout,
			"ants %d, table number %d, colors %d, states %d\n",
			ap->n, i, ap->ncolors, ap->nstates);
	if (label)
		(void) sprintf(ap->ruleString, "%d table number %d", ap->n, i);
	for (j = 0; j < total; j++) {
		status = &(ap->machine[j]);
		status->color = *patptr++;
		status->turn = *patptr++;
		status->next = *patptr++;
#ifdef DEBUG
		(void) printf("Status: color %d, turn %d, colors %d\n",
			status->color, status->turn, ap->ncolors);
#endif
	}
}

static void
getTurk(ModeInfo * mi, int logClass, int number)
{
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	int power4, j, total;
	char rule[NCOLORS * STATES + 1];

#ifdef DEBUG
	(void) printf("logClass %d, number %d\n", logClass, number);
#endif
	power4 = 1 << ((logClass - 1) << 1);
	ap->ncolors = logClass;
	ap->nstates = 1;
	total = ap->ncolors * ap->nstates;
	for (j = 0; j < total; j++) {
		ap->machine[j].color = (j + 1) % total;
		ap->machine[j].turn = ((power4 * 3) & number) / power4;
		rule[j] = ap->machine[j].turn + '0';
		ap->machine[j].next = 0;
		power4 >>= 2;
	}
	rule[total] = '\0';
	if (MI_IS_VERBOSE(mi))
		(void) fprintf(stdout,
	 		"ants %d, Turk's number 0x%x (0%s)B4, colors %d\n",
			ap->n, number, rule, ap->ncolors);
	if (label)
		(void) sprintf(ap->ruleString, "%d Turk's number 0x%x",
			ap->n, number);
}

static void
parseRule(ModeInfo * mi)
{
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];

	if (rule) {
		int n = 0, l, number = 0, logClass = 0, power4;
		int bucket[4];

		bucket[0] = bucket[1] = bucket[2] = bucket[3] = 0;
		while (rule[n]) {
			if (rule[n] == 'T' || rule[n] == 't') {
				n++;
				while (rule[n]) {
					number = number * 10 +
						(int) (rule[n] - '0');
					n++;
				}
				getTable(mi, number);
				return;
			}
			if (rule[n] == 'A' || rule[n] == 'a') {
				if (!NRAND(2 * NUMSTIPPLES)) {
					getTable(mi, (int) (NRAND(NTABLES)));
					return;
				} else {
					int mask = 3, shiftNumber, j;

					logClass = (int) (NRAND(NUMSTIPPLES - 4)) + 4;
					/* <logClass = 4;> has 4 quadranary digits */
					power4 = 1 << (2 * (logClass - 1));
					do {
						bucket[0] = bucket[1] = bucket[2] = bucket[3] = 0;
						number = NRAND(power4 - 1) + power4 *
						((ap->neighbors == 12) ? 1 : (NRAND(3) + 1));
						/* leading 2 or 3 just cause boring spirals */
#ifdef DEBUG
						(void) printf("number 0x%x, power4 0x%x, logClass %d\n",
							number, power4, logClass);
#endif
						/* Want to make sure 3 of 4 quadranary digits are here */
						shiftNumber = number;
						for (j = 0; j < logClass; j++) {
							bucket[mask & shiftNumber]++;
							shiftNumber = shiftNumber >> 2;
						}
#ifdef DEBUG
						(void) printf("Buckets: %d %d %d %d\n",
							bucket[0], bucket[1], bucket[2], bucket[3]);
#endif
					} while ((!bucket[0] && !bucket[1]) || (!bucket[0] && !bucket[2]) ||
						 (!bucket[0] && !bucket[3]) || (!bucket[1] && !bucket[2]) ||
						 (!bucket[1] && !bucket[3]) || (!bucket[2] && !bucket[3]));

					getTurk(mi, logClass, number);
					return;
				}
			} else {
				l = rule[n] - '0';
				if (l >= 0 && l <= 3) {
					number = (number << 2) + l;
					if (logClass > 0 || l != 0)
						logClass++;
					bucket[l]++;
				}
			}
			n++;
		}
#ifdef DEBUG
		(void) printf("Buckets: %d %d %d %d\n",
			bucket[0], bucket[1], bucket[2], bucket[3]);
#endif
		if ((!bucket[0] && !bucket[1]) || (!bucket[0] && !bucket[2]) ||
			(!bucket[0] && !bucket[3]) || (!bucket[1] && !bucket[2]) ||
			(!bucket[1] && !bucket[3]) || (!bucket[2] && !bucket[3])) {
			if (ap->neighbors == 12) {
				switch (NRAND(5)) {
				case 0: getTurk(mi, 3, 0x12);
					break;
				case 1: getTurk(mi, 3, 0x13);
					break;
				case 2: getTurk(mi, 3, 0x18);
					break;
				case 3: getTurk(mi, 3, 0x1b);
					break;
				default: getTurk(mi, 4, 0x47);
					break;
				}
			} else {
				switch (NRAND(7)) {
				case 0: getTurk(mi, 3, 0x12);
					break;
				case 1: getTurk(mi, 3, 0x13);
					break;
				case 2: getTurk(mi, 3, 0x18);
					break;
				case 3: getTurk(mi, 3, 0x1b);
					break;
				case 4: getTurk(mi, 3, 0x21);
					break;
				case 5: getTurk(mi, 3, 0x27);
					break;
				default: getTurk(mi, 3, 0x2d);
					break;
				}
			}
		} else {
			getTurk(mi, logClass, number);
		}
	}
}

static void
NewViewpoint(antfarm3dstruct * ap, double x, double y, double z)
{
	double      k, l, d1, d2;

	k = x * x + y * y;
	l = sqrt(k + z * z);
	k = sqrt(k);
	d1 = (EyeToScreen / HalfScreenD);
	d2 = EyeToScreen / (HalfScreenD * ap->height / ap->width);
	ap->A = d1 * l * (ap->width / 2) / k;
	ap->B = l * l;
	ap->C = d2 * (ap->height / 2) / k;
	ap->F = k * k;
}


static void
lissajous(antfarm3dstruct * ap)
{
	double alt, azm, dist;

	alt = 44.0 * sin(ap->metaAlt * IP) + 45.0;
	ap->metaAlt += 1.123;
	if (ap->metaAlt >= 360.0)
		ap->metaAlt -= 360.0;
	if (ap->metaAlt < 0.0)
		ap->metaAlt += 360.0;
	azm = 44.0 * sin(ap->metaAzm * IP) + 45.0;
	ap->metaAzm += 0.987;
	if (ap->metaAzm >= 360.0)
		ap->metaAzm -= 360.0;
	if (ap->metaAzm < 0.0)
		ap->metaAzm += 360.0;
	dist = 10.0 * sin(ap->metaDist * IP) + 50.0;
	ap->metaDist += 1.0;
	if (ap->metaDist >= 360.0)
		ap->metaDist -= 360.0;
	if (ap->metaDist < 0.0)
		ap->metaDist += 360.0;
#if 0
	if (alt >= 90.0)
		alt = 90.0;
	else if (alt < -90.0)
		alt = -90.0;
#endif
	ap->azm = azm;
#ifdef FDEBUG
	(void) printf("dist %g, alt %g, azm %g\n", dist, alt, azm);
#endif
	ap->vx = (sin(azm * IP) * cos(alt * IP) * dist);
	ap->vy = (cos(azm * IP) * cos(alt * IP) * dist);
	ap->vz = (sin(alt * IP) * dist);
	NewViewpoint(ap, ap->vx, ap->vy, ap->vz);
}

static void
NewPoint(antfarm3dstruct * ap, double x, double y, double z,
	 register XPoint * pts)
{
	double      p1, E;

	p1 = x * ap->vx + y * ap->vy;
	E = ap->B - p1 - z * ap->vz;
	pts->x = (int) (ap->width / 2 -
		ap->A * (ap->vx * y - ap->vy * x) / E);
	pts->y = (int) (ap->height / 2 -
		ap->C * (z * ap->F - ap->vz * p1) / E);
}


/* Chain together all cells that are at the same distance.
 * These cannot mutually overlap. */
static void
SortList(antfarm3dstruct * ap)
{
	short       dist;
	double      d, col, row, stack, rsize;
	int         i, r;
	XPoint      point;
	antstruct  *ptr;

	for (i = 0; i < NBUCKETS; ++i)
		ap->buckethead[i] = ap->bucketend[i] = NULL;

	/* Calculate distances and re-arrange pointers to chain off buckets */
	ptr = ap->ptrhead;
	while (ptr != NULL) {

		col = (double) ptr->col - ap->ox;
		row = (double) ptr->row - ap->oy;
		stack = (double) ptr->stack - ap->oz;
		d = Distance(ap->vx, ap->vy, ap->vz, col, row, stack);
		if (ap->vx * (ap->vx - col) + ap->vy * (ap->vy - row) +
		    ap->vz * (ap->vz - stack) > 0 && d > 1.5)
			ptr->visible = 1;
		else
			ptr->visible = 0;

		ptr->dist = (short) d;
		dist = (short) (d * BUCKETSIZE);
		if (dist > NBUCKETS - 1)
			dist = NBUCKETS - 1;

		if (ap->buckethead[dist] == NULL) {
			ap->buckethead[dist] = ap->bucketend[dist] = ptr;
			ptr->priority = NULL;
		} else {
			ap->bucketend[dist]->priority = ptr;
			ap->bucketend[dist] = ptr;
			ap->bucketend[dist]->priority = NULL;
		}
		ptr = ptr->next;
	}

	/* Check for invisibility */
	rsize = 0.47 * ap->width / ((double) HalfScreenD * 2);
	i = (int) ap->azm;
	if (i < 0)
		i = -i;
	i = i % RT_ANGLE;
	if (i > HALFRT_ANGLE)
		i = RT_ANGLE - i;
	rsize /= cos(i * IP);

	for (i = 0; i < NBUCKETS; ++i)
		if (ap->buckethead[i] != NULL) {
			ptr = ap->buckethead[i];
			while (ptr != NULL) {
				if (ptr->visible) {
					col = (double) ptr->col - ap->ox;
					row = (double) ptr->row - ap->oy;
					stack = (double) ptr->stack - ap->oz;
					NewPoint(ap, col, row, stack, &point);

					r = (int) (rsize *
						(double) EyeToScreen /
						(double) ptr->dist);
					if (point.x + r < 0 ||
					    point.y + r < 0 ||
					    point.x - r >= ap->width ||
					    point.y - r >= ap->height) {
						ap->nInvisible++;
					} else if (ap->nInvisible) {
						ap->nInvisible = 0;
					}
				}
				ptr = ptr->priority;
			}
		}
}

static void
DrawEyes(ModeInfo * mi, int color, XPoint * pts,
		int p1, int p2, int p3, int p4)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc;
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	XPoint      facepts[5];
	XPoint      av;

	av.x = (pts[p1].x + pts[p2].x + pts[p3].x + pts[p4].x) / 4;
	av.y = (pts[p1].y + pts[p2].y + pts[p3].y + pts[p4].y) / 4;
	facepts[0] = pts[p1];
	facepts[1] = pts[p2];
	facepts[2] = pts[p3];
	facepts[3] = pts[p4];
	facepts[4] = pts[p1];
	facepts[0].x = (av.x + facepts[0].x) / 2;
	facepts[0].y = (av.y + facepts[0].y) / 2;
	facepts[1].x = (av.x + facepts[1].x) / 2;
	facepts[1].y = (av.y + facepts[1].y) / 2;
	facepts[2].x = (av.x + facepts[2].x) / 2;
	facepts[2].y = (av.y + facepts[2].y) / 2;
	facepts[3].x = (av.x + facepts[3].x) / 2;
	facepts[3].y = (av.y + facepts[3].y) / 2;
	facepts[4].x = (av.x + facepts[4].x) / 2;
	facepts[4].y = (av.y + facepts[4].y) / 2;

	if (color == NCOLORS) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
		gc = MI_GC(mi);
	} else if (!color) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_BLACK_PIXEL(mi));
		gc = MI_GC(mi);
	} else if (MI_NPIXELS(mi) > 2) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			MI_PIXEL(mi, ap->colors[color - 1]));
		gc = MI_GC(mi);
	} else {
		XGCValues   gcv;

		gcv.stipple = ap->pixmaps[(color - 1) % NUMSTIPPLES];
		gcv.foreground = MI_WHITE_PIXEL(mi);
		gcv.background = MI_BLACK_PIXEL(mi);
		XChangeGC(MI_DISPLAY(mi), ap->stippledGC,
			GCStipple | GCForeground | GCBackground, &gcv);
		gc = ap->stippledGC;
	}
	if (!ap->wireframe)
		XFillPolygon(display, (Drawable) ap->dbuf, gc, facepts, 4,
			Convex, CoordModeOrigin);
	gc = MI_GC(mi);
	if (!ap->wireframe) {
		if (color == NCOLORS) {
			XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
		} else {
			XSetForeground(display, gc, MI_WHITE_PIXEL(mi));
		}
	}
	XDrawLines(display, (Drawable) ap->dbuf, gc, facepts, 5,
		CoordModeOrigin);
}

static void
drawQuad(ModeInfo * mi, int dir, int color, antstruct * cell, XPoint * pts,
		int p1, int p2, int p3, int p4)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc;
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	XPoint      facepts[5];

	facepts[0] = pts[p1];
	facepts[1] = pts[p2];
	facepts[2] = pts[p3];
	facepts[3] = pts[p4];
	facepts[4] = pts[p1];

	if (color == NCOLORS) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
		gc = MI_GC(mi);
	} else if (!color) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_BLACK_PIXEL(mi));
		gc = MI_GC(mi);
	} else if (MI_NPIXELS(mi) > 2) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			MI_PIXEL(mi, ap->colors[color - 1]));
		gc = MI_GC(mi);
	} else {
		XGCValues   gcv;

		gcv.stipple = ap->pixmaps[(color - 1) % NUMSTIPPLES];
		gcv.foreground = MI_WHITE_PIXEL(mi);
		gcv.background = MI_BLACK_PIXEL(mi);
		XChangeGC(MI_DISPLAY(mi), ap->stippledGC,
			GCStipple | GCForeground | GCBackground, &gcv);
		gc = ap->stippledGC;
	}
	if (!ap->wireframe)
		XFillPolygon(display, (Drawable) ap->dbuf, gc, facepts, 4,
			Convex, CoordModeOrigin);
	gc = MI_GC(mi);
	if (!ap->wireframe) {
		if (color == NCOLORS) {
			XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
		} else {
			XSetForeground(display, gc, MI_WHITE_PIXEL(mi));
		}
	}
	XDrawLines(display, (Drawable) ap->dbuf, gc, facepts, 5, CoordModeOrigin);
	if (!ap->wireframe && ap->eyes && color == NCOLORS && dir >= 0) {
		antstruct  *anant;
		int i;

		for (i = 0; i < ap->n; i++) {
			anant = &ap->ants[i];
			if (anant->col == cell->col &&
					anant->row == cell->row &&
					anant->stack == cell->stack) {
#if 0
				(void) printf("%d: direction %d, gravity %d\n",
					i, anant->direction, anant->gravity);
#endif
				/* TODO: use gravity to position better eyes */
				if (anant->direction == dir)
					DrawEyes(mi, 0, pts, p1, p2, p3, p4);
			}
		}
	}
}

#define LEN 0.45
#define LEN2 0.9

static int
DrawCube(ModeInfo * mi, antstruct * cell)
{
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	XPoint pts[8];	/* screen coords for point */
	int i = 0, out = 0;
	unsigned int mask;
	double x, y, z;
	double dx, dy, dz;
	int color = cell->color;

	x = (double) cell->col - ap->ox;
	y = (double) cell->row - ap->oy;
	z = (double) cell->stack - ap->oz;
	for (dz = z - LEN; dz <= z + LEN2; dz += LEN2)
		for (dy = y - LEN; dy <= y + LEN2; dy += LEN2)
			for (dx = x - LEN; dx <= x + LEN2; dx += LEN2) {
				NewPoint(ap, dx, dy, dz, &pts[i]);
				if (pts[i].x < 0 ||
				    pts[i].x >= ap->width ||
				    pts[i].y < 0 ||
				    pts[i].y >= ap->height)
					++out;
				++i;
			}
	if (out == 8)
		return (0);

	if (cell->visible)
		mask = 0xFFFF;
	else
		mask = 0x0;

	/* Only draw those faces that are visible */
	dx = ap->vx - x;
	dy = ap->vy - y;
	dz = ap->vz - z;
	/*
		6----7
	       /    /|
	      /    / |
	     2----3  |
	    |  4 |  5
	    |    | /
	    0____1/
	*/
	if (ap->wireframe) {
		if (dz <= LEN)
			drawQuad(mi, -1, (int) (color & mask), cell, pts, 4, 5, 7, 6);
		else if (dz >= -LEN)
			drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 1, 3, 2);
		if (dx <= LEN)
			drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 3, 7, 5);
		else if (dx >= -LEN)
			drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 2, 6, 4);
		if (dy <= LEN)
			drawQuad(mi, -1, (int) (color & mask), cell, pts, 2, 3, 7, 6);
		else if (dy >= -LEN)
			drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 1, 5, 4);
	}
	if (dz > LEN)
		drawQuad(mi, ZP, (int) (color & mask), cell, pts, 4, 5, 7, 6);
	else if (dz < -LEN)
		drawQuad(mi, ZN, (int) (color & mask), cell, pts, 0, 1, 3, 2);
	if (dx > LEN)
		drawQuad(mi, XP, (int) (color & mask), cell, pts, 1, 3, 7, 5);
	else if (dx < -LEN)
		drawQuad(mi, XN, (int) (color & mask), cell, pts, 0, 2, 6, 4);
	if (dy > LEN)
		drawQuad(mi, YP, (int) (color & mask), cell, pts, 2, 3, 7, 6);
	else if (dy < -LEN)
		drawQuad(mi, YN, (int) (color & mask), cell, pts, 0, 1, 5, 4);
	return (1);
}

static int
DrawRhombicDodecahedron(ModeInfo * mi, antstruct * cell)
{
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	XPoint pts[14];	/* screen coords for point */
	int i, out = 0, da, db, sortx, sorty, sortz; unsigned int mask;
	double x, y, z;
	double dx, dy, dz;
	int color;

	x = (double) cell->col - ap->ox;
	y = (double) cell->row - ap->oy;
	z = (double) cell->stack - ap->oz;
	color = cell->color;
	i = 3;
	for (dx = x + LEN; dx >= x - LEN2; dx -= LEN2)
		for (dy = y + LEN; dy >= y - LEN2; dy -= LEN2)
			for (dz = z + LEN; dz >= z - LEN2; dz -= LEN2) {
				NewPoint(ap, dx, dy, dz, &pts[i]);
				if (pts[i].x < 0 ||
				    pts[i].x >= ap->width ||
				    pts[i].y < 0 ||
				    pts[i].y >= ap->height)
					++out;
				++i;
			}
	for (da = 0; da < 2; da++) {
		if (da == 1) {
			i = 0;
		} else
			i = 11; /* 3 + 8 */
		for (db = 0; db < 3; db++) {
			if (db == 0) {
				dx = ((da == 0) ? -LEN2 : LEN2) + x;
				dy = y;
				dz = z;
			} else if (db == 1) {
				dy = ((da == 0) ? -LEN2 : LEN2) + y;
				dz = z;
				dx = x;
			} else {
				dz = ((da == 0) ? -LEN2 : LEN2) + z;
				dx = x;
				dy = y;
			}
			NewPoint(ap, dx, dy, dz, &pts[i]);
			if (pts[i].x < 0 ||
			    pts[i].x >= ap->width ||
			    pts[i].y < 0 ||
			    pts[i].y >= ap->height)
				++out;
			++i;
		}
	}
	if (out == 14)
		return (0);

	if (cell->visible)
		mask = 0xFFFF;
	else
		mask = 0x0;

	/* Only draw those faces that are visible */
	dx = ap->vx - x;
	dy = ap->vy - y;
	dz = ap->vz - z;
	/* back points looking through front
	   8   1  4
	     7  3
	  11   2  0
	     9  5
	  10  12  6
	  ---------
	   front points
	  7    1  3
	     8  4
	  11  13  0
	    10  6
	  9   12  5
	*/
	if (ap->wireframe) {
		if (dx <= LEN) {
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 3, 2, 5);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 4, 1, 3);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		} else if (dx >= -LEN) {
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		}
		if (dy <= LEN) {
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 4, 1, 3);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 7, 2, 3);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		} else if (dy >= -LEN) {
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		}
		if (dz <= LEN) {
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 7, 2, 3);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 3, 2, 5);
		} else if (dz >= -LEN) {
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		drawQuad(mi, -1, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		}
	}
	sortx = sorty = sortz = 0;
	if (ABS(dz - LEN) > ABS(dy - LEN))
		sortz++;
	if (ABS(dz - LEN) > ABS(dx - LEN))
		sortz++;
	if (ABS(dy - LEN) > ABS(dx - LEN))
		sorty++;
	if (ABS(dy - LEN) > ABS(dz - LEN))
		sorty++;
	if (ABS(dx - LEN) > ABS(dz - LEN))
		sortx++;
	if (ABS(dx - LEN) > ABS(dy - LEN))
		sortx++;
	for (i = 0; i < 3; i++) {
	  if (sortx == i) {
	    if (dx > LEN) {
	      if (sortz > sorty) {
		if (dz < 0)
		  drawQuad(mi, ZPXP, (int) (color & mask), cell, pts, 0, 3, 2, 5);
		else
		  drawQuad(mi, ZNXP, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		drawQuad(mi, XPYN, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		drawQuad(mi, XPYP, (int) (color & mask), cell, pts, 0, 4, 1, 3);
		if (dz < 0)
		  drawQuad(mi, ZNXP, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		else
		  drawQuad(mi, ZPXP, (int) (color & mask), cell, pts, 0, 3, 2, 5);
	      } else {
		if (dy < 0)
		  drawQuad(mi, XPYP, (int) (color & mask), cell, pts, 0, 4, 1, 3);
		else
		  drawQuad(mi, XPYN, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		drawQuad(mi, ZNXP, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		drawQuad(mi, ZPXP, (int) (color & mask), cell, pts, 0, 3, 2, 5);
		if (dy < 0)
		  drawQuad(mi, XPYN, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		else
		  drawQuad(mi, XPYP, (int) (color & mask), cell, pts, 0, 4, 1, 3);
	      }
	    } else if (dx < -LEN) {
	      if (sortz > sorty) {
		if (dz < 0)
		  drawQuad(mi, ZPXN, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		else
		  drawQuad(mi, ZNXN, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		drawQuad(mi, XNYN, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		drawQuad(mi, XNYP, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		if (dz < 0)
		  drawQuad(mi, ZNXN, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		else
		  drawQuad(mi, ZPXN, (int) (color & mask), cell, pts, 2, 7, 11, 9);
	      } else {
		if (dy < 0)
		  drawQuad(mi, XNYP, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		else
	  	  drawQuad(mi, XNYN, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		drawQuad(mi, ZNXN, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		drawQuad(mi, ZPXN, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		if (dy < 0)
	  	  drawQuad(mi, XNYN, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		else
		  drawQuad(mi, XNYP, (int) (color & mask), cell, pts, 1, 8, 11, 7);
	      }
	    }
	  }
	  if (sorty == i) {
	    if (dy > LEN) {
	      if (sortx > sortz) {
		if (dx < 0)
		  drawQuad(mi, XPYP, (int) (color & mask), cell, pts, 0, 4, 1, 3);
		else
		  drawQuad(mi, XNYP, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		drawQuad(mi, YPZN, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		drawQuad(mi, YPZP, (int) (color & mask), cell, pts, 1, 7, 2, 3);
		if (dx < 0)
		  drawQuad(mi, XNYP, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		else
		  drawQuad(mi, XPYP, (int) (color & mask), cell, pts, 0, 4, 1, 3);
	      } else {
		if (dz < 0)
		  drawQuad(mi, YPZP, (int) (color & mask), cell, pts, 1, 7, 2, 3);
		else
		  drawQuad(mi, YPZN, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		drawQuad(mi, XNYP, (int) (color & mask), cell, pts, 1, 8, 11, 7);
		drawQuad(mi, XPYP, (int) (color & mask), cell, pts, 0, 4, 1, 3);
		if (dz < 0)
		  drawQuad(mi, YPZN, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		else
		  drawQuad(mi, YPZP, (int) (color & mask), cell, pts, 1, 7, 2, 3);
	      }
	    } else if (dy < -LEN) {
	      if (sortx > sortz) {
		if (dx < 0)
		  drawQuad(mi, XPYN, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		else
		  drawQuad(mi, XNYN, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		drawQuad(mi, YNZP, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		drawQuad(mi, YNZN, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		if (dx < 0)
		  drawQuad(mi, XNYN, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		else
		  drawQuad(mi, XPYN, (int) (color & mask), cell, pts, 0, 5, 12, 6);
	      } else {
		if (dz < 0)
		  drawQuad(mi, YNZP, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		else
		  drawQuad(mi, YNZN, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		drawQuad(mi, XNYN, (int) (color & mask), cell, pts, 9, 11, 10, 12);
		drawQuad(mi, XPYN, (int) (color & mask), cell, pts, 0, 5, 12, 6);
		if (dz < 0)
		  drawQuad(mi, YNZN, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		else
		  drawQuad(mi, YNZP, (int) (color & mask), cell, pts, 2, 9, 12, 5);
	      }
	    }
	  }
	  if (sortz == i) {
	    if (dz > LEN) {
	      if (sorty > sortx) {
		if (dy < 0)
		  drawQuad(mi, YPZP, (int) (color & mask), cell, pts, 1, 7, 2, 3);
		else
		  drawQuad(mi, YNZP, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		drawQuad(mi, ZPXP, (int) (color & mask), cell, pts, 0, 3, 2, 5);
		drawQuad(mi, ZPXN, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		if (dy < 0)
		  drawQuad(mi, YNZP, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		else
		  drawQuad(mi, YPZP, (int) (color & mask), cell, pts, 1, 7, 2, 3);
	      } else {
		if (dx < 0)
		  drawQuad(mi, ZPXP, (int) (color & mask), cell, pts, 0, 3, 2, 5);
		else
		  drawQuad(mi, ZPXN, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		drawQuad(mi, YNZP, (int) (color & mask), cell, pts, 2, 9, 12, 5);
		drawQuad(mi, YPZP, (int) (color & mask), cell, pts, 1, 7, 2, 3);
		if (dx < 0)
		  drawQuad(mi, ZPXN, (int) (color & mask), cell, pts, 2, 7, 11, 9);
		else
		  drawQuad(mi, ZPXP, (int) (color & mask), cell, pts, 0, 3, 2, 5);
	      }
	    } else if (dz < -LEN) {
	      if (sorty > sortx) {
		if (dy < 0)
		  drawQuad(mi, YPZN, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		else
		  drawQuad(mi, YNZN, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		drawQuad(mi, ZNXN, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		drawQuad(mi, ZNXP, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		if (dy < 0)
		  drawQuad(mi, YNZN, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		else
		  drawQuad(mi, YPZN, (int) (color & mask), cell, pts, 1, 4, 13, 8);
	      } else {
		if (dx < 0)
		  drawQuad(mi, ZNXP, (int) (color & mask), cell, pts, 0, 6, 13, 4);
		else
		  drawQuad(mi, ZNXN, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		drawQuad(mi, YNZN, (int) (color & mask), cell, pts, 6, 12, 10, 13);
		drawQuad(mi, YPZN, (int) (color & mask), cell, pts, 1, 4, 13, 8);
		if (dx < 0)
		  drawQuad(mi, ZNXN, (int) (color & mask), cell, pts, 8, 13, 10, 11);
		else
		  drawQuad(mi, ZNXP, (int) (color & mask), cell, pts, 0, 6, 13, 4);
	      }
	    }
	  }
	}
	return (1);
}

static void
DrawScreen(ModeInfo * mi)
{
	antfarm3dstruct *ap = &antfarm3ds[MI_SCREEN(mi)];
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	antstruct  *ptr;
	antstruct  *eraserptr;
	int         i;

	SortList(ap);

	XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
	XFillRectangle(display, (Drawable) ap->dbuf, gc, 0, 0,
		ap->width, ap->height);

	/* Erase dead cubes */
	eraserptr = ap->eraserhead.next;
	while (eraserptr != &ap->eraserend) {
		eraserptr->visible = 0;
		if (ap->neighbors == 12)
			(void) DrawRhombicDodecahedron(mi, eraserptr);
		else
			(void) DrawCube(mi, eraserptr);
		DelFromEraseList(eraserptr);
		eraserptr = ap->eraserhead.next;
	}

	/* draw furthest cubes first */
	for (i = NBUCKETS - 1; i >= 0; --i) {
		ptr = ap->buckethead[i];
		while (ptr != NULL) {
			if (ap->neighbors == 12)
				(void) DrawRhombicDodecahedron(mi, ptr);
			else
			/*if (ptr->visible) */
			/* v += */ (void) DrawCube(mi, ptr);
			ptr = ptr->priority;
			/* ++count; */
		}
	}
	if (label) {
		int size = MAX(MIN(MI_WIDTH(mi), MI_HEIGHT(mi)) - 1, 1);

		if (size >= 10 * FONT_WIDTH) {
			/* hard code these to corners */
			XSetForeground(display, gc, MI_WHITE_PIXEL(mi));
			/*XDrawString(display, ap->dbuf, gc,
				16 + ap->labelOffsetX,
				16 + ap->labelOffsetY + FONT_HEIGHT,
				ap->ruleString, strlen(ap->ruleString)); */
			XDrawString(display, (Drawable) ap->dbuf, gc,
				16 + ap->labelOffsetX, MI_HEIGHT(mi) - 16 -
				ap->labelOffsetY - FONT_HEIGHT / 2,
				ap->ruleString, strlen(ap->ruleString));
		}
	}
	XFlush(display);
	XCopyArea(display, (Drawable) ap->dbuf, MI_WINDOW(mi), gc,
		0, 0, ap->width, ap->height, 0, 0);
#if 0
	{
		int         count = 0, v = 0;


		(void) printf("Pop=%-4d  Viewpoint (%3d,%3d,%3d)  Origin (%3d,%3d,%3d)  Mode %dx%d\
(%d,%d) %d\n",
		     count, (int) (ap->vx + ap->ox), (int) (ap->vy + ap->oy),
			      (int) (ap->vz + ap->oz), (int) ap->ox, (int) ap->oy, (int) ap->oz,
			      ap->width, ap->height, ap->alt, ap->azm, v);
	}
#endif
}

static void
free_ant3d_screen(Display *display, antfarm3dstruct *ap)
{
	int shade;

	if (ap == NULL) {
		return;
	}
	if (ap->eraserhead.next != NULL) {
		End3D(ap);
	}
	if (ap->stippledGC != None) {
		XFreeGC(display, ap->stippledGC);
		ap->stippledGC = None;
	}
	for (shade = 0; shade < ap->init_bits; shade++) {
		XFreePixmap(display, ap->pixmaps[shade]);
	}
	ap->init_bits = 0;
	if (ap->dbuf != None) {
		XFreePixmap(display, ap->dbuf);
		ap->dbuf = None;
	}
	if (ap->tape != NULL) {
		free(ap->tape);
		ap->tape = (unsigned char *) NULL;
	}
	if (ap->antList != NULL) {
		free(ap->antList);
		ap->antList = (antstruct **) NULL;
	}
	if (ap->ants != NULL) {
		free(ap->ants);
		ap->ants = (antstruct *) NULL;
	}
	ap = NULL;
}

ENTRYPOINT void
free_ant3d(ModeInfo * mi)
{
	free_ant3d_screen(MI_DISPLAY(mi), &antfarm3ds[MI_SCREEN(mi)]);
}

ENTRYPOINT void
init_ant3d(ModeInfo * mi)
{
	Display *display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	antfarm3dstruct *ap;
	int i;

	MI_INIT(mi, antfarm3ds);
	ap = &antfarm3ds[MI_SCREEN(mi)];

	ap->nInvisible = 0;
#ifdef DO_STIPPLE
	if (MI_NPIXELS(mi) <= 2) {
		if (ap->stippledGC == None) {
			XGCValues   gcv;

			gcv.fill_style = FillOpaqueStippled;
			if ((ap->stippledGC = XCreateGC(display, window,
					GCFillStyle, &gcv)) == None) {
				free_ant3d_screen(display, ap);
				return;
			}
		}
		if (ap->init_bits == 0) {
			for (i = 1; i < NUMSTIPPLES; i++) {
				ANT3DBITS(stipples[i], STIPPLESIZE, STIPPLESIZE);
			}
		}
	}
#endif /* DO_STIPPLE */
	ap->generation = 0;
	ap->n = MI_COUNT(mi);
	if (ap->n < -MINANTS) {
		/* if ap->n is random ... the size can change */
		if (ap->ants != NULL) {
			free(ap->ants);
			ap->ants = NULL;
		}
		/*ap->n = NRAND(-ap->n - MINANTS + 1) + MINANTS;*/
		/* skew so that 1 is more likely */
		ap->n = -ap->n - (int) (sqrt(NRAND(ap->n *ap->n - 1))) - 1 +
			MINANTS;
	} else if (ap->n < MINANTS)
		ap->n = MINANTS;

	for (i = 0; i < NEIGHBORKINDS; i++) {
		if (neighbors == plots[i]) {
			ap->neighbors = plots[i];
			break;
		}
		if (i == NEIGHBORKINDS - 1) {
			ap->neighbors = plots[NRAND(NEIGHBORKINDS)];
			break;
		}
	}

	if (!ap->eraserhead.next) {
		ap->metaDist = (double) NRAND(360);
		ap->metaAlt = (double) NRAND(360);
		ap->metaAzm = (double) NRAND(360);
		ap->ncols = MAXCOLUMNS;
		ap->nrows = MAXROWS;
		ap->nstacks = MAXSTACKS;
		ap->ox = ap->ncols / 2;
		ap->oy = ap->nrows / 2;
		ap->oz = ap->nstacks / 2;

		Init3D(ap);
	} else {
		End3D(ap);
	}
	ap->labelOffsetX = NRAND(8);
	ap->labelOffsetY = NRAND(8);
	parseRule(mi);
	if (MI_NPIXELS(mi) > 2) {
		int offset = NRAND(MI_NPIXELS(mi));

		for (i = 0; i < (int) ap->ncolors - 1; i++) {
			ap->colors[i] = (unsigned char) ((offset +
				(i * MI_NPIXELS(mi) /
				(int) (ap->ncolors - 1))) % MI_NPIXELS(mi));
		}
	}
	if (ap->tape != NULL)
		free(ap->tape);
	if ((ap->tape = (unsigned char *) calloc(
			ap->ncols * ap->nrows * ap->nstacks,
			sizeof (unsigned char))) == NULL) {
		free_ant3d_screen(display, ap);
		return;
	}
	ap->width = MI_WIDTH(mi);
	ap->height = MI_HEIGHT(mi);
	ap->memstart = 1;
	/*ap->tablesMade = 0; */

	if (MI_IS_FULLRANDOM(mi)) {
		ap->wireframe = (NRAND(8) == 0);
	} else {
		ap->wireframe = MI_IS_WIREFRAME(mi);
	}

	MI_CLEARWINDOW(mi);
	ap->painted = False;

	lissajous(ap);
	if (ap->ants == NULL) {
		if ((ap->ants = (antstruct *) malloc(ap->n *
				sizeof (antstruct))) == NULL) {
			free_ant3d_screen(display, ap);
			return;
		}
	}
	if (ap->antList == NULL) {
		if ((ap->antList = (antstruct **) malloc(ap->n *
				sizeof (antstruct *))) == NULL) {
			free_ant3d_screen(display, ap);
			return;
		}
	}

	for (i = 0; i < ap->n; i++) {
		antstruct *anant = &ap->ants[i];
		anant->col = ap->ncols / 2;
		anant->row = ap->nrows / 2;
		anant->stack = ap->nstacks / 2;
		anant->direction = NRAND((ap->neighbors == 12) ? 12 : 6);
		anant->gravity = NRAND(6);
		if (ap->neighbors == 12) {
			if (anant->direction / 4 == 0) {
				anant->gravity = NRAND(2) + ZN;
			} else if (anant->direction / 4 == 1) {
				anant->gravity = NRAND(2) + XN;
			} else {
				anant->gravity = NRAND(2) + YN;
			}
		} else {
			while ((anant->gravity / 2) % 3 ==
					(anant->direction / 2) % 3)
				anant->gravity = NRAND(6);
		}
		anant->state = 0;
		InitSetList3D(anant->col, anant->row, anant->stack,
			NCOLORS);
		ap->antList[i] = ap->ptrend;
	}
	if (ap->dbuf != None) {
		XFreePixmap(display, ap->dbuf);
		ap->dbuf = None;
	}
	if ((ap->dbuf = XCreatePixmap(display, window,
		ap->width, ap->height, MI_DEPTH(mi))) == None) {
		free_ant3d_screen(display, ap);
		return;
	}
	if (MI_IS_FULLRANDOM(mi)) {
		ap->eyes = (Bool) (LRAND() & 1);
	} else {
		ap->eyes = eyes;
	}
	DrawScreen(mi);
}

ENTRYPOINT void
draw_ant3d(ModeInfo * mi)
{
	antfarm3dstruct *ap;

	if (antfarm3ds == NULL)
		return;
	ap = &antfarm3ds[MI_SCREEN(mi)];
	if (ap->eraserhead.next == NULL)
		return;

	if (!RunAnt3D(ap)) {
		if (ap->eraserhead.next != NULL)
			 End3D(ap);
		return;
	}
	lissajous(ap);
	DrawScreen(mi);
	MI_IS_DRAWN(mi) = True;

	if (++ap->generation > MI_CYCLES(mi) / ap->n || ap->nInvisible > 32) {
		/*CountCells3D(ap) == 0) */
		init_ant3d(mi);
	} else
		ap->painted = True;
}

ENTRYPOINT void
release_ant3d(ModeInfo * mi)
{
	if (antfarm3ds != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_ant3d_screen(MI_DISPLAY(mi), &antfarm3ds[screen]);
		free(antfarm3ds);
		antfarm3ds = (antfarm3dstruct *) NULL;
	}
}

#ifndef STANDALONE
ENTRYPOINT void
refresh_ant3d(ModeInfo * mi)
{
	antfarm3dstruct *ap;

	if (antfarm3ds == NULL)
		return;
	ap = &antfarm3ds[MI_SCREEN(mi)];

	if (ap->painted) {
		MI_CLEARWINDOW(mi);
	}
}
#endif

XSCREENSAVER_MODULE ("Ant3d", ant3d)

#endif /* MODE_ant3d */
