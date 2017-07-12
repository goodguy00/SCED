/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/ConstraintBoxP.h,v 1.0 1997/05/06 20:39:45 schenney Exp $
 *
 * $Log: ConstraintBoxP.h,v $
 * Revision 1.0  1997/05/06 20:39:45  schenney
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
**  ConstraintBoxP.h: Private header for constraintBoxWidgetClass.
*/

#define PATCHLEVEL 0

#ifndef _ConstraintBoxP_h
#define _ConstraintBoxP_h

#include <ConstraintBox.h>
#include <X11/CompositeP.h>

typedef struct {
    int empty;
    } ConstraintBoxClassPart;

typedef struct _ConstraintBoxClassRec {
    CoreClassPart           core_class;
    CompositeClassPart      composite_class;
    ConstraintBoxClassPart  constraintbox_class;
    } ConstraintBoxClassRec;

extern ConstraintBoxClassRec constraintBoxClassRec;

typedef struct {
    /* Resources. */
    int             h_space;
    int             v_space;
    XtCallbackList  new_callback;
    Boolean         new_is_menu;
    String          new_label;
    String          new_menu_name;
    XtCallbackList  modify_callback;
    String          modify_label;
    XtCallbackList  state_callback;
    XtCallbackList  title_callback;
    Boolean         title_is_menu;
    Boolean         title_is_command;
    String          title_label;
    String          title_menu_name;
    Pixmap          menu_bitmap;
    Pixel           foreground;

    /* Internal. */
    XPoint          box_min;
    XPoint          box_max;
    GC              box_gc;

    } ConstraintBoxPart;

typedef struct _ConstraintBoxRec {
    CorePart            core;
    CompositePart       composite;
    ConstraintBoxPart   constraint_box;
    } ConstraintBoxRec;

#endif /* _ConstraintBoxP_h */

