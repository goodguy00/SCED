/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/ConstraintBox.h,v 1.0 1997/05/06 20:39:30 schenney Exp $
 *
 * $Log: ConstraintBox.h,v $
 * Revision 1.0  1997/05/06 20:39:30  schenney
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
**  ConstraintBox.h: Public header file for the constraintBox widget class.
*/

/*
**  The constraintBox class is a subclass of Composite. It is intended
**  to display constraint options, as well as menus and buttons for
**  manipulating constraints.
*/

#ifndef _ConstraintBox_h
#define _ConstraintBox_h

/* Resources:
**
**  All the Composite resources plus:
**
 Name             Class       RepType     Default
 ----             -----       -------     -------
 hSpace           HSpace      Int         4
 leftBitmap       Bitmap      Bitmap      None
 newCallback      Callback    Callback    NULL
 newIsMenu        Boolean     Boolean     TRUE
 newLabel         String      String      "New"
 newMenuName      String      String      "newMenu"
 modifyCallback   Callback    Callback    NULL
 modifyLabel      String      String      "Modify"
 stateCallback    Callback    Callback    NULL
 titleCallback    Callback    Callback    NULL
 titleIsCommand   Boolean     Boolean     FALSE
 titleIsMenu      Boolean     Boolean     FALSE
 titleLabel       String      String      "Constraints"
 titleMenuName    String      String      NULL
 vSpace           VSpace      Int         4

*/

/* Define the special resource names. */
#define XtNleftBitmap       "leftBitmap"
#define XtNnewCallback      "newCallback"
#define XtNnewIsMenu        "newIsMenu"
#define XtNnewLabel         "newLabel"
#define XtNnewMenuName      "newMenu"
#define XtNmodifyCallback   "modifyCallback"
#define XtNmodifyLabel      "modifyLabel"
#define XtNstateCallback    "stateCallback"
#define XtNtitleCallback    "titleCallback"
#define XtNtitleIsCommand   "titleIsCommand"
#define XtNtitleIsMenu      "titleIsMenu"
#define XtNtitleLabel       "titleLabel"
#define XtNtitleMenuName    "titleMenu"


typedef struct _ConstraintBoxClassRec   *ConstraintBoxWidgetClass;
typedef struct _ConstraintBoxRec        *ConstraintBoxWidget;

typedef struct _ConstraintBoxData {
    int     index;
    Boolean state;
    } ConstraintBoxData;

extern WidgetClass  constraintBoxWidgetClass;


/* Convenience routines. */
extern void     ConstraintBoxSetConstraints(Widget, int, String*, Boolean*);
extern void     ConstraintBoxAddConstraint(Widget, String, Boolean, int);
extern void     ConstraintBoxRemoveConstraint(Widget, int);
extern void     ConstraintBoxSetConstraintState(Widget, int, Boolean, Boolean);
extern void     ConstraintBoxSetConstraintLabel(Widget, int, char*);
extern Boolean  ConstraintBoxGetState(Widget, int);

#endif /* _ConstraintBox_h */
