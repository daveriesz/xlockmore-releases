/* -*- Mode: C; tab-width: 4 -*- */
/* text3d --- Shows moving 3D texts */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)text3d.cc	1.2 98/10/01 xlockmore";

#endif

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
 * This module is based on a demo of the gltt graphics library
 * Copyright (C) 1998 Stephane Rehel.
 *
 * See the gltt Official Site at http://home.worldnet.fr/~rehel/gltt/gltt.html
 *
 * My e-mail address is lassauge@sagem.fr
 *
 * Eric Lassauge  (August-18-1998)
 *
 * REVISION HISTORY:
 *       08/23/98: add better handling of "faulty" fontfile and randomize 
 *                 fontfile if '-ttfont' value is a directory.
 *		   Minor changes for AIX from Jouk Jansen (joukj@hrem.stm.tudelft.nl).
 * 
 * TODO : 
 *       I've still not really decided how to "move" the 3Dtext. 
 *       Function Animate needs still a lot of work! Help welcome !!
 *       
 */

#ifdef STANDALONE
#define PROGCLASS "Text3d"
#define HACK_INIT init_text3d
#define HACK_DRAW draw_text3d
#define text3d_opts xlockmore_opts
#define DEFAULTS "*delay: 100000 \n" \
 "*ncolors: 64 \n" \
 "*font: \n" \
 "*text: \n" \
 "*filename: \n" \
 "*fortunefile: \n" \
 "*program: \n"

extern "C" {
#include "xlockmore.h"		/* from the xscreensaver distribution */
}
#else /* !STANDALONE */
# include "xlock.h"		/* from the xlockmore distribution */
#include "vis.h"

#endif /* !STANDALONE */

#ifdef MODE_text3d

#if 1
#include <gltt/FTEngine.h>
#include <gltt/FTFace.h>
#include <gltt/FTInstance.h>
#include <gltt/FTGlyph.h>
#include <gltt/FTFont.h>
#include <gltt/GLTTOutlineFont.h>
#include <gltt/GLTTFont.h>
#include <gltt/GLTTGlyphPolygonizer.h>
#include <gltt/GLTTGlyphTriangulator.h>
#else
#include "FTEngine.h"
#include "FTFace.h"
#include "FTInstance.h"
#include "FTGlyph.h"
#include "FTFont.h"
#include "GLTTOutlineFont.h"
#include "GLTTFont.h"
#include "GLTTGlyphPolygonizer.h"
#include "GLTTGlyphTriangulator.h"
#endif

#include "text3d.h"
#include <GL/glu.h>

#define USE_BLANK
#ifdef USE_BLANK	/* if the module cannot create the font struct
			 * then use blank mode instead 
			 */
extern "C" {void init_blank(ModeInfo * mi);}
extern "C" {void draw_blank(ModeInfo * mi);}
extern "C" {void release_blank(ModeInfo * mi);}
extern "C" {void refresh_blank(ModeInfo * mi);}
#endif

/* Yes, it's an ugly mix of 'C' and 'C++' functions */
extern "C" {void init_text3d(ModeInfo * mi);}
extern "C" {void draw_text3d(ModeInfo * mi);}
extern "C" {void change_text3d(ModeInfo * mi);}
extern "C" {void release_text3d(ModeInfo * mi);}
extern "C" {void refresh_text3d(ModeInfo * mi);}

/* Manage Option vars */
/* I found this font on NT in c:\WINDOWS\SYSTEM\ARIAL.TTF can also be found
   on Windows95 in c:\windows\fonts\arial.ttf */
#define DEF_FONT "arial.ttf"
#define DEF_EXTRUSION  "25.0"
#define DEF_ROTAMPL  "1.0"
#define DEF_ROTFREQ  "0.001"
#define DEF_FONTSIZE  220
static float  	extrusion;
static float  	rampl;
static float  	rfreq;
static char 	*mode_font;

static XrmOptionDescRec opts[] =
{
     {"-ttfont", ".text3d.ttfont", XrmoptionSepArg, (caddr_t) NULL},
     {"-extrusion", ".text3d.extrusion", XrmoptionSepArg, (caddr_t) NULL},
     {"-rot_amplitude", ".text3d.rot_amplitude", XrmoptionSepArg, (caddr_t) NULL},
     {"-rot_frequency", ".text3d.rot_frequency", XrmoptionSepArg, (caddr_t) NULL}
};

