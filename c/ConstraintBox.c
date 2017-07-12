/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/ConstraintBox.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: ConstraintBox.c,v $
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
**  ConstraintBox.c: Source code for the constraintBoxWidgetClass.
*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <ConstraintBoxP.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/ToggleP.h>
#include <stdio.h>


static void     ConstraintBoxInitialise(Widget, Widget, ArgList, Cardinal*);
static void     ConstraintBoxResize(Widget);
static void     ConstraintBoxRedisplay(Widget, XEvent*, Region);
static Boolean  ConstraintBoxSetValues(Widget, Widget, Widget, ArgList,
                                       Cardinal*);
static XtGeometryResult ConstraintBoxGeometryManager(Widget, XtWidgetGeometry*,
                                                     XtWidgetGeometry*);
static XtGeometryResult ConstraintBoxQueryGeometry(Widget, XtWidgetGeometry*,
                                                   XtWidgetGeometry*);
static void     ConstraintBoxChangeManaged(Widget);
static void     ConstraintBoxInsertChild(Widget);
static void     ConstraintBoxDeleteChild(Widget);


#define offset(field) XtOffsetOf(ConstraintBoxRec, constraint_box.field)

static XtResource   resources[] = {
    { XtNhSpace, XtCHSpace, XtRInt, sizeof(int),
      offset(h_space), XtRImmediate, (XtPointer)4 },
    { XtNvSpace, XtCVSpace, XtRInt, sizeof(int),
      offset(v_space), XtRImmediate, (XtPointer)4 },
#if ( XtSpecificationRelease > 4 )
    { XtNleftBitmap, XtCBitmap, XtRBitmap, sizeof(Pixmap),
      offset(menu_bitmap), XtRPixmap, None },
#endif
    { XtNnewCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(new_callback), XtRCallback, NULL },
    { XtNnewIsMenu, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(new_is_menu), XtRImmediate, (XtPointer)TRUE },
    { XtNnewLabel, XtCString, XtRString, sizeof(String),
      offset(new_label), XtRImmediate, (XtPointer)"New" },
    { XtNnewMenuName, XtCString, XtRString, sizeof(String),
      offset(new_menu_name), XtRImmediate, (XtPointer)"newMenu" },
    { XtNmodifyCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(modify_callback), XtRCallback, NULL },
    { XtNmodifyLabel, XtCString, XtRString, sizeof(String),
      offset(modify_label), XtRImmediate, (XtPointer)"Modify" },
    { XtNstateCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(state_callback), XtRCallback, NULL },
    { XtNtitleCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(title_callback), XtRCallback, NULL },
    { XtNtitleIsMenu, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(title_is_menu), XtRImmediate, (XtPointer)FALSE },
    { XtNtitleIsCommand, XtCBoolean, XtRBoolean, sizeof(Boolean),
      offset(title_is_command), XtRImmediate, (XtPointer)FALSE },
    { XtNtitleLabel, XtCString, XtRString, sizeof(String),
      offset(title_label), XtRImmediate, (XtPointer)"Constraints" },
    { XtNtitleMenuName, XtCString, XtRString, sizeof(String),
      offset(title_menu_name), XtRString, NULL },
    { XtNforeground, XtCColor, XtRPixel, sizeof(Pixel),
      offset(foreground), XtRString, XtDefaultForeground }
    };

#undef offset


/* Class record struct. */
ConstraintBoxClassRec   constraintBoxClassRec = {
    { /* Core fields. */
      /* superclass       */  (WidgetClass) &compositeClassRec,
      /* class_name       */  "ConstraintBox",
      /* widget_size      */  sizeof(ConstraintBoxRec),
      /* class_initialize */  NULL,
      /* class_part_initialize    */  NULL,
      /* class_inited     */  FALSE,
      /* initialize       */  ConstraintBoxInitialise,
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
      /* resize           */  ConstraintBoxResize,
      /* expose           */  ConstraintBoxRedisplay,
      /* set_values       */  ConstraintBoxSetValues,
      /* set_values_hook  */  NULL,
      /* set_values_almost*/  XtInheritSetValuesAlmost,
      /* get_values_hook  */  NULL,
      /* accept_focus     */  NULL,
      /* version          */  XtVersion,
      /* callback_private */  NULL,
      /* tm_table         */  NULL,
      /* query_geometry   */  ConstraintBoxQueryGeometry,  
      /* display_accelerator  */  XtInheritDisplayAccelerator,
      /* extension        */  NULL
    },
    { /* Composite fields.*/
      /* geometry manager.*/  ConstraintBoxGeometryManager,
      /* change managed.  */  ConstraintBoxChangeManaged,
      /* insert child.    */  ConstraintBoxInsertChild,
      /* delete child.    */  ConstraintBoxDeleteChild,
      /* extension.       */  NULL
    },
    { 0 }
    };

