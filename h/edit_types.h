/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/edit_types.h,v 1.0 1997/05/06 20:41:34 schenney Exp $
 *
 * $Log: edit_types.h,v $
 * Revision 1.0  1997/05/06 20:41:34  schenney
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
**  edit_types.h : header for common editing types.
*/

#ifndef __SCED_EDIT_TYPES__
#define __SCED_EDIT_TYPES__


typedef struct _ConPoint {
    ConstraintSpecType  type;
    XPoint              pt;
    } ConPoint;

typedef struct _ConDefPoints {
    XPoint      *pts;
    int         num_pts;
    XArc        sphere_arc;
    ConPoint    *def_pts;
    int         num_def_pts;
    } ConDefPoints, *ConDefPointsPtr;

typedef struct _ConstrainedPoint {
    Vector          world;
    int             num_view;
    Vertex          *view;
    XArc            circle;
    Resultant       resultant;
    ConDefPoints    def_pts;
    } ConstraintPoint, *ConstraintPointPtr;


/* A type to hold all the general info about editing. */
typedef struct _EditInfo {
    WindowInfoPtr       window;
    InstanceList        inst;
    ObjectInstancePtr   obj;

    InstanceList    all_available;
    InstanceList    other_available;
    InstanceList    reference_available;

    Transformation  drag_transform;

    int                 max_features;
    ConstraintPointPtr  features;

    Matrix          axes;
    Matrix          axes_inverse;
    Resultant       rotate_resultant;
    ConDefPoints    arcball_pts;

    int         drag_type;
    Vector      drag_start;

    Boolean     selecting;
    Boolean     modifying;
    int         current_feature;
    int         constr_index;

    struct _EditStateType   *undo_stack;
    struct _EditStateType   *redo_stack;

    } EditInfo;


#endif /* __SCED_EDIT_TYPES__ */