static argtype vars[] =
{
{(caddr_t *) & mode_font, "ttfont", "TTFont", DEF_FONT, t_String},
{(caddr_t *) & extrusion, "extrusion", "Extrusion", DEF_EXTRUSION, t_Float},
{(caddr_t *) & rampl, "rot_amplitude", "RotationAmplitude", DEF_ROTAMPL, t_Float},
{(caddr_t *) & rfreq, "rot_frequency", "RotationFrequency", DEF_ROTFREQ, t_Float},
};

static OptionStruct desc[] =
{
     {"-ttfont filename", "Text3d TrueType font file name"},
     {"-extrusion float", "Text3d extrusion length"},
     {"-rot_amplitude float", "Text3d rotation amplitude"},
     {"-rot_frequency float", "Text3d rotation frequency"}
};

ModeSpecOpt text3d_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
ModStruct   text3d_description =
{"text3d", "init_text3d", "draw_text3d", "release_text3d",
 "refresh_text3d", "change_text3d", NULL, &text3d_opts,
 100000, 1, 1, 1, 64, 1.0, "",
 "Shows 3D texts", 0, NULL};
#endif

static double camera_dist= 0.0; /* could it be a per screen variable ? */
static double ref_camera_dist= 0.0; /* could it be a per screen variable ? */
static text3dstruct *text3d = (text3dstruct*) NULL;

/* Hacks to use $TOP/xlock/util.c and iostuff.c functions */
extern "C" {char       *getWords(int screen, int screens);}
extern "C" {FILE       *my_fopen(char *filename, char *type);}

#if HAVE_DIRENT_H
#define sel_font	sel_image /* "rename" the sel_image function */

extern "C" {void get_dir(char *fullpath, char *dir, char *filename);}
extern "C" {int  sel_font(struct dirent *name);}
extern "C" {int  scan_dir(const char *directoryname, 
		     struct dirent ***namelist,
                     int (*select) (struct dirent *),
                     int (*compare) (const void *, const void *));}
#endif /* HAVE_DIRENT_H */

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *    Mode funcs.
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

/*
 *-----------------------------------------------------------------------------
 *    Utils.
 *-----------------------------------------------------------------------------
 */

static int
readable(char *filename)
{
        FILE       *fp;

        if ((fp = my_fopen(filename, "r")) == NULL)
                return False;
        (void) fclose(fp);
        return True;
}
#ifdef AIXV3
extern "C" long int lrand48(void); /*Seems to missing in xlC for AIX3.2.5*/
#endif

	/*-------------------------------------------------------------*/
#if HAVE_DIRENT_H

#ifdef AIXV3
extern "C" long int lrand48(void); /*Seems to missing in xlC for AIX3.2.5*/
#endif

static void
randomFileFromList(char *directory,
                   struct dirent **filelist, int numfiles, char *file_local)
{
        int         num;

        num = NRAND(numfiles);
        if (strlen(directory) + strlen(filelist[num]->d_name) + 1 < 256) {
                (void) sprintf(file_local, "%s%s",
                               directory, filelist[num]->d_name);
        }
}

	/*-------------------------------------------------------------*/
static void
getRandomFontFile(char *randomfile, char *randomfile_local)
{
	/* mimic getRandomImageFile from $TOP/xlock/iostuff.c */
        char directory_r[DIRBUF];
        struct dirent ***fonts_list;
        struct dirent **font_list = (dirent**) NULL;
        int  num_list;

	/* extern char 'stolen' from $TOP/xlock/resource.c */				
        extern char filename_r[MAXNAMLEN];

        get_dir(randomfile, directory_r, filename_r);

        fonts_list = (struct dirent ***) malloc(sizeof (struct dirent **));

        num_list = scan_dir(directory_r, fonts_list, sel_font, (int(*)(const
						void*,const void*)) NULL);
        font_list = *fonts_list;
        (void) free((void *) fonts_list);
        if (num_list > 0) {
                randomFileFromList(directory_r, font_list, num_list, randomfile_local);
        }
}
#endif /* HAVE_DIRENT_H */

	/*-------------------------------------------------------------*/