WidgetClass constraintBoxWidgetClass = (WidgetClass)&constraintBoxClassRec;

/* New menu translations. */
static char new_menu_trans[] = "<EnterWindow>: highlight() \n\
                                <LeaveWindow>: reset() \n\
                                <BtnDown>:  set() notify() reset() PopupMenu()";

/* Global used to get insert position. */
static Cardinal insert_pos;

static Cardinal
ConstraintBoxOrderProc(Widget w)
{
    return insert_pos;
}


/* This function is invoked as the callback for the title button, if it's
** a command widget.
*/
static void
ConstraintBoxTitleCallback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ((ConstraintBoxWidget)XtParent(w))->constraint_box.title_callback )
        XtCallCallbackList(XtParent(w),
            ((ConstraintBoxWidget)XtParent(w))->constraint_box.title_callback,
            NULL);
}


/* This function is invoked as the callback for the toggles.
*/
static void
ConstraintBoxToggleCallback(Widget w, XtPointer cl, XtPointer ca)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)XtParent(w);
    ConstraintBoxData   data;

    data.state = ca ? TRUE : FALSE;
    for ( data.index = 0 ;
          data.index < box->composite.num_children &&
          box->composite.children[data.index] != w ;
          data.index++ );
    data.index--;

    if ( box->constraint_box.state_callback )
        XtCallCallbackList((Widget)box, box->constraint_box.state_callback,
                           (XtPointer)&data);
}


/* This function is invoked as the callback for the modify button, if it's
** a command widget.
*/
static void
ConstraintBoxModifyCallback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ((ConstraintBoxWidget)XtParent(w))->constraint_box.modify_callback )
        XtCallCallbackList(XtParent(w),
            ((ConstraintBoxWidget)XtParent(w))->constraint_box.modify_callback,
            NULL);
}


/* This function is invoked as the callback for the new button, if it's
** a command widget or menu.
*/
static void
ConstraintBoxNewCallback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ((ConstraintBoxWidget)XtParent(w))->constraint_box.new_callback )
        XtCallCallbackList(XtParent(w),
            ((ConstraintBoxWidget)XtParent(w))->constraint_box.new_callback,
            NULL);
}


/*  Returns the box's preferred geometry. */
static void
ConstraintBoxQueryPreferredSizes(ConstraintBoxWidget box, int *width,
                                 int *height, int *child_width)
{
    int max_width, max_border;
    int i;

    /* Calculate the new width and height, based on the children. */
    /* Then set the widths of all the children to match the widest. */
    max_width = max_border = 0;
    *height = box->constraint_box.v_space;
    for ( i = 0 ; i < box->composite.num_children ; i++ )
    {
        if ( ! XtIsManaged(box->composite.children[i]) )
            continue;
        *height += box->composite.children[i]->core.height +
                   box->constraint_box.v_space +
                   2 * box->composite.children[i]->core.border_width;
        if ( box->composite.children[i]->core.width > max_width )
            max_width = box->composite.children[i]->core.width;
        if ( box->composite.children[i]->core.border_width > max_border )
            max_border = box->composite.children[i]->core.border_width;
    }

    *height += 4;
    *width = max_width + box->constraint_box.h_space * 2 + 2 * max_border + 4;
    *child_width = max_width;
}


