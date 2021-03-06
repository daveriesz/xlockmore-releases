/* text3d2 --- Shows moving 3D texts */

/* Copyright (c) E. Lassauge, 2004. */

/*
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
 * Eric Lassauge  (March-09-2004)
 *
 */

extern      "C" {
#include <GL/gl.h>
#include <GL/glx.h>
} typedef struct {
#ifdef WIN32
	HGLRC       glx_context;
#else
	GLXContext  *glx_context;
#endif
	GLint       WinH, WinW;
	/* global Parameters */
	int         wire, counter;
	char        *words, *words_start;
	/* per Screen variables */
	FTFont      *font;
	rotator     *rot;
} text3d2struct;
