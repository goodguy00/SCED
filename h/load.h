/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/load.h,v 1.0 1997/05/06 20:42:20 schenney Exp $
 *
 * $Log: load.h,v $
 * Revision 1.0  1997/05/06 20:42:20  schenney
 * Initial revision
 *
 */
/*
**    ScEd: A Constraint Based Scene Editor.
**    Copyright (C) 1994-1998  Stephen Chenney (schenney@cs.berkeley.edu)
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program; if not, write to the Free Software
**    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define PATCHLEVEL 0
/*
**  sced: A Constraint Based Object Scene Editor
**
**  load.h : definitions for the lexical analyser for load.c
*/

#define ERROR_TOKEN         -1
#define EOF_TOKEN           0
#define STRING_TOKEN        1
#define INT_TOKEN           2
#define FLOAT_TOKEN         3
#define MAINVIEW_TOKEN      4
#define CSGVIEW_TOKEN       5
#define LOOKFROM_TOKEN      6
#define LOOKAT_TOKEN        7
#define LOOKUP_TOKEN        8
#define VIEWDIST_TOKEN      9
#define EYEDIST_TOKEN       10
#define BASEOBJECTS_TOKEN   11
#define INSTANCES_TOKEN     12
#define WIREFRAME_TOKEN     13
#define ATTRIBUTES_TOKEN    14
#define TRANSFORM_TOKEN     15
#define COLOUR_TOKEN        16
#define DIFFUSE_TOKEN       17
#define SPECULAR_TOKEN      18
#define TRANSPARENCY_TOKEN  19
#define REFLECT_TOKEN       20
#define REFRACT_TOKEN       21
#define CAMERA_TOKEN        22
#define NONE_TOKEN          23
#define RAYSHADE_TOKEN      24
#define POVRAY_TOKEN        25
#define GENRAY_TOKEN        26
#define HFOV_TOKEN          27
#define VFOV_TOKEN          28
#define UP_TOKEN            29
#define RIGHT_TOKEN         30
#define SCREEN_TOKEN        31
#define MAGNIFY_TOKEN       32
#define LIGHT_TOKEN         33
#define AMBIENT_TOKEN       34
#define POSITION_TOKEN      35
#define REFERENCE_TOKEN     36
#define DEPENDENTS_TOKEN    37
#define CONSTRAINTS_TOKEN   38
#define PLANE_TOKEN         39
#define LINE_TOKEN          40
#define POINT_TOKEN         41
#define ACTIVE_TOKEN        42
#define LAYER_TOKEN         43
#define SCALE_TOKEN         44
#define ROTATE_TOKEN        45
#define AXES_TOKEN          46
#define ORIGIN_TOKEN        47
#define ALLIGN_TOKEN        48
#define SCENEDIR_TOKEN      49
#define UNION_TOKEN         50
#define INTERSECTION_TOKEN  51
#define DIFFERENCE_TOKEN    52
#define DEFAULT_TOKEN       53
#define MID_TOKEN           54
#define MAJOR_TOKEN         55
#define MINOR_TOKEN         56
#define VIEWPORT_TOKEN      57
#define DENSE_TOKEN         58
#define FULL_TOKEN          59
#define CSG_TOKEN           60
#define TARGET_TOKEN        61
#define INCLUDES_TOKEN      62
#define COMPRESS_TOKEN      63
#define RADIANCE_TOKEN      64
#define DECLARE_TOKEN       65
#define EXTEND_TOKEN        66
#define OPEN_TOKEN          67
#define VERS_TOKEN          68
#define NORMAL_TOKEN        69
#define UNKNOWN_TOKEN       70
#define GENSCAN_TOKEN       71
#define INTERNAL_TOKEN      72
#define OBJECT_TOKEN        73
#define MATRIX_TOKEN        74
#define MODE_TOKEN          75
#define RENDERMAN_TOKEN     76
#define LOAD_VRML_TOKEN     77
#define OPTIONS_TOKEN       78
#define ALIAS_TOKEN         79
#define FEATURES_TOKEN      80
#define END_TOKEN           81
#define AUTOSAVE_TOKEN      82
#define LOD_TOKEN           83
#define HEADER_TOKEN        84
#define AGG_TOKEN           85
#define INTENSITY_TOKEN     86
#define SAMPLE_TOKEN        87
#define RADIUS_TOKEN        88
#define TIGHTNESS_TOKEN     89
#define JITTER_TOKEN        90
#define INVERT_TOKEN        91
#define AGGVIEW_TOKEN       92

extern  int     line_num;
extern  long    lex_int;
extern  double  lex_float;
extern  char    *lex_string;

extern  FILE    *yyin;

extern char *merge_filename;

extern int      yylex();
extern void     Load_Internal_File(FILE*, int);
extern void     Load_Simple_File(FILE*, int, int);
extern int      Load_View(Viewport*, char*);

#if LEX==flex
extern  void    yyrestart(FILE*);
#endif


