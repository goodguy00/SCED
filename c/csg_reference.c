/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/csg_reference.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: csg_reference.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
 * Initial revision
 *
 */
#define PATCHLEVEL 0
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
**  sced: A Constraint Based Object Scene Editor
**
**  csg_reference.c: Code for the dialog that allows the reference point for
**                  a csg object to be selected.
*/

#include <math.h>
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#include <select_point.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <View.h>

#define CANCEL -1
#define CONTINUE -2

static String   select_string =
        "<BtnDown> : Highlight_Object()\n"
        "<PtrMoved> : Highlight_Closest()\n"
        "<BtnUp> : Select_Point()";

static int  Init_CSG_Reference(Boolean, Boolean);
static void Cleanup_CSG_Reference();
static void Create_CSG_Reference_Shell();

Boolean         *target_full;
WireframePtr    *target;
CSGNodePtr      target_obj;

static Widget   csg_reference_shell = NULL;
static Widget   ref_view;
static Widget   full_button;
static Widget   cancel_button;

static Vector   *ref_worlds;
static Vertex   *ref_verts;
static Vector   *ref_normals;
static GC       ref_gc;
static GC       ref_hidden_gc;
static GC       ref_clear_gc;
static InstanceListElmt ref_inst;

static Viewport     csg_ref_viewport;
static WindowInfo   csg_ref_window;

static int  finished;

static Widget   progress_shell = NULL;
static Widget   progress_label = NULL;
static Boolean  progress_up = FALSE;


/*  void
**  Select_CSG_Base_Reference(BaseObjectPtr obj)
**  Instigates the selection of the reference point for a csg base object.
*/
int
Select_Base_Reference(WireframePtr *wire, CSGNodePtr tree, Boolean *use_full)
{
    if ( ! csg_reference_shell )
        Create_CSG_Reference_Shell();

    target_full = use_full;
    target_obj = tree;
    target = wire;

    return Init_CSG_Reference(TRUE, (use_full ? TRUE : FALSE));
}


static int
CSG_Reference_Select_Callback(ConstraintSpecPtr spec)
{
    int     index;
    Vector  diff;

    for ( index = 0 ;
          ! VEqual((*target)->vertices[index], spec->spec_vector, diff) ;
          index++ );

    Cleanup_CSG_Reference();

    XtPopdown(csg_reference_shell);

    finished = index;

    return IGNORE;
}


static void
CSG_Full_Wireframe_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Cleanup_CSG_Reference();

    Wireframe_Destroy(*target);

    *target = CSG_Generate_Full_Wireframe(target_obj, 0);

    *target_full = TRUE;

    Init_CSG_Reference(FALSE, FALSE);
}


static void
CSG_Cancel_Reference_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Cleanup_CSG_Reference();

    XtPopdown(csg_reference_shell);

    finished = CANCEL;
}

static void
CSG_Reference_Calc_Points()
{
    double  max_x, max_y;
    double      scale;
    Dimension   width, height;
    int     i;

    max_x = max_y = 0.0;
    for ( i = 0 ; i < (*target)->num_real_verts ; i++ )
    {
        if (fabs(ref_verts[i].view.x) > max_x)
            max_x = fabs(ref_verts[i].view.x);
        if (fabs(ref_verts[i].view.y) > max_y)
            max_y = fabs(ref_verts[i].view.y);
    }
    XtVaGetValues(ref_view, XtNwidth, &width, XtNheight, &height, NULL);
    if ( IsZero(max_x) || IsZero(max_y) )
        scale = 100.0;
    else
        scale = min((((int)width - 5 ) / (double)max_x ),
                    (((int)height - 5 ) / (double)max_y));

    Convert_View_To_Screen(ref_verts, (*target)->num_real_verts,
                        &csg_ref_viewport, (short)width, (short)height, scale);
}

static void
CSG_Reference_Draw()
{
    Dimension   width, height;
    XRectangle  area;

    XtVaGetValues(ref_view, XtNwidth, &width, XtNheight, &height, NULL);

    area.x = area.y = 0;
    area.width = (short)width;
    area.height = (short)height;

    XFillRectangles(XtDisplay(ref_view), XtWindow(ref_view), ref_clear_gc,
                    &area, 1);

    Draw_Edges(XtDisplay(ref_view), XtWindow(ref_view), ref_gc, ref_hidden_gc,
               *target, ref_verts, ref_worlds, ref_normals, &csg_ref_viewport);
    
    Select_Highlight_Closest(XtWindow(ref_view));
    Draw_Selection_Points(XtWindow(ref_view));
}

