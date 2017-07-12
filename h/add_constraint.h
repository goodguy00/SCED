/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/add_constraint.h,v 1.0 1997/05/06 20:40:47 schenney Exp $
 *
 * $Log: add_constraint.h,v $
 * Revision 1.0  1997/05/06 20:40:47  schenney
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

/*
**  add_constraint.h: Header for functions related to adding new constraints.
*/

#ifndef __ADD_CONSTRAINT_H__
#define __ADD_CONSTRAINT_H__

#define MAX_NAME_LENGTH 32
#define MAX_RATIO_LENGTH 16

/* A multitude of flags. */
#define HavePt1     (1)
#define HavePt2     (1<<1)
#define HavePt3     (1<<2)
#define HavePt4     (1<<3)
#define HavePt5     (1<<4)
#define HavePt6     (1<<5)
#define TwoisPt     (1<<6)
#define ThreeisPt   (1<<7)
#define TwoisNorm   (1<<8)
#define ThreeisNorm (1<<9)
#define RatioPlane  (1<<10)
#define RatioPoint  (1<<11)
#define Axis        (1<<12)
#define DirAxis     (1<<13)
#define NormPlane   (1<<14)
#define ConstNorm   (1<<15)
#define AllFlags    ( HavePt1 | HavePt2 | HavePt3 | HavePt4 | HavePt5 | \
                      HavePt6 | TwoisPt | ThreeisPt | TwoisNorm | \
                      ThreeisNorm | RatioPlane | RatioPoint | Axis | \
                      DirAxis | NormPlane | ConstNorm )

/* Type defs for the dialog box structures. */
typedef struct _AddPointSet     *AddPointSetPtr;
typedef struct _AddPointDialog  *AddPointDialogPtr;
typedef void    (*AddToggleCallback)(AddPointDialogPtr, int, Boolean);

typedef struct _AddPointSet {
    int         num_widgets;
    Widget      *point_widgets;
    AddToggleCallback   toggle_callback;
    AddToggleCallback   extra_callback;
    } AddPointSet;

typedef struct _AddPointDialog {
    Widget      shell;
    AddPointSet points;
    Widget      ratio;
    char        ratio_string[MAX_RATIO_LENGTH];
    Widget      name;
    char        name_string[MAX_NAME_LENGTH];
    Widget      select;
    Widget      done;
    Widget      remove;
    int         flags;
    int         menu_hit;
    } AddPointDialog;

/* Functions for updating/creating constraints. */
typedef Boolean (*Update_Constraint_Func)(ConstraintPtr, VectorPtr, VectorPtr);
extern Update_Constraint_Func   update_con_function[];

/* The various dialogs. */
extern AddPointDialog   new_plane_dialog;
extern AddPointDialog   new_line_dialog;
extern AddPointDialog   new_point_dialog;
extern AddPointDialog   new_sphere_dialog;
extern AddPointDialog   new_circle_dialog;
extern AddPointDialog   new_ratiopt_dialog;
extern AddPointDialog   new_axis_dialog;


/* Creation routines. */
extern void Add_Create_Plane_Dialog();
extern void Add_Create_Line_Dialog();
extern void Add_Create_Point_Dialog();
extern void Add_Create_Sphere_Dialog();
extern void Add_Create_Circle_Dialog();
extern void Add_Create_RatioPt_Dialog();
extern void Add_Create_Axis_Dialog();

/* Plane routines. */
extern void Add_Extra_Callback(AddPointDialogPtr, int, Boolean);
extern void Add_Plane_Plane_Pt_Callback(Widget, XtPointer, XtPointer);
extern void Add_Plane_Normal_Pt_Callback(Widget, XtPointer, XtPointer);

/* Line routines. */
extern void Add_Line_Line_Pt_Callback(Widget, XtPointer, XtPointer);
extern void Add_Line_Dir_Pt_Callback(Widget, XtPointer, XtPointer);

/* Sphere routines. */
extern void Add_Sphere_Extra_Callback(AddPointDialogPtr, int, Boolean);

/* Circle routines. */
extern void Add_Circle_Radius_Pt_Callback(Widget, XtPointer, XtPointer);
extern void Add_Circle_Plane_Pt_Callback(Widget, XtPointer, XtPointer);
extern void Add_Circle_Normal_Pt_Callback(Widget, XtPointer, XtPointer);

/* Axis routines. */
extern void Add_Axis_Extra_Callback(AddPointDialogPtr, int, Boolean);

/* Common routines. */
extern void Add_Dialog_Cancel_Callback(Widget, XtPointer, XtPointer);
extern void Add_Dialog_Done_Callback(Widget, XtPointer, XtPointer);
extern void Add_Dialog_Reset_Callback(Widget, XtPointer, XtPointer);
extern void Add_Dialog_Remove_Callback(Widget, XtPointer, XtPointer);

/* Width setting. */
extern void Add_Set_Widths(AddPointDialogPtr);


#endif /* __ADD_CONSTRAINT_H__ */