#ifdef DIFFUSE_COLOR
static void hsv_to_rgb( double h, double s, double v,
                 double* r, double* g, double* b )
{
  double xh = fmod( h*360., 360 ) / 60.0,
          i = floor(xh),
          f = xh - i,
         p1 = v * (1 - s),
         p2 = v * (1 -(s * f)),
         p3 = v * (1 -(s * (1 - f)));

  switch( (int) i )
    {
    case 0: *r=  v; *g= p3; *b= p1; break;
    case 1: *r= p2; *g=  v; *b= p1; break;
    case 2: *r= p1; *g=  v; *b= p3; break;
    case 3: *r= p1; *g= p2; *b=  v; break;
    case 4: *r= p3; *g= p1; *b=  v; break;
    case 5: *r=  v; *g= p1; *b= p2; break;
    }
}
#endif /* DIFFUSE_COLOR */

	/*-------------------------------------------------------------*/
static void spheric_camera( text3dstruct * tp,
                            float center_x, float center_y, float center_z,
                            float phi, float theta, float radius )
{
  float x= center_x + cos(phi) * cos(theta) * radius;
  float y= center_y + sin(phi) * cos(theta) * radius;
  float z= center_z +            sin(theta) * radius;

  float vx= -cos(phi) * sin(theta) * radius;
  float vy= -sin(phi) * sin(theta) * radius;
  float vz=             cos(theta) * radius;

  glViewport(0, 0, tp->WinW, tp->WinH);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective( 60, GLfloat(tp->WinW)/GLfloat(tp->WinH), 10, 10000 );

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt( x, y, z, center_x, center_y, center_z, vx, vy, vz );
}

	/*-------------------------------------------------------------*/
class GLTTGlyphTriangles: public GLTTGlyphTriangulator
{
public:
  struct Triangle
    {
    FTGlyphVectorizer::POINT* p1;
    FTGlyphVectorizer::POINT* p2;
    FTGlyphVectorizer::POINT* p3;
    };

  Triangle* triangles;
  int nTriangles;

  GLTTboolean count_them;

  GLTTGlyphTriangles( FTGlyphVectorizer* vectorizer ):
    GLTTGlyphTriangulator(vectorizer)
    {
    triangles= 0;
    nTriangles= 0;
    count_them= GLTT_TRUE;
    }
  virtual ~GLTTGlyphTriangles()
    {
    delete [] triangles;
    triangles= 0;
    }
  void alloc()
    {
    delete triangles;
    triangles= new Triangle [ nTriangles + 1 ];
    }
  virtual void triangle( FTGlyphVectorizer::POINT* p1,
                         FTGlyphVectorizer::POINT* p2,
                         FTGlyphVectorizer::POINT* p3 )
    {
    if( count_them )
      {
      ++nTriangles;
      return;
      }
    triangles[nTriangles].p1= p1;
    triangles[nTriangles].p2= p2;
    triangles[nTriangles].p3= p3;
    ++nTriangles;
    }
};

/*
 *-----------------------------------------------------------------------------
 *    "Main" local funcs.
 *-----------------------------------------------------------------------------
 */

static void
Reshape(ModeInfo * mi, int width, int height)
{
	text3dstruct *tp = &text3d[MI_SCREEN(mi)];

        glViewport(0, 0, tp->WinW = (GLint) width, tp->WinH = (GLint) height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, (GLdouble) width / (GLdouble) height, 10, 10000);
        glMatrixMode(GL_MODELVIEW);
}

	/*-------------------------------------------------------------*/