static int
Init_CSG_Reference(Boolean popup, Boolean allow_full)
{
    Cuboid  bound;
    Vector  sum;
    int     i;

    bound = Calculate_Bounds((*target)->vertices, (*target)->num_real_verts);
    VAdd(bound.min, bound.max, sum);
    VScalarMul(sum, 0.5, csg_ref_viewport.view_at);
    Build_Viewport_Transformation(&csg_ref_viewport);

    ref_worlds = New(Vector, (*target)->num_real_verts);
    for ( i = 0 ; i < (*target)->num_real_verts ; i++ )
        ref_worlds[i] = (*target)->vertices[i];
    ref_verts = New(Vertex, (*target)->num_real_verts);
    Convert_World_To_View(ref_worlds, ref_verts,
                    (*target)->num_real_verts, &csg_ref_viewport);
    ref_normals = New(Vector, (*target)->num_faces);
    for ( i = 0 ; i < (*target)->num_faces ; i++ )
        ref_normals[i] = (*target)->faces[i].normal;

    ref_inst.the_instance->o_num_vertices = (*target)->num_real_verts;
    ref_inst.the_instance->o_world_verts = ref_worlds;
    ref_inst.the_instance->o_main_verts = ref_verts;
    ref_inst.the_instance->o_wireframe = *target;
    ref_inst.the_instance->o_flags = ObjVisible;

    if ( allow_full )
    {
        *target_full = FALSE;
        XtSetSensitive(full_button, TRUE);
    }
    else
        XtSetSensitive(full_button, FALSE);

    if ( popup )
    {
        Position_Shell(csg_reference_shell, FALSE);
        XtPopup(csg_reference_shell, XtGrabExclusive);
    }

    CSG_Reference_Calc_Points();

    select_window = &csg_ref_window;
    select_highlight = FALSE;
    allow_text_entry = FALSE;
    select_callback = CSG_Reference_Select_Callback;
    select_instances = &ref_inst;
    VNew(0, 0, 0, select_center);

    Prepare_Selection_Drawing();

    CSG_Reference_Draw();

    if ( popup )
    {
        XtAppContext    context;
        XEvent          event;

        finished = CONTINUE;
        
        context = XtWidgetToApplicationContext(main_window.shell);
        while ( finished == CONTINUE )
        {
            XtAppNextEvent(context, &event);
            XtDispatchEvent(&event);
        }
    }

    return finished;
}


static void
Cleanup_CSG_Reference()
{
    Cleanup_Selection();
    free(ref_worlds);
    free(ref_verts);
    free(ref_normals);
}


static void
CSG_Reference_Redraw(Widget w, XtPointer cl, XtPointer ca)
{
    CSG_Reference_Calc_Points();
    CSG_Reference_Draw();
}


static void
CSG_Reference_GCs()
{
    Pixel       foreground, background;
    XGCValues   gc_vals; 

    XtVaGetValues(ref_view, XtNforeground, &foreground,
                            XtNbackground, &background, NULL);

    gc_vals.function = GXcopy;
    gc_vals.foreground = foreground;
    ref_gc = XtGetGC(csg_reference_shell, GCFunction|GCForeground|GCBackground,
                    &gc_vals);

    gc_vals.line_style = LineOnOffDash;
    ref_hidden_gc = XtGetGC(csg_reference_shell,
                    GCFunction|GCForeground|GCBackground|GCLineStyle, &gc_vals);

    gc_vals.function = GXcopy;
    gc_vals.foreground = background;
    ref_clear_gc = XtGetGC(csg_reference_shell,
                    GCForeground | GCFunction, &gc_vals);
}


