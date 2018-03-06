/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/ConstraintPt.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: ConstraintPt.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
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
**  ConstraintPt.c: Source code for the constraintPtWidgetClass.
*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <ConstraintPtP.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/ToggleP.h>

static void     ConstraintPtInitialise(Widget, Widget, ArgList, Cardinal*);
static void     ConstraintPtResize(Widget);
static Boolean  ConstraintPtSetValues(Widget, Widget, Widget, ArgList,
                                       Cardinal*);
static XtGeometryResult ConstraintPtGeometryManager(Widget, XtWidgetGeometry*,
                                                     XtWidgetGeometry*);
static XtGeometryResult ConstraintPtQueryGeometry(Widget, XtWidgetGeometry*,
                                                   XtWidgetGeometry*);

#define offset(field) XtOffsetOf(ConstraintPtRec, constraint_pt.field)

static XtResource   resources[] = {
    { XtNforeground, XtCColor, XtRPixel, sizeof(Pixel),
      offset(foreground), XtRString, XtDefaultForeground },
    { XtNhSpace, XtCHSpace, XtRInt, sizeof(int),
      offset(h_space), XtRImmediate, (XtPointer)4 },
    { XtNlabel, XtCString, XtRString, sizeof(String),
      offset(label), XtRImmediate, (XtPointer)"0 0 0" },
    { XtNleftBitmap, XtCBitmap, XtRBitmap, sizeof(Pixmap),
      offset(menu_bitmap), XtRPixmap, None },
    { XtNextraCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(extra_callback), XtRCallback, NULL },
    { XtNextraLabel, XtCString, XtRString, sizeof(String),
      offset(extra_label), XtRImmediate, (XtPointer)"Extra" },
    { XtNextraRequired, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(have_extra_toggle), XtRImmediate, (XtPointer)FALSE },
    { XtNextraState, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(extra_state), XtRImmediate, (XtPointer)FALSE },
    { XtNtitleCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(title_callback), XtRCallback, NULL },
    { XtNtitleIsMenu, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(title_is_menu), XtRImmediate, (XtPointer)FALSE },
    { XtNtitleLabel, XtCString, XtRString, sizeof(String),
      offset(title), XtRImmediate, (XtPointer)"Constraints" },
    { XtNtitleMenuName, XtCString, XtRString, sizeof(String),
      offset(title_menu_name), XtRString, NULL },
    { XtNtoggleCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList), 
      offset(toggle_callback), XtRCallback, NULL },
    { XtNtoggleRequired, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(toggle_required), XtRImmediate, (XtPointer)TRUE },
    { XtNtoggleState, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(toggle_state), XtRImmediate, (XtPointer)FALSE }
    };

#undef offset


/* Class record struct. */
ConstraintPtClassRec    constraintPtClassRec = {
    { /* Core fields. */
      /* superclass       */  (WidgetClass) &compositeClassRec,
      /* class_name       */  "ConstraintPt",
      /* widget_size      */  sizeof(ConstraintPtRec),
      /* class_initialize */  NULL,
      /* class_part_initialize    */  NULL,
      /* class_inited     */  FALSE,
      /* initialize       */  ConstraintPtInitialise,
      /* initialize_hook  */  NULL,
      /* realize          */  XtInheritRealize,
      /* actions          */  NULL,
      /* num_actions      */  0,
      /* resources        */  resources,
      /* num_resources    */  XtNumber(resources),
      /* xrm_class        */  NULLQUARK,
      /* compress_motion  */  TRUE,
      /* compress_exposure*/  TRUE,
      /* compress_enterleave  */  TRUE,
      /* visible_interest */  FALSE,
      /* destroy          */  NULL,
      /* resize           */  ConstraintPtResize,
      /* expose           */  NULL,
      /* set_values       */  ConstraintPtSetValues,
      /* set_values_hook  */  NULL,
      /* set_values_almost*/  XtInheritSetValuesAlmost,
      /* get_values_hook  */  NULL,
      /* accept_focus     */  NULL,
      /* version          */  XtVersion,
      /* callback_private */  NULL,
      /* tm_table         */  NULL,
      /* query_geometry   */  ConstraintPtQueryGeometry,  
      /* display_accelerator  */  XtInheritDisplayAccelerator,
      /* extension        */  NULL
    },
    { /* Composite fields.*/
      /* geometry manager.*/  ConstraintPtGeometryManager,
      /* change managed.  */  XtInheritChangeManaged,
      /* insert child.    */  XtInheritInsertChild,
      /* delete child.    */  XtInheritDeleteChild,
      /* extension.       */  NULL
    },
    { 0 }
    };