/* Do a layout, repositioning children. */
/* If exclude is non null, do not touch that child. This is for
** geometry management.
** If request, request a new geometry for the widget.
*/
static void
ConstraintBoxLayout(ConstraintBoxWidget box, Widget exclude, Boolean request)
{
    int height, temp_h;
    int width;
    int max_width;
    int i;

    /* Reposition/Resize children. */
    ConstraintBoxQueryPreferredSizes(box, &width, &height, &max_width);

    if ( request )
        max_width = width - 2 * box->constraint_box.h_space - 4;
    else
        max_width = box->core.width - 2 * box->constraint_box.h_space - 4;

    /* Set the size of all the children. */
    for ( i = 0, temp_h = box->constraint_box.v_space ;
          i < box->composite.num_children ;
          i++ )
    {
        if ( ! XtIsManaged(box->composite.children[i]) )
            continue;
        if ( exclude != box->composite.children[i] )
            XtConfigureWidget(box->composite.children[i],
                              box->constraint_box.h_space + 2,
                              temp_h,
                              max_width - 2,
                              box->composite.children[i]->core.height,
                              box->composite.children[i]->core.border_width);
        temp_h += box->composite.children[i]->core.height +
                  box->constraint_box.v_space +
                  2 * box->composite.children[i]->core.border_width;
        if ( i == 0 )
        {
            box->constraint_box.box_min.y = temp_h;
            temp_h += 2;
        }
        if ( i == box->composite.num_children - 3 )
        {
            box->constraint_box.box_max.y = temp_h -
                                            box->constraint_box.v_space + 2;
            temp_h += 2;
        }
    }
    box->constraint_box.box_min.x = box->constraint_box.h_space;
    box->constraint_box.box_max.x = box->constraint_box.box_min.x + max_width +
                                    3;

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
ConstraintBoxInitialise(Widget request, Widget new, ArgList orig_args,
                        Cardinal *num_args)
{   
    ConstraintBoxWidget box = (ConstraintBoxWidget)new;
    Widget              title, new_w, ret_w;
    XGCValues           gc_vals;
    Arg                 args[10];
    int                 n;

    int height;
    int width;
    int dummy;

    /* Set the insert_position field of the superclass. */
    box->composite.insert_position = ConstraintBoxOrderProc;

    /* Set the GC. */
    gc_vals.foreground = box->constraint_box.foreground;
    box->constraint_box.box_gc = XtGetGC(new, GCForeground, &gc_vals);

    /* Create children - title, new and modify. */
    insert_pos = 0;
    box->constraint_box.title_label =
        XtNewString(box->constraint_box.title_label);
    n = 0;
    XtSetArg(args[n], XtNlabel, box->constraint_box.title_label);   n++;
    if ( box->constraint_box.title_is_menu )
    {
        XtSetArg(args[n], XtNmenuName, box->constraint_box.title_menu_name);n++;
#if ( XtSpecificationRelease > 4 )
        if ( box->constraint_box.menu_bitmap )
        {
            XtSetArg(args[n], XtNleftBitmap, box->constraint_box.menu_bitmap);
            n++;
        }
#endif
        title = XtCreateManagedWidget("title", menuButtonWidgetClass, new,
                                      args, n);
    }
    else if ( box->constraint_box.title_is_command )
    {
        title = XtCreateManagedWidget("title", commandWidgetClass, new, args,n);
        XtAddCallback(title, XtNcallback, ConstraintBoxTitleCallback, NULL);
    }
    else
    {
        XtSetArg(args[n], XtNborderWidth, 0);   n++;
        title = XtCreateManagedWidget("title", labelWidgetClass, new, args, n);
    }

    insert_pos = 1;
    n = 0;
    box->constraint_box.new_label = XtNewString(box->constraint_box.new_label);
    XtSetArg(args[n], XtNlabel, box->constraint_box.new_label); n++;
    if ( box->constraint_box.new_is_menu )
    {
        XtSetArg(args[n], XtNmenuName, box->constraint_box.new_menu_name);  n++;
#if ( XtSpecificationRelease > 4 )
        if ( box->constraint_box.menu_bitmap )
        {
            XtSetArg(args[n], XtNleftBitmap, box->constraint_box.menu_bitmap);
            n++;
        }
#endif
        new_w = XtCreateManagedWidget("new", menuButtonWidgetClass, new, args,
                                      n);
        XtOverrideTranslations(new_w, XtParseTranslationTable(new_menu_trans));
    }
    else
        new_w = XtCreateManagedWidget("new", commandWidgetClass, new, args,n);
    XtAddCallback(new_w, XtNcallback, ConstraintBoxNewCallback, NULL);

    insert_pos = 2;
    box->constraint_box.modify_label =
        XtNewString(box->constraint_box.modify_label);
    n = 0;
    XtSetArg(args[n], XtNlabel, box->constraint_box.modify_label);  n++;
    ret_w = XtCreateManagedWidget("modify", commandWidgetClass, new, args,n);
    XtAddCallback(ret_w, XtNcallback, ConstraintBoxModifyCallback, NULL);

    /* Calculate the geometry. */
    ConstraintBoxQueryPreferredSizes(box, &width, &height, &dummy);
    box->core.width = width;
    box->core.height = height;
    ConstraintBoxLayout(box, NULL, FALSE);
}

static void
ConstraintBoxResize(Widget w)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;

    ConstraintBoxLayout(box, NULL, FALSE);
}