static void
Animate(text3dstruct * tp)
{
#ifdef FUNNY_ANIMATE
  	const double angle_speed= 2.5/180.0*M_PI;
	int key = NRAND(32);
  	switch (key)
    	{
    	case 0:case 1: case 2: case 3:
      		tp->theta+= angle_speed;
      		break;
    	case 4: case 5: case 6: case 7:
      		tp->theta-= angle_speed;
      		break;
    	case 8: case 9: case 10 : case 11:
      		tp->phi-= angle_speed;
      		break;
    	case 12: case 13: case 14: case 15:
      		tp->phi+= angle_speed;
		
    	case 16: case 17:
		if (camera_dist / 1.05 > ref_camera_dist)
      			camera_dist /= 1.05;
      		break;
    	case 18: case 19:
		if (camera_dist * 1.05 < (ref_camera_dist * 2.0))
      			camera_dist *= 1.05;
      		break;
    	case 20:
  		if ( (tp->extrusion - 5.0) > 25)
      			tp->extrusion -= 5.0;
      		break;
    	case 21:
  		if ( (tp->extrusion + 5.0) < 75)
      		tp->extrusion += 5.0;
      		break;
    	case 22:case 23:
      		tp->rampl /= 1.15;
      		break;
    	case 24:case 25:
      		tp->rampl *= 1.15;
      		break;
    	case 26:case 27:
      		tp->rfreq *= 1.15;
      		break;
    	case 28:case 29:
      		tp->rfreq /= 1.15;
      		break;
	}
#else /* !FUNNY_ANIMATE */
  	const double angle_speed= 2.5/180.0*M_PI;

      	tp->phi-= angle_speed;
      	tp->theta-= angle_speed;
#endif
}

	/*-------------------------------------------------------------*/
static void
Draw(text3dstruct * tp,
	Display    *display,
	Window      window)
{
  const int text_length= strlen(tp->words);

  GLTTFont font(tp->face);

  if( ! font.create(DEF_FONTSIZE) )
    return;

  FTGlyphVectorizer* vec= new FTGlyphVectorizer [ text_length ];
  GLTTGlyphTriangles** tri= new GLTTGlyphTriangles* [ text_length ];

  int i;
  for( i= 0; i < text_length; ++i )
    tri[i]= new GLTTGlyphTriangles( vec + i );

  if (camera_dist == 0.0)
  	ref_camera_dist = camera_dist = font.getWidth(tp->words) * 0.75;
  double min_y= 1e20;
  double max_y=-1e20;
  double size_x= 0.0;

  for( i= 0; i < text_length; ++i )
    {
    int ch= (unsigned char)tp->words[i];

    FTGlyph* g= font.getFont()->getGlyph(ch);
    if( g == 0 )
      continue;
    FTGlyphVectorizer& v= vec[i];
    v.setPrecision(14.);
    if( ! v.init(g) )
      continue;

    size_x+= v.getAdvance();

    if( ! v.vectorize() )
      continue;

    for( int c= 0; c < v.getNContours(); ++c )
      {
      FTGlyphVectorizer::Contour* contour= v.getContour(c);
      if( contour == 0 )
        continue;
      for( int j= 0; j < contour->nPoints; ++j )
        {
        FTGlyphVectorizer::POINT* point= contour->points + j;
        if( point->y < min_y ) min_y= point->y;
        if( point->y > max_y ) max_y= point->y;
        point->data= (void*) new double [ 6 ];
        }
      }
    GLTTGlyphTriangles* t= tri[i];

    if( ! t->init(g) )
      continue;

    t->count_them= GLTT_TRUE;
    t->nTriangles= 0;
    t->triangulate();

    t->count_them= GLTT_FALSE;
    t->alloc();
    t->nTriangles= 0;
    t->triangulate();
    }

  if( size_x == 0.0 )
  {
    fprintf(stderr,"Please give something to draw !\n");
    return;
  }

  double y_delta= (min_y+max_y)/2. + min_y + 50;
  for( i= 0; i < text_length; ++i )
    {
    FTGlyphVectorizer& v= vec[i];

    for( int c= 0; c < v.getNContours(); ++c )
      {
      FTGlyphVectorizer::Contour* contour= v.getContour(c);
      if( contour == 0 )
        continue;
      for( int j= 0; j < contour->nPoints; ++j )
        {
        FTGlyphVectorizer::POINT* point= contour->points + j;
        point->y -= y_delta;
        }
      }
    }

#define LIGHTY
  float front_emission[4]= { 0.,0.,0.,0 };
  float front_ambient[4]= { 0.2,0.2,0.2,0 };
  float front_diffuse[4]= { 0.95,0.95,0.8,0 };
  float  back_diffuse[4]= { 0.6,0.6,0.95,0 };
  float front_specular[4]= { 0.6,0.6,0.6,0 };
  glMaterialfv(GL_FRONT,GL_EMISSION,front_emission);
  glMaterialfv(GL_FRONT,GL_AMBIENT,front_ambient);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,front_diffuse);
  glMaterialfv(GL_FRONT,GL_SPECULAR,front_specular);
  glMaterialf(GL_FRONT,GL_SHININESS,32.);