WidgetClass constraintPtWidgetClass = (WidgetClass)&constraintPtClassRec;

/* The pixmap for the toggle. */
static Pixmap   toggle_pixmap = 0;
#define select_width 12
#define select_height 12
static char select_bits[] = {
   0x00, 0x0c, 0x00, 0x0f, 0xe0, 0x07, 0xf8, 0x07, 0xf0, 0x03, 0xe0, 0x03,
   0xf0, 0x03, 0xb8, 0x01, 0x1c, 0x01, 0x0e, 0x00, 0x07, 0x00, 0x03, 0x00};

static char toggle_trans[] = "<EnterWindow>:highlight(Always)\n"
                             "<LeaveWindow>:unhighlight()\n"
                             "<Btn1Down>,<Btn1Up>:set() notify()";
static char menu_trans[] = "<EnterWindow>: highlight() \n\
                            <LeaveWindow>: reset() \n\
                            <BtnDown>:  set() notify() reset() PopupMenu()";


/* This function is invoked as the callback for the extra toggle.
*/
static void
ConstraintPtExtraToggleCallback(Widget w, XtPointer cl, XtPointer ca)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)XtParent(w);

    box->constraint_pt.extra_state = ca ? TRUE : FALSE;
    if ( box->constraint_pt.extra_callback )
        XtCallCallbackList((Widget)box, box->constraint_pt.extra_callback, ca);
}


static void
ConstraintPtToggleCallback(Widget w, XtPointer cl, XtPointer ca)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)XtParent(w);

    box->constraint_pt.toggle_state = ca ? TRUE : FALSE;
    if ( box->constraint_pt.toggle_callback )
        XtCallCallbackList((Widget)box, box->constraint_pt.toggle_callback, ca);
}


static void
ConstraintPtMenuCallback(Widget w, XtPointer cl, XtPointer ca)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)XtParent(w);

    if ( box->constraint_pt.title_callback )
        XtCallCallbackList((Widget)box, box->constraint_pt.title_callback,NULL);
}


/*  Returns the box's preferred geometry. */
static void
ConstraintPtQueryPreferredSizes(ConstraintPtWidget box, int *width, int *height)
{
    int i;

    /* Calculate the new width and height, based on the children. */
    *width = 0;
    *height = 0;
    for ( i = 0 ; i < box->composite.num_children ; i++ )
    {
        if ( ! XtIsManaged(box->composite.children[i]) )
            continue;
        if ( box->composite.children[i]->core.height +
             2 * box->composite.children[i]->core.border_width  > *height )
            *height = box->composite.children[i]->core.height +
                      2 * box->composite.children[i]->core.border_width;
        *width += box->composite.children[i]->core.width +
                   2 * box->composite.children[i]->core.border_width +
                   box->constraint_pt.h_space;
    }

    *width -= box->constraint_pt.h_space;
}


/* Do a layout, repositioning children.
** If request, request a new geometry for the widget.
*/
static void
ConstraintPtLayout(ConstraintPtWidget box, Boolean request)
{
    int height, temp_x;
    int width;
    int i;

    /* Reposition/Resize children. */
    ConstraintPtQueryPreferredSizes(box, &width, &height);

    /* Set the size of all the children. */
    for ( i = 0, temp_x = 0 ;
          i < box->composite.num_children ;
          i++ )
    {
        if ( ! XtIsManaged(box->composite.children[i]) )
            continue;
        XtConfigureWidget(box->composite.children[i],
                          temp_x, 0,
                          box->composite.children[i]->core.width,
                          box->composite.children[i]->core.height,
                          box->composite.children[i]->core.border_width);
        temp_x += box->composite.children[i]->core.width +
                  2 * box->composite.children[i]->core.border_width +
                  box->constraint_pt.h_space;
    }

    /* Request size from parent. */
    if ( request )
    {
        XtWidgetGeometry    req;

        req.request_mode = CWWidth | CWHeight;
        req.width = width;
        req.height = height;

        if ( XtMakeGeometryRequest((Widget)box,&req,&req) == XtGeometryAlmost )
            XtMakeGeometryRequest((Widget)box, &req, NULL);
    }
}