static void
ConstraintBoxRedisplay(Widget w, XEvent *event, Region reg)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;

    /* Draw a box around the constraints. */
    XDrawRectangle(XtDisplay(w), XtWindow(w), box->constraint_box.box_gc,
                 box->constraint_box.box_min.x, box->constraint_box.box_min.y,
                 box->constraint_box.box_max.x - box->constraint_box.box_min.x,
                 box->constraint_box.box_max.y - box->constraint_box.box_min.y);
}

static Boolean
ConstraintBoxSetValues(Widget old, Widget request, Widget new, ArgList args,
                       Cardinal *num_args)
{
    ConstraintBoxWidget old_box = (ConstraintBoxWidget)old;
    ConstraintBoxWidget new_box = (ConstraintBoxWidget)new;
    Arg                 new_args[3];
    int                 new_n = 0;
    Arg                 title_args[3];
    int                 title_n = 0;
    Widget              new_widg;

#define NotEqual(field) (old_box->field != new_box->field)

#if ( XtSpecificationRelease > 4 )
    if ( NotEqual(constraint_box.menu_bitmap) )
    {
        if ( ! new_box->constraint_box.new_is_menu )
        {
            XtSetArg(new_args[new_n], XtNleftBitmap,
                     new_box->constraint_box.menu_bitmap);  new_n++;
        }
        if ( ! new_box->constraint_box.title_is_menu )
        {
            XtSetArg(title_args[title_n], XtNleftBitmap,
                     new_box->constraint_box.menu_bitmap);  title_n++;
        }
    }
#endif

    if ( NotEqual(constraint_box.new_is_menu) )
    {
        /* Destroy the old one. */
        XtDestroyWidget(
            new_box->composite.children[new_box->composite.num_children - 2]);

        /* Create the new one. */
        insert_pos = new_box->composite.num_children - 1;
        new_n = 0;
        XtSetArg(new_args[new_n], XtNlabel, new_box->constraint_box.new_label);
        new_n++;
        if ( new_box->constraint_box.new_is_menu )
        {
            Widget  new_w;

            XtSetArg(new_args[new_n], XtNmenuName,
                     new_box->constraint_box.new_menu_name);    new_n++;
#if ( XtSpecificationRelease > 4 )
            XtSetArg(new_args[new_n], XtNleftBitmap,
                     new_box->constraint_box.menu_bitmap);      new_n++;
#endif
            new_w = XtCreateManagedWidget("new", menuButtonWidgetClass,
                                (Widget)new_box, new_args, new_n);
            XtOverrideTranslations(new_w,
                                   XtParseTranslationTable(new_menu_trans));
        }
        else
            new_widg = XtCreateManagedWidget("new", commandWidgetClass,
                                             (Widget)new_box, new_args, new_n);
        XtAddCallbacks(new_widg, XtNcallback,
                       new_box->constraint_box.new_callback);
        new_n = 0;
    }
    else
    {
        if ( NotEqual(constraint_box.new_label) )
        {
            new_box->constraint_box.new_label =
                XtNewString(new_box->constraint_box.new_label);
            XtFree(old_box->constraint_box.new_label);
            XtSetArg(new_args[new_n], XtNlabel,
                     new_box->constraint_box.new_label);    new_n++;
        }
        if ( NotEqual(constraint_box.new_menu_name) )
        {
            XtSetArg(new_args[new_n], XtNmenuName,
                     new_box->constraint_box.new_menu_name);    new_n++;
        }
        if ( new_n )
            XtSetValues(
             new_box->composite.children[new_box->composite.num_children - 2],
             new_args, new_n);
    }

    if ( NotEqual(constraint_box.modify_label) )
    {
        new_box->constraint_box.modify_label =
            XtNewString(new_box->constraint_box.modify_label);
        XtFree(old_box->constraint_box.modify_label);
        XtVaSetValues(
            new_box->composite.children[new_box->composite.num_children - 1],
            XtNlabel, new_box->constraint_box.modify_label, NULL);
    }

    if ( NotEqual(constraint_box.title_is_menu) ||
         NotEqual(constraint_box.title_is_command) )
    {
        /* Destroy the old one. */
        XtDestroyWidget(new_box->composite.children[0]);

        insert_pos = 0;
        title_n = 0;
        XtSetArg(
            title_args[title_n], XtNlabel, new_box->constraint_box.title_label);
        title_n++;
        if ( new_box->constraint_box.title_is_menu )
        {
            XtSetArg(title_args[title_n], XtNmenuName,
                     new_box->constraint_box.title_menu_name);  title_n++;
#if ( XtSpecificationRelease > 4 )
            XtSetArg(title_args[title_n], XtNleftBitmap,
                     new_box->constraint_box.menu_bitmap);      title_n++;
#endif
            XtCreateManagedWidget("title", menuButtonWidgetClass,
                                  (Widget)new_box, title_args, title_n);
        }
        else if ( new_box->constraint_box.title_is_command )
        {
            new_widg = XtCreateManagedWidget("title", commandWidgetClass,
                                    (Widget)new_box, title_args, title_n);
            XtAddCallbacks(new_widg, XtNcallback,
                           new_box->constraint_box.title_callback);
        }
        else
        {
            XtSetArg(title_args[title_n], XtNborderWidth, 0);   title_n++;
            XtCreateManagedWidget("title", labelWidgetClass, (Widget)new_box,
                                  title_args, title_n);
        }
    }
    else
    {
        if ( NotEqual(constraint_box.title_label) )
        {
            new_box->constraint_box.title_label =
                XtNewString(new_box->constraint_box.title_label);
            XtFree(old_box->constraint_box.title_label);
            XtSetArg(title_args[title_n], XtNlabel,
                     new_box->constraint_box.title_label);  title_n++;
        }
        if ( NotEqual(constraint_box.title_menu_name) )
        {
            XtSetArg(title_args[title_n], XtNmenuName,
                     new_box->constraint_box.title_menu_name);  title_n++;
        }
        if ( title_n )
            XtSetValues( new_box->composite.children[0], title_args, title_n);
    }

    if ( NotEqual(constraint_box.h_space) || NotEqual(constraint_box.v_space) )
    {
        int width, height, dummy;

        /* Calculate the new size and set new width/height fields. */
        ConstraintBoxQueryPreferredSizes(new_box, &width, &height, &dummy);
        new_box->core.width = width;
        new_box->core.height = height;
    }

    return FALSE;
}