#ifdef LIGHTY
  float light1_ambient[4]= { 0.2,0.2,0.2,1 };
  float light1_diffuse[4]= { 0.9,0.9,0.9,1 };	/* A "white" light */
  float light1_specular[4]= { 0.7,0.7,0.7,1 };
  float light1_position[4]= { -1,1,1,0 };
  glLightfv(GL_LIGHT1,GL_AMBIENT,light1_ambient);
  glLightfv(GL_LIGHT1,GL_DIFFUSE,light1_diffuse);
  glLightfv(GL_LIGHT1,GL_SPECULAR,light1_specular);
  glLightfv(GL_LIGHT1,GL_POSITION,light1_position);
  glEnable(GL_LIGHT1);

  float light2_ambient[4]= { 0.1,0.1,0.1,1 };
  float light2_diffuse[4]= { 0.8,0.2,0.2,1 };	/* A "red" light */
  float light2_specular[4]= { 0.5,0.5,0.5,1 };
  float light2_position[4]= { 1,-1,-1,0 };
  glLightfv(GL_LIGHT2,GL_AMBIENT,light2_ambient);
  glLightfv(GL_LIGHT2,GL_DIFFUSE,light2_diffuse);
  glLightfv(GL_LIGHT2,GL_SPECULAR,light2_specular);
  glLightfv(GL_LIGHT2,GL_POSITION,light2_position);
  glEnable(GL_LIGHT2);

#else
  GLfloat pos[4] = { -1.0,1.0,1.0,0.0 };
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glEnable(GL_LIGHT0);
#endif

  float  back_color[4]= { 0.2,0.2,0.6,0 };
  glMaterialfv(GL_BACK,GL_DIFFUSE,back_color);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_CULL_FACE);

  glColorMaterial(GL_FRONT,GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);

  spheric_camera( tp, tp->center_x,
                  tp->center_y + size_x/2.,
                  0,
                  tp->phi, tp->theta+M_PI/2, camera_dist );
  glClearColor(0,0,0,0);
  glEnable(GL_DEPTH_TEST);
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_NORMALIZE);

  double base_x= 0.;
  for( i= 0; i < text_length; ++i )
    {
    FTGlyphVectorizer& v= vec[i];

    int c;
    for( c= 0; c < v.getNContours(); ++c )
      {
      FTGlyphVectorizer::Contour* contour= v.getContour(c);
      if( contour == 0 )
        continue;

      for( int j= 0; j < contour->nPoints; ++j )
        {
        FTGlyphVectorizer::POINT* point= contour->points + j;
        double cx= -point->y;
        double cy= base_x+point->x;
        double phi= sin(cy*tp->rfreq)* tp->rampl * M_PI/2.;
        double rcx= cx * cos(phi);
        double rcz= cx * sin(phi);

        double* p= (double*) point->data;
        double* n= p + 3;

        p[0]= rcx;
        p[1]= cy;
        p[2]= rcz;

        n[0]= -sin(phi);
        n[1]= 0.;
        n[2]=  cos(phi);
        }
      }

    GLTTGlyphTriangles::Triangle* triangles= tri[i]->triangles;
    int nTriangles= tri[i]->nTriangles;

    glBegin(GL_TRIANGLES);

    for( int j= 0; j < nTriangles; ++j )
      {
      GLTTGlyphTriangles::Triangle& t= triangles[j];

      double* p1= ((double*) t.p1->data);
      double* p2= ((double*) t.p2->data);
      double* p3= ((double*) t.p3->data);
      double* n1= p1 + 3;
      double* n2= p2 + 3;
      double* n3= p3 + 3;

#ifdef LIGHTY
      glColor4fv(front_diffuse);
#endif

      glNormal3dv( n1 );
      glVertex3dv( p1 );
      glNormal3dv( n2 );
      glVertex3dv( p2 );
      glNormal3dv( n3 );
      glVertex3dv( p3 );

      glColor4fv(back_diffuse);

      glNormal3d( -n3[0], 0., -n3[2] );
      glVertex3d( p3[0]-n3[0]*tp->extrusion,
                  p3[1],
                  p3[2]-n3[2]*tp->extrusion );
      glNormal3d( -n2[0], 0., -n2[2] );
      glVertex3d( p2[0]-n2[0]*tp->extrusion,
                  p2[1],
                  p2[2]-n2[2]*tp->extrusion );
      glNormal3d( -n1[0], 0., -n1[2] );
      glVertex3d( p1[0]-n1[0]*tp->extrusion,
                  p1[1],
                  p1[2]-n1[2]*tp->extrusion );
      }
    glEnd();

    for( c= 0; c < v.getNContours(); ++c )
      {
      FTGlyphVectorizer::Contour* contour= v.getContour(c);
      if( contour == 0 )
        continue;
      glBegin(GL_QUAD_STRIP);
      for( int j= 0; j <= contour->nPoints; ++j )
        {
        int j1= (j < contour->nPoints) ? j : 0;
        int j0= (j1==0) ? (contour->nPoints-1) : (j1-1);

        FTGlyphVectorizer::POINT* point0= contour->points + j0;
        FTGlyphVectorizer::POINT* point1= contour->points + j1;
        double* p0= (double*) point0->data;
        double* p1= (double*) point1->data;
        double* e= p0 + 3;
        double vx= p1[0] - p0[0];
        double vy= p1[1] - p0[1];
        double vz= p1[2] - p0[2];
        double nx=           - vy * e[2];
        double ny= e[2] * vx - vz * e[0];
        double nz= e[0] * vy ;

#ifdef DIFFUSE_COLOR
        double u= double( (j*2) % contour->nPoints )
                    / double(contour->nPoints);

        double r, g, b;
        hsv_to_rgb(u,0.7,0.7,&r,&g,&b);

        glColor4f(r,g,b,1); // diffuse color of material
#else
	GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
	glColor4fv(blue);
#endif

        glNormal3f(nx,ny,nz);
        glVertex3f( p0[0]-e[0]*tp->extrusion,
                    p0[1],
                    p0[2]-e[2]*tp->extrusion );
        glNormal3f(nx,ny,nz);
        glVertex3f( p0[0], p0[1], p0[2] );
        }
      glEnd();
      }

    base_x += v.getAdvance();
    }

  glDisable(GL_NORMALIZE);
  glDisable(GL_LIGHTING);
  glShadeModel(GL_FLAT);
  glPopMatrix();

  glXSwapBuffers(display, window);

  for( i= 0; i < text_length; ++i )
    {
    delete tri[i];

    FTGlyphVectorizer& v= vec[i];
    for( int c= 0; c < v.getNContours(); ++c )
      {
      FTGlyphVectorizer::Contour* contour= v.getContour(c);
      if( contour == 0 )
        continue;
      for( int j= 0; j < contour->nPoints; ++j )
        {
        FTGlyphVectorizer::POINT* point= contour->points + j;
        delete [] point->data;
        point->data= 0;
        }
      }
    }

  delete [] tri;
  delete [] vec;
}

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *    Xlock hooks.
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