static void
ConstraintPtInitialise(Widget request, Widget new, ArgList orig_args,
                        Cardinal *num_args)
{   
    ConstraintPtWidget  box = (ConstraintPtWidget)new;
    Widget              menu, toggle, extra;
    Arg                 args[10];
    int                 n;

    int height;
    int width;

    /* Create children - title, label and toggles. */
    n = 0;
    box->constraint_pt.title = XtNewString(box->constraint_pt.title);
    XtSetArg(args[n], XtNlabel, box->constraint_pt.title);  n++;
    if ( box->constraint_pt.title_is_menu )
    {
        XtSetArg(args[n], XtNmenuName, box->constraint_pt.title_menu_name);n++;
        if ( box->constraint_pt.menu_bitmap )
        {
            XtSetArg(args[n], XtNleftBitmap, box->constraint_pt.menu_bitmap);
            n++;
        }
        menu =
            XtCreateManagedWidget("title", menuButtonWidgetClass, new, args, n);
        XtAddCallback(menu, XtNcallback, ConstraintPtMenuCallback, NULL);
        XtOverrideTranslations(menu, XtParseTranslationTable(menu_trans));
    }
    else
    {
        XtSetArg(args[n], XtNborderWidth, 0);   n++;
        XtCreateManagedWidget("title", labelWidgetClass, new, args, n);
    }

    n = 0;
    box->constraint_pt.label = XtNewString(box->constraint_pt.label);
    XtSetArg(args[n], XtNlabel, box->constraint_pt.label);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);                   n++;
    XtCreateManagedWidget("label", labelWidgetClass, new, args, n);

    if ( box->constraint_pt.toggle_required )
    {
        if ( ! toggle_pixmap )
            toggle_pixmap = XCreateBitmapFromData(XtDisplay(new),
                                    RootWindowOfScreen(XtScreen(new)),
                                    select_bits, select_width, select_height);
        n = 0;
        XtSetArg(args[n], XtNbitmap, toggle_pixmap);                        n++;
        XtSetArg(args[n], XtNforeground, box->constraint_pt.foreground);    n++;
        XtSetArg(args[n], XtNstate, box->constraint_pt.toggle_state);       n++;
        XtSetArg(args[n], XtNradioData, (XtPointer)new);                    n++;
        toggle = XtCreateManagedWidget("toggle", toggleWidgetClass, new,args,n);
        XtAddCallback(toggle, XtNcallback, ConstraintPtToggleCallback, NULL);
        XtOverrideTranslations(toggle, XtParseTranslationTable(toggle_trans));
    }

    if ( box->constraint_pt.have_extra_toggle )
    {
        box->constraint_pt.extra_label =
            XtNewString(box->constraint_pt.extra_label);
        n = 0;
        XtSetArg(args[n], XtNlabel, box->constraint_pt.extra_label);    n++;
        XtSetArg(args[n], XtNstate, box->constraint_pt.extra_state);    n++;
        extra = XtCreateManagedWidget("extra", toggleWidgetClass, new, args, n);
        XtAddCallback(extra, XtNcallback, ConstraintPtExtraToggleCallback,NULL);
    }

    /* Calculate the geometry. */
    ConstraintPtQueryPreferredSizes(box, &width, &height);
    box->core.width = width;
    box->core.height = height;
    ConstraintPtLayout(box, FALSE);
}

static void
ConstraintPtResize(Widget w)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)w;

    ConstraintPtLayout(box, FALSE);
}



