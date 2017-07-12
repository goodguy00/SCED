/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/ConstraintPt.h,v 1.0 1997/05/06 20:39:53 schenney Exp $
 *
 * $Log: ConstraintPt.h,v $
 * Revision 1.0  1997/05/06 20:39:53  schenney
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
**  ConstraintPt.h: Public header file for the constraintBox widget class.
*/

/*
**  The constraintPt class is a subclass of Composite. It is intended
**  to display a single constraint defining point at the time of
**  constraint selection.
*/

#ifndef _ConstraintPt_h
#define _ConstraintPt_h

/* Resources:
**
**  All the Composite resources plus:
**
 Name             Class       RepType     Default
 ----             -----       -------     -------
 foreground       Pixel       Pixel       XtDefaultForeground
 hSpace           HSpace      Int         4
 label            String      String      "0 0 0"
 leftBitmap       Bitmap      Bitmap      None
 extraCallback    Callback    Callback    NULL
 extraLabel       String      String      "Extra"
 extraRequired    Boolean     Boolean     FALSE
 extraState       Boolean     Boolean     FALSE
 titleCallback    Callback    Callback    NULL
 titleIsMenu      Boolean     Boolean     FALSE
 titleLabel       String      String      "Point"
 titleMenuName    String      String      "titleMenu"
 toggleCallback   Callback    Callback    NULL
 toggleRequired   Boolean     Boolean     TRUE
 toggleState      Boolean     Boolean     FALSE

*/

/* Define the special resource names. */
#ifndef XtNleftBitmap
#define XtNleftBitmap       "leftBitmap"
#endif
#ifndef XtNextraCallback
#define XtNextraCallback    "extraCallback"
#endif
#ifndef XtNextraLabel
#define XtNextraLabel       "extraLabel"
#endif
#ifndef XtNextraRequired
#define XtNextraRequired    "extraRequired"
#endif
#ifndef XtNextraState
#define XtNextraState       "extraState"
#endif
#ifndef XtNtitleCallback
#define XtNtitleCallback    "titleCallback"
#endif
#ifndef XtNtitleIsMenu
#define XtNtitleIsMenu      "titleIsMenu"
#endif
#ifndef XtNtitleLabel
#define XtNtitleLabel       "titleLabel"
#endif
#ifndef XtNtitleMenuName
#define XtNtitleMenuName    "titleMenuName"
#endif
#ifndef XtNtoggleCallback
#define XtNtoggleCallback   "toggleCallback"
#endif
#ifndef XtNtoggleRequired
#define XtNtoggleRequired   "toggleRequired"
#endif
#ifndef XtNtoggleState
#define XtNtoggleState      "toggleState"
#endif


typedef struct _ConstraintPtClassRec    *ConstraintPtWidgetClass;
typedef struct _ConstraintPtRec         *ConstraintPtWidget;

extern WidgetClass  constraintPtWidgetClass;

typedef struct _ConstraintPtGeometry {
    int height;
    int title_width;
    int label_width;
    int toggle_width;
    int extra_width;
    int total_width;
    } ConstraintPtGeometry;

/* Public procedure. */
extern void ConstraintPt_Change_Radio_Group(Widget, Widget);
extern void ConstraintPt_Query_Geometry(Widget, ConstraintPtGeometry*);
extern void ConstraintPt_Set_Geometry(Widget, ConstraintPtGeometry*);

#endif /* _ConstraintPt_h */
