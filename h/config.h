/* h/config.h.  Generated automatically by configure.  */
/*
 * $Header: /opt/sced/cvs/src/h/config.h.in,v 1.1.1.1 1997/09/07 15:23:15 chipr Exp $
 *
 * $Log: config.h.in,v $
 * Revision 1.1.1.1  1997/09/07 15:23:15  chipr
 * Original 1.0b2 sources
 *
 * Revision 1.0  1997/05/06 20:37:15  schenney
 * Initial revision
 *
 */
#define PATCHLEVEL 0
/*
**	sced: A Constraint Based Object Scene Editor
**
**	config.h : Various configuration definitions.
**
**	This file is intended to be modified by configure!
*/

#ifndef __CONFIG__
#define __CONFIG__

/* Define this to be 1 if you have problems with X atoms. */
#define MISSING_ATOMS 0

/* The next few lines are the only really system dependent stuff.
** They are max values for various types and epsilons for float comparison.
*/



/*
**	Things for configure to set.
*/

/* Whether or not we're using flex. */
#define FLEX 1

/* Whether or not we have getcwd
*/
#define HAVE_GETCWD 1

/* Whether or not we have strerror. */
#define HAVE_STRERROR 1

/*	Whether or not we have gzip.
*/
#define HAVE_GZIP 1

/* Whether or not we have malloc.h */
#define HAVE_MALLOC_H 1

#define HAVE_UNISTD_H 1

#define HAVE_SYS_WAIT_H 1
#define RETSIGTYPE void

/* Various max values (used as masks). */
#define SIZEOF_SHORT 2
#if ( SIZEOF_SHORT == 2 )
#define MAX_SIGNED_SHORT 0x7FFF
#define MAX_UNSIGNED_SHORT 0xFFFF
#else
#define MAX_SIGNED_SHORT 0x7FFFFFFF
#define MAX_UNSIGNED_SHORT 0xFFFFFFFF
#endif

/*
** Support for old input files.
*/
#define PREV_SUPPORT 1

/*
 * define ELK_SUPPORT if we want to include
 * support for scheme.
 */
#define ELK_SUPPORT 0

/*
**	Support flags for all the renderers.
*/
#define POVRAY_SUPPORT 1
#define RADIANCE_SUPPORT 0
#define RAYSHADE_SUPPORT 0
#define RENDERMAN_SUPPORT 0
#define VRML_SUPPORT 0


/* A Define to fix a bizarre HPUX bug with text widgets. */
#if ( HPUX || hpux )
#define HPFIX "   "
#else
#define HPFIX ""
#endif

/* Directory header file info. */
#define HAVE_DIRENT_H 1
#define HAVE_SYS_NDIR_H 0
#define HAVE_SYS_DIR_H 0
#define HAVE_NDIR_H 0

/* Strings header file info. */
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1

/* Whether or not const is defined. */
#define const const

/* Defaults for the raytracer locations for previewing. */
#define RAYSHADE_PATH ""
#define RAYSHADE_OPTIONS ""
#define RADIANCE_PATH ""
#define RADIANCE_OPTIONS ""
#define POVRAY_PATH "/usr/bin/povray"
#define POVRAY_OPTIONS ""
#define RENDERMAN_PATH ""
#define RENDERMAN_OPTIONS ""
#define VRML_PATH ""
#define VRML_OPTIONS ""

/*
**	The rest are defaults. All are setable through the config file or
**	Xresources.
*/

/* Define a  machine EPSILON, used to compare floats to 0.	*/
#define EPSILON 1.e-13
#define SMALL_NUM 1.e-5

/* The max size of the text entry buffer. */
/* Make it bigger or smaller depending on memory, but 128 is enough. */
#define ENTRY_STRING_LENGTH 128

/* Default atttribute values. */
#define DEFAULT_DIFFUSE 1.0
#define DEFAULT_SPECULAR 0.0
#define DEFAULT_REFLECT 0.0
#define DEFAULT_REFRACT 1.0
#define DEFAULT_TRANSPARENCY 0.0
#define DEFAULT_RED 1.0
#define DEFAULT_GREEN 1.0
#define DEFAULT_BLUE 1.0
#define DEFAULT_I_RED 1.0
#define DEFAULT_I_GREEN 1.0
#define DEFAULT_I_BLUE 1.0
#define DEFAULT_RADIUS 1.0
#define DEFAULT_TIGHTNESS 10.0
#define DEFAULT_SAMPLES ( 0x3 | ( 0x3 << 8 ) )
#define DEFAULT_JITTER FALSE
#define DEFAULT_INVERT FALSE


/* The initial viewport specifications. */
#define INIT_LOOK_FROM_X 5
#define INIT_LOOK_FROM_Y 4
#define INIT_LOOK_FROM_Z 3
#define INIT_LOOK_AT_X 0
#define INIT_LOOK_AT_Y 0
#define INIT_LOOK_AT_Z 0
#define INIT_LOOK_UP_X 0
#define INIT_LOOK_UP_Y 0
#define INIT_LOOK_UP_Z 1
#define INIT_DISTANCE 20
#define INIT_EYE_DIST 20
#define INIT_SCALE 100

/* The default width for the rectangle used to select objects by a single
** click.
*/
#define SELECTION_CLICK_WIDTH 3


/* Version control. */
#define VERSION "1"
#define VERSION_FLOAT 1.0
#define SUB_VERSION "03"


/*
**	Defines for math values if not already defined.
*/
#if ( ! defined(M_PI) )
#define M_PI 3.14159265358979323846
#endif
#if ( ! defined(M_PI_4) )
#define M_PI_4 0.78539816339744830962
#endif
#if ( ! defined(M_PI_2) )
#define M_PI_2 1.57079632679489661923
#endif
#if ( ! defined(M_SQRT1_2) )
#define M_SQRT1_2 0.70710678118654752440
#endif
#if ( ! defined(M_SQRT2) )
#define M_SQRT2 1.41421356237309504880
#endif
#if ( ! defined(HUGE_VAL) )
#define HUGE_VAL 1.0e10		/* Not very huge, but big enough. */
#endif

#endif