static XtGeometryResult
ConstraintBoxGeometryManager(Widget w, XtWidgetGeometry *request,
                             XtWidgetGeometry *result)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)XtParent(w);

    /* Refuse x / y requests. */
    if ( request->request_mode & ( CWX | CWY ) )
        return XtGeometryNo;

    /* Grant the child's request, and ask for a resize from the parent. */
    /* Check that the width isn't smaller than the parent's width. */
    if ( ( request->request_mode & CWWidth ) &&
         request->width < box->core.width - box->constraint_box.v_space * 2 )
        return XtGeometryNo;

    /* Check for the query flag. */
    if ( ! ( request->request_mode & XtCWQueryOnly ) )
    {
        /* Set the child's fields. */
        if ( request->request_mode & CWWidth )
            w->core.width = request->width;
        if ( request->request_mode & CWHeight )
            w->core.height = request->height;

        /* Redo the layout. */
        ConstraintBoxLayout(box, w, TRUE);
    }

    return XtGeometryYes;
}


static XtGeometryResult
ConstraintBoxQueryGeometry(Widget w, XtWidgetGeometry *request,
                           XtWidgetGeometry *preferred)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;
    int width, height, dummy;

    ConstraintBoxQueryPreferredSizes(box, &width, &height, &dummy);
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

