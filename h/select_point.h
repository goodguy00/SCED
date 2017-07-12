/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/select_point.h,v 1.0 1997/05/06 20:42:43 schenney Exp $
 *
 * $Log: select_point.h,v $
 * Revision 1.0  1997/05/06 20:42:43  schenney
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
**  select_point.h : header file for vertex selection functions as used
**                   by add_constraint.c and placement.c.
*/


#define SELECT      1
#define DESELECT    2
#define IGNORE      3

#define Spec_Equal(a, b, c) ( (a).spec_type == (b).spec_type && \
                              (a).spec_data == (b).spec_data && \
                              VEqual((a).spec_vector, (b).spec_vector, (c)) )

/* A procedure type for procedures called when the required number of points
** have been selected.
*/
typedef int (*SelectPointCallbackProc)(ConstraintSpecPtr);

/* A structure for reference vertices.  Holds a screen vertex and information
** about how to find it again.
*/
typedef struct _SelectPointStruct {
    XPoint              vert;
    ObjectInstancePtr   obj;
    int                 offset;
    } SelectPointStruct;

/* The window to do it in. */
extern WindowInfoPtr    select_window;

/* The vertices to select from. */
/*
extern int                  num_select_verts;
extern SelectPointStruct    *select_verts;
*/
extern InstanceList select_instances;

extern Vector   select_center;
extern Boolean  prompt_offset;

/* The callback to invoke when all the points have been selected.   */
extern SelectPointCallbackProc  select_callback;

/* Whether to allow text entry of points. */
extern Boolean  allow_text_entry;

/* Control of highlighting. */
extern Boolean  select_highlight;

/* Function declarations. */
extern void Select_Point_Action(Widget, XEvent*, String*, Cardinal*);
extern void Select_Point(XPoint, ConstraintSpecType);
extern void Select_Deselect_Point(ConstraintSpecPtr);
extern void Select_Highlight_Object(Widget, XEvent*, String*, Cardinal*);
extern void Select_Highlight_Action(Widget, XEvent*, String*, Cardinal*);
extern void Select_Highlight_Closest(Window);
extern void Selection_Recalculate_Points(Viewport*, short, short, double);
extern void Draw_Selection_Points(Drawable);
extern void Prepare_Selection_Drawing();
extern void Cleanup_Selection();

extern Vector   Spec_Absolute(ConstraintSpecPtr, Vector);

extern void Register_Select_Operation(WindowInfoPtr window);
extern void Cancel_Select_Operation(WindowInfoPtr window);