static void
Create_CSG_Reference_Shell()
{
    Widget  form;
    Widget  label;
    Arg     args[10];
    int     n;
    Dimension   width, form_height, label_height;
    int         gap;

    /* Create the shell. */
    n = 0;
    XtSetArg(args[n], XtNtitle, "CSG Reference Shell"); n++;
    csg_reference_shell = XtCreatePopupShell("csgReferenceShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("csgReferenceForm", formWidgetClass,
                                 csg_reference_shell, NULL, 0);

    n = 0;
    XtSetArg(args[n], XtNlabel, "Select The Scaling Point");    n++;
    XtSetArg(args[n], XtNtop, XtChainTop);                      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);                   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);                    n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    label = XtCreateManagedWidget("csgReferenceLabel", labelWidgetClass,
                                  form, args, n);

    XtRealizeWidget(csg_reference_shell);

    n = 0;
    XtSetArg(args[n], XtNwidth, &width);            n++;
    XtSetArg(args[n], XtNheight, &form_height);     n++;
    XtSetArg(args[n], XtNdefaultDistance, &gap);    n++;
    XtGetValues(form, args, n);

    n = 0;
    XtSetArg(args[n], XtNheight, &label_height);    n++;
    XtGetValues(label, args, n);

    n = 0;
    XtSetArg(args[n], XtNwidth, (int)width - 2 * gap);  n++;
    XtSetArg(args[n], XtNheight, (int)form_height - (int)label_height -
                                 (int)label_height - 6 * gap);      n++;
    XtSetArg(args[n], XtNtop, XtChainTop);              n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);        n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainRight);          n++;
    XtSetArg(args[n], XtNfromVert, label);              n++;
    ref_view = XtCreateManagedWidget("csgReferenceView", viewWidgetClass,
                                     form, args, n);
    XtAddCallback(ref_view, XtNexposeCallback, CSG_Reference_Redraw, NULL);
    XtOverrideTranslations(ref_view, XtParseTranslationTable(select_string));

    /* The full wireframe command button. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Full Wireframe");  n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);       n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNfromVert, ref_view);       n++;
    full_button = XtCreateManagedWidget("csgRefButton", commandWidgetClass,
                                    form, args, n);
    XtAddCallback(full_button, XtNcallback, CSG_Full_Wireframe_Callback, NULL);

    /* The cancel command button. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);       n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNfromVert, ref_view);       n++;
    XtSetArg(args[n], XtNfromHoriz, full_button);   n++;
    cancel_button = XtCreateManagedWidget("csgRefCancelButton",
                                          commandWidgetClass, form, args, n);
    XtAddCallback(cancel_button, XtNcallback, CSG_Cancel_Reference_Callback,
                  NULL);

    /* Set up the viewport. */
    VNew(5, 4, 3, csg_ref_viewport.view_from);
    VNew(0, 0, 1, csg_ref_viewport.view_up);
    csg_ref_viewport.view_distance = 500;
    csg_ref_viewport.eye_distance = 500;

    csg_ref_window.shell = csg_reference_shell;
    csg_ref_window.view_widget = ref_view;

    /* Set up the initial instance. Needed for select point. */
    ref_inst.the_instance = New(ObjectInstance, 1);
    ref_inst.the_instance->o_parent = NULL;
    ref_inst.next = NULL;

    CSG_Reference_GCs();
}


static void
CSG_Create_Progress_Shell()
{
    Widget  form;
    Arg args[5];
    int n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Progress");        n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    progress_shell = XtCreatePopupShell("progressShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("progressForm", formWidgetClass,
                                 progress_shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNresizable, TRUE);  n++;
    progress_label = XtCreateManagedWidget("progressLabel", labelWidgetClass,
                                           form, args, n);

    XtRealizeWidget(progress_shell);
}


void
CSG_Popup_Progress(char *label)
{
    if ( ! progress_shell )
        CSG_Create_Progress_Shell();

    XtVaSetValues(progress_label, XtNlabel, label, NULL);

    Position_Shell(progress_shell, TRUE);

    XtPopup(progress_shell, XtGrabNone);

    progress_up = TRUE;
}

void
CSG_Popdown_Progress()
{
    if ( ! progress_up )
        return;
    XtPopdown(progress_shell);
}


void
CSG_Set_Progress(char *label)
{
    if ( ! progress_up )
        return;
    XtVaSetValues(progress_label, XtNlabel, label, NULL);
    XFlush(XtDisplay(progress_label));
}