/*
 *-----------------------------------------------------------------------------
 *    Initialize text3d.  Called each time the window changes.
 *-----------------------------------------------------------------------------
 */

void
init_text3d(ModeInfo * mi)
{
	int         	screen = MI_SCREEN(mi);
	text3dstruct 	*tp;
	Display    	*display = MI_DISPLAY(mi);
	Window      	window = MI_WINDOW(mi);
	char 		*fontfile = (char *)NULL;

	if (text3d == NULL) {
		if ((text3d = (text3dstruct *) calloc(MI_NUM_SCREENS(mi),
					   sizeof (text3dstruct))) == NULL)
			return;
	}
	tp = &text3d[screen];
	tp->wire = MI_IS_WIREFRAME(mi);
        tp->extrusion = extrusion;
        tp->rampl = rampl;
        tp->rfreq = rfreq;

	/* Get fontfile from mode_font (it can be a dir name) */
        if (mode_font && strlen(mode_font)) 
	{
                fontfile = (char *) malloc(256);
                (void) strncpy(fontfile, mode_font, 256);
#if HAVE_DIRENT_H
                getRandomFontFile(mode_font, fontfile);
#endif
        }

	/* Check if fontfile exists */
        if (!fontfile || !strlen(fontfile) || (fontfile && !readable(fontfile)))
	{
                (void) fprintf(stderr,
                               "%s: could not read file \"%s\" !\n", MI_NAME(mi),fontfile);
		(void) free((void *) text3d);
		text3d = (text3dstruct*) NULL;
#ifdef USE_BLANK
                (void) fprintf(stderr,
                               "%s: jumping to 'blank' mode.\n", MI_NAME(mi));
		init_blank(mi);
#endif
		return;
	}

        tp->face= new FTFace;
	if( ! tp->face->open(fontfile) )
	{
		fprintf( stderr, "%s: unable to open True Type font %s !\n", MI_NAME(mi), fontfile);
		delete tp->face;
		(void) free((void *) text3d);
		text3d = (text3dstruct*) NULL;
#ifdef USE_BLANK
                (void) fprintf(stderr,
                               "%s: jumping to 'blank' mode.\n", MI_NAME(mi));
		init_blank(mi);
#endif
		return;
	}
        if (MI_IS_DEBUG(mi)) {
                (void) fprintf(stderr,
                               "%s:\n\tfontfile=%s .\n",
                               MI_NAME(mi),
                               fontfile
                        );
	}
	free(fontfile);

	tp->words = getWords(MI_SCREEN(mi), MI_NUM_SCREENS(mi));

	if ((tp->glx_context = init_GL(mi)) != NULL) {

		Reshape(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
		glDrawBuffer(GL_BACK);
        	if (MI_IS_DEBUG(mi)) {
                	(void) fprintf(stderr,
                               "%s:\n\tcamera_dist=%.1f\n\ttheta=%.1f\n\tphi=%.1f\n\textrusion=%.1f\n\trampl=%.1f.\n",
                               MI_NAME(mi),
                               camera_dist,
                               tp->theta,
                               tp->phi,
                               tp->extrusion,
                               tp->rampl
                        );
        	}
		glXSwapBuffers(display, window);

	} else {
		MI_CLEARWINDOW(mi);
	}
}

/*
 *-----------------------------------------------------------------------------
 *    Called by the mainline code periodically to update the display.
 *-----------------------------------------------------------------------------
 */
void
draw_text3d(ModeInfo * mi)
{
	text3dstruct *tp = (text3dstruct *) NULL;

	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

	MI_IS_DRAWN(mi) = True;

	if (text3d) 
		tp = &text3d[MI_SCREEN(mi)];
#ifdef USE_BLANK
	if (!tp)
	{
		draw_blank(mi);
		return;
	}
#else
	if (!tp || !tp->glx_context)
		return;
#endif
	glXMakeCurrent(display, window, *(tp->glx_context));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	Draw(tp,display,window);
	Animate(tp);
        if (MI_IS_DEBUG(mi)) {
                (void) fprintf(stderr,
                               "%s:\n\tcamera_dist=%.1f\n\ttheta=%.1f\n\tphi=%.1f\n\textrusion=%.1f\n\trampl=%.1f\n",
                               MI_NAME(mi),
                               camera_dist,
                               tp->theta,
                               tp->phi,
                               tp->extrusion,
                               tp->rampl
                        );
        }

	glXSwapBuffers(display, window);
}


/*
 *-----------------------------------------------------------------------------
 *    The display is being taken away from us.  Free up malloc'ed 
 *      memory and X resources that we've alloc'ed.  Only called
 *      once, we must zap everything for every screen.
 *-----------------------------------------------------------------------------
 */

void
release_text3d(ModeInfo * mi)
{
	int         screen;

	if (text3d != NULL) {
                for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
                        text3dstruct *tp = &text3d[screen];

                        if (tp->face)
				delete tp->face;
                }
		(void) free((void *) text3d);
		text3d = (text3dstruct*) NULL;
	}
#ifdef USE_BLANK
	else
		release_blank(mi);
#endif
	FreeAllGL(mi);
}

void
refresh_text3d(ModeInfo * mi)
{
        /* Do nothing, it will refresh by itself :) */
}

void
change_text3d(ModeInfo * mi)
{
	text3dstruct *tp = (text3dstruct *) NULL;

	if (text3d) 
		tp = &text3d[MI_SCREEN(mi)];

#ifdef USE_BLANK
	if (!tp)
	{
		refresh_blank(mi);
		return;
	}
#else
	if (!tp || !tp->glx_context)
		return;
#endif

	glXMakeCurrent(MI_DISPLAY(mi), MI_WINDOW(mi), *(tp->glx_context));
}

#endif /* MODE_text3d */