static Boolean
ConstraintPtSetValues(Widget old, Widget request, Widget new, ArgList args,
                       Cardinal *num_args)
{
    ConstraintPtWidget old_box = (ConstraintPtWidget)old;
    ConstraintPtWidget new_box = (ConstraintPtWidget)new;
    Arg                 title_args[3];
    int                 title_n = 0;
    Arg                 extra_args[3];
    int                 extra_n = 0;
    Boolean             old_state;

#define NotEqual(field) (old_box->field != new_box->field)

    if ( NotEqual(constraint_pt.menu_bitmap) &&
         ! new_box->constraint_pt.title_is_menu )
    {
        XtSetArg(title_args[title_n], XtNleftBitmap,
                 new_box->constraint_pt.menu_bitmap);   title_n++;
    }

    if ( NotEqual(constraint_pt.label) )
    {
        new_box->constraint_pt.label =
            XtNewString(new_box->constraint_pt.label);
        XtFree(old_box->constraint_pt.label);
        XtVaSetValues(new_box->composite.children[1],
                      XtNlabel, new_box->constraint_pt.label, NULL);
    }

    if ( NotEqual(constraint_pt.title) )
    {
        new_box->constraint_pt.title =
            XtNewString(new_box->constraint_pt.title);
        XtFree(old_box->constraint_pt.title);
        XtSetArg(title_args[title_n], XtNlabel, new_box->constraint_pt.title);
        title_n++;
    }
    if ( NotEqual(constraint_pt.title_menu_name) )
    {
        XtSetArg(title_args[title_n], XtNmenuName,
                 new_box->constraint_pt.title_menu_name);   title_n++;
    }
    if ( title_n )
        XtSetValues( new_box->composite.children[0], title_args, title_n);

    if ( new_box->constraint_pt.toggle_required )
    {
        XtVaGetValues(new_box->composite.children[2], XtNstate, &old_state,
                      NULL);
        if ( new_box->constraint_pt.toggle_state != old_state )
            XtVaSetValues(new_box->composite.children[2],
                          XtNstate, new_box->constraint_pt.toggle_state, NULL);
    }

    if ( new_box->constraint_pt.have_extra_toggle )
    {
        if ( NotEqual(constraint_pt.extra_label) )
        {
            new_box->constraint_pt.extra_label =
                XtNewString(new_box->constraint_pt.extra_label);
            XtFree(old_box->constraint_pt.extra_label);
            XtSetArg(extra_args[extra_n], XtNlabel,
                     new_box->constraint_pt.extra_label);   extra_n++;
        }
        XtVaGetValues(new_box->composite.children[3], XtNstate, &old_state,
                      NULL);
        if ( new_box->constraint_pt.extra_state != old_state )
        {
            XtSetArg(extra_args[extra_n], XtNstate,
                     new_box->constraint_pt.extra_state);   extra_n++;
        }
        if ( extra_n )
            XtSetValues( new_box->composite.children[3], extra_args, extra_n);
    }

    if ( NotEqual(constraint_pt.h_space) )
    {
        int width, height;

        /* Calculate the new size and set new width/height fields. */
        ConstraintPtQueryPreferredSizes(new_box, &width, &height);
        new_box->core.width = width;
        new_box->core.height = height;
    }

    return FALSE;
}



static XtGeometryResult
ConstraintPtGeometryManager(Widget w, XtWidgetGeometry *request,
                             XtWidgetGeometry *result)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)XtParent(w);

    /* Refuse x / y requests. */
    if ( request->request_mode & ( CWX | CWY ) )
        return XtGeometryNo;

    /* Check for the query flag. */
    if ( ! ( request->request_mode & XtCWQueryOnly ) )
    {
        /* Set the child's fields. */
        if ( request->request_mode & CWWidth )
            w->core.width = request->width;
        if ( request->request_mode & CWHeight )
            w->core.height = request->height;

        /* Redo the layout, getting a new size from the parent. */
        ConstraintPtLayout(box, TRUE);
    }

    return XtGeometryYes;
}