static void
ConstraintBoxChangeManaged(Widget w)
{
    ConstraintBoxLayout((ConstraintBoxWidget)w, NULL, TRUE);
}


static void
ConstraintBoxInsertChild(Widget w)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)XtParent(w);

    (*((CompositeWidgetClass)XtSuperclass((Widget)box))
        ->composite_class.insert_child)(w);

    if ( box->composite.num_children > 3 )
        ConstraintBoxLayout(box, NULL, TRUE);
}


static void
ConstraintBoxDeleteChild(Widget w)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)XtParent(w);

    (*((CompositeWidgetClass)XtSuperclass((Widget)box))
        ->composite_class.delete_child)(w);

    ConstraintBoxLayout(box, NULL, TRUE);
}


void
ConstraintBoxSetConstraints(Widget w, int num, String *labels, Boolean *states)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;
    Widget              toggle;
    int                 i;

    if ( num > box->composite.num_children - 3 )
    {
        /* Need more. */
        for ( i = box->composite.num_children - 3 ; i < num ; i++ )
        {
            insert_pos = i + 1;
            toggle =
                XtCreateManagedWidget("toggle", toggleWidgetClass, w, NULL, 0);
            XtAddCallback(toggle, XtNcallback, ConstraintBoxToggleCallback,
                NULL);
        }
    }
    else if ( num < box->composite.num_children - 3 )
    {
        /* Get rid of some. */
        for ( i = box->composite.num_children - 3 ; i > num ; i-- )
            XtDestroyWidget(box->composite.children[i]);
    }

    /* Now have all the widgets. */
    /* Set args. */
    for ( i = 1 ; i <= num ; i++ )
        XtVaSetValues(box->composite.children[i], XtNlabel, labels[i-1],
                      XtNstate, states[i-1], NULL);
}


void
ConstraintBoxAddConstraint(Widget w, String label, Boolean state, int index)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;
    Widget              toggle;
    Arg                 args[3];
    int                 n;

    if ( index == -1 )
        index = box->composite.num_children - 3;
    if ( index > box->composite.num_children - 3 )
    {
        fprintf(stderr, "Warning: Index out of range: %d\n", index);
        return;
    }
    insert_pos = index + 1;

    n = 0;
    XtSetArg(args[n], XtNlabel, label);                 n++;
    XtSetArg(args[n], XtNstate, state);                 n++;
    toggle = XtCreateManagedWidget("toggle", toggleWidgetClass, w, args, n);
    XtAddCallback(toggle, XtNcallback, ConstraintBoxToggleCallback, NULL);
}


void
ConstraintBoxRemoveConstraint(Widget w, int index)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;

    if ( index < 0 || index >= box->composite.num_children - 3 )
    {
        fprintf(stderr, "Warning: Index out of range: %d\n", index);
        return;
    }
    XtDestroyWidget(box->composite.children[index + 1]);
}


void
ConstraintBoxSetConstraintState(Widget w, int index, Boolean state,
                                Boolean notify)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;
    ConstraintBoxData   data;

    if ( index < 0 || index >= box->composite.num_children - 3 )
    {
        fprintf(stderr, "Warning: Index out of range: %d\n", index);
        return;
    }
    XtVaSetValues(box->composite.children[index + 1], XtNstate, state, NULL);

    if ( notify )
    {
        data.state = state;
        data.index = index;
        if ( box->constraint_box.state_callback )
            XtCallCallbackList((Widget)box, box->constraint_box.state_callback,
                               (XtPointer)&data);
    }
}


Boolean
ConstraintBoxGetState(Widget w, int index)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;
    Boolean             result;

    if ( index < 0 || index >= box->composite.num_children - 3 )
    {
        fprintf(stderr, "Warning: Index out of range: %d\n", index);
        return FALSE;
    }
    XtVaGetValues(box->composite.children[index - 1], XtNstate, &result, NULL);
    return result;
}


void
ConstraintBoxSetConstraintLabel(Widget w, int index, char *name)
{
    ConstraintBoxWidget box = (ConstraintBoxWidget)w;

    if ( index < 0 || index >= box->composite.num_children - 3 )
    {
        fprintf(stderr, "Warning: Index out of range: %d\n", index);
        return;
    }
    XtVaSetValues(box->composite.children[index - 1], XtNlabel, name, NULL);
}

