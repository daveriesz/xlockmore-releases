/* text3d --- Shows moving 3D texts */

/* Copyright (c) E. Lassauge, 1998. */

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
 * My e-mail address is lassauge@sagem.fr
 *
 * Eric Lassauge  (August-18-1998)
 *
 */

extern      "C" {
#include <GL/gl.h>
#include <GL/glx.h>
} typedef struct {

	/* global Parameters */
	int         wire;
	float       extrusion;
	float       rampl;
	float       rfreq;
	char       *words;
	/* per Screen variables */
	GLint       WinH, WinW;
	GLXContext *glx_context;
	FTFace     *face;
	float       center_x, center_y, center_z;
	float       phi, theta, radius;
} text3dstruct;