static XtGeometryResult
ConstraintPtQueryGeometry(Widget w, XtWidgetGeometry *request,
                           XtWidgetGeometry *preferred)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)w;
    int width, height;

    ConstraintPtQueryPreferredSizes(box, &width, &height);
    preferred->request_mode = CWWidth | CWHeight;
    preferred->width = width;
    preferred->height = height;

    if ( ( ( request->request_mode & ( CWWidth | CWHeight ) ) ==
           ( CWWidth | CWHeight ) ) &&
         ( request->width == preferred->width ) &&
         ( request->height == preferred->height ) )
        return XtGeometryYes;
    else if ( preferred->width == w->core.width &&
              preferred->height == w->core.height )
        return XtGeometryNo;
    else
        return XtGeometryAlmost;
}

void
ConstraintPt_Change_Radio_Group(Widget w, Widget radio)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)w;
    ConstraintPtWidget  target = (ConstraintPtWidget)radio;

    XawToggleChangeRadioGroup(box->composite.children[2],
                              target->composite.children[2]);
}

void
ConstraintPt_Query_Geometry(Widget w, ConstraintPtGeometry *geometry)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)w;

    ConstraintPtQueryPreferredSizes(box, &(geometry->total_width),
                                    &(geometry->height));
    geometry->title_width = box->composite.children[0]->core.width;
    geometry->label_width = box->composite.children[1]->core.width;
    if ( box->constraint_pt.toggle_required )
        geometry->toggle_width = box->composite.children[2]->core.width;
    else
        geometry->toggle_width = 0;
    if ( box->constraint_pt.have_extra_toggle )
        geometry->extra_width = box->composite.children[3]->core.width;
    else
        geometry->extra_width = 0;
}

void
ConstraintPt_Set_Geometry(Widget w, ConstraintPtGeometry *geometry)
{
    ConstraintPtWidget  box = (ConstraintPtWidget)w;
    XtWidgetGeometry    req;
    int temp_x;
    int max_height = 0;

    temp_x = box->constraint_pt.h_space;
    /* Set the size of all the children. */
    XtConfigureWidget(box->composite.children[0],
                      temp_x, 0,
                      box->composite.children[0]->core.width,
                      box->composite.children[0]->core.height,
                      box->composite.children[0]->core.border_width);
    temp_x += geometry->title_width +
              2 * box->composite.children[0]->core.border_width +
              box->constraint_pt.h_space;
    if ( box->composite.children[0]->core.height > max_height )
        max_height = box->composite.children[0]->core.height;
    XtConfigureWidget(box->composite.children[1],
                      temp_x, 0,
                      box->composite.children[1]->core.width,
                      box->composite.children[1]->core.height,
                      box->composite.children[1]->core.border_width);
    temp_x += geometry->label_width +
              2 * box->composite.children[1]->core.border_width +
              box->constraint_pt.h_space;
    if ( box->composite.children[1]->core.height > max_height )
        max_height = box->composite.children[1]->core.height;
    if ( box->constraint_pt.toggle_required )
    {
        XtConfigureWidget(box->composite.children[2],
                          temp_x, 0,
                          box->composite.children[2]->core.width,
                          box->composite.children[2]->core.height,
                          box->composite.children[2]->core.border_width);
        temp_x += geometry->toggle_width +
                  2 * box->composite.children[2]->core.border_width +
                  box->constraint_pt.h_space;
        if ( box->composite.children[2]->core.height > max_height )
            max_height = box->composite.children[2]->core.height;
    }
    if ( box->constraint_pt.have_extra_toggle )
    {
        XtConfigureWidget(box->composite.children[3],
                          temp_x, 0,
                          box->composite.children[3]->core.width,
                          box->composite.children[3]->core.height,
                          box->composite.children[3]->core.border_width);
        temp_x += geometry->extra_width +
                  2 * box->composite.children[3]->core.border_width +
                  box->constraint_pt.h_space;
        if ( box->composite.children[3]->core.height > max_height )
            max_height = box->composite.children[3]->core.height;
    }

    /* Request size from parent. */
    req.request_mode = CWWidth | CWHeight;
    req.width = temp_x;
    req.height = max_height;

    if ( XtMakeGeometryRequest((Widget)box,&req,&req) == XtGeometryAlmost )
        XtMakeGeometryRequest((Widget)box, &req, NULL);
}

