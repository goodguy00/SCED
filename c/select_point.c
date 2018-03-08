/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/select_point.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: select_point.c,v $
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
**  select_point.c : Functions to select individual vertices.
*/

#include <math.h>
#include <sced.h>
#include <bezier.h>
#include <select_point.h>
#include <torus.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>


static void Select_Calc_Highlight(XPoint);
static void Select_Apply_Callback(Widget, XtPointer, XtPointer);
static void Select_Point_Apply_Absolute(Widget, XtPointer, XtPointer);
static void Select_Apply_Create_Dialog();


/* The window to do it in. */
WindowInfoPtr   select_window;

/* The vertices to select from. */
/*
int                 num_select_verts;
SelectPointStruct   *select_verts;
*/
InstanceList    select_instances;

/* The center, used for offsets that are entered as text. */
Vector  select_center;

/* The callback to invoke when a point has been selected.   */
SelectPointCallbackProc select_callback;

/* Whether to allow text entry of points. */
Boolean allow_text_entry;

/* Whether to perform highlights and reference cycling. */
Boolean select_highlight;

/* Whether offset points are allowed from text entry. */
Boolean prompt_offset;

/* Points chosen so far, needed for redraw. */
static int              num_chosen = 0;
static XPoint           chosen_pts[6];
static ConstraintSpec   chosen_specs[6];

/* Points for higlighting closest points. */
static InstanceList highlight_inst;
static int          highlight_offset;
static double       closest_dist;

/* GCs for selected and unselected points. */
static Boolean  initialized = FALSE;
static GC       selected_gc;
static GC       control_gc;
static GC       ready_gc;
static GC       highlight_gc;
static Cursor   select_cursor;

static char orig_string[ENTRY_STRING_LENGTH];
static char orig_label[ENTRY_STRING_LENGTH];
static Vector   entered_vect;

/* For black and white. */
#define stipple_width 2
#define stipple_height 2
static char stipple_bits[] = {0x01, 0x02};
static Pixmap   stippled_pixmap = 0;

/* For text entry. */
static Widget   select_apply_dialog = NULL;

#define highlight_vert \
    highlight_inst->the_instance->o_main_verts[highlight_offset]

/*  void
**  Select_Point_Action(Widget w, XEvent *e, String *s, Cardinal *n)
**  An action procedure designed to be called for button events.  It simply
**  invokes Select_Point().
*/
void
Select_Point_Action(Widget w, XEvent *e, String *s, Cardinal *n)
{
    XPoint  pt;
    ConstraintSpecType type = absolute_spec;

    pt.x = e->xbutton.x;
    pt.y = e->xbutton.y;

    switch ( e->xbutton.button )
    {
        case Button1:
            type = reference_spec;
            break;
        case Button2:
            type = offset_spec;
            break;
        case Button3:
            type = absolute_spec;
            break;
    }

    Select_Point(pt, type);
}


static Boolean
Select_Cycle_References(Boolean ignore)
{
    static short    event_count = 0;
    Boolean         found, done;
    InstanceList    new_inst = NULL;
    int             new_offset = 0;
    InstanceList    temp_l;
    int             i;

    if ( ! select_highlight ) return FALSE;

    /* To smooth things out a bit, only process
    ** every third event. This is somewhat arbitrary.
    */
    if ( ignore && event_count++ < 3 ) return FALSE;
    event_count = 0;

    /* Find the next common point. */
    for ( temp_l = highlight_inst, found = done = FALSE, i = highlight_offset+1;
          ! done && ! found ;
          temp_l = ( temp_l->next ? temp_l->next : select_instances ), i = 0 )
    {
        if ( ( temp_l->the_instance->o_flags & ObjDepends ) ||
             ! ( temp_l->the_instance->o_flags & ObjVisible ) )
        {
            if ( temp_l == highlight_inst && i <= highlight_offset )
                done = TRUE;
            else
                continue;
        }

        for ( ; ! found && ! done && i < temp_l->the_instance->o_num_vertices ;
              i++ )
        {
            if ( highlight_vert.screen.x ==
                 temp_l->the_instance->o_main_verts[i].screen.x &&
                 highlight_vert.screen.y ==
                 temp_l->the_instance->o_main_verts[i].screen.y &&
                 temp_l->the_instance->o_main_verts[i].drawn )
            {
                new_inst = temp_l;
                new_offset = i;
                found = TRUE;
            }
            if ( temp_l == highlight_inst && i == highlight_offset )
                done = TRUE;
        }
    }

    if ( ! found ) return FALSE;

    /* Unhighlight the currently highlighted. */
    Draw_All_Edges(XtDisplay(select_window->shell),
                   XtWindow(select_window->view_widget),
                   highlight_gc,
                   highlight_inst->the_instance->o_wireframe,
                   highlight_inst->the_instance->o_main_verts,
                   &(select_window->viewport));

    highlight_inst = new_inst;
    highlight_offset = new_offset;

    /* Highlight it. */
    Draw_All_Edges(XtDisplay(select_window->shell),
                   XtWindow(select_window->view_widget),
                   highlight_gc,
                   highlight_inst->the_instance->o_wireframe,
                   highlight_inst->the_instance->o_main_verts,
                   &(select_window->viewport));

    return TRUE;
}


/*  void
**  Select_Highlight_Object(Widget w, XEvent *e, String *s, Cardinal *n)
**  Highlights the referenced object, if any.
*/
void
Select_Highlight_Object(Widget w, XEvent *e, String *s, Cardinal *n)
{
    if ( ! select_highlight || e->xbutton.button != Button1 ||
         ! highlight_inst )
        return;

    if ( ( highlight_inst->the_instance->o_flags & ObjDepends ) &&
         ! Select_Cycle_References(FALSE) )
        return;

    /* Simply draw the object referenced by the highlight point in a different
    ** colour.
    */
    Draw_All_Edges(XtDisplay(select_window->shell),
                   XtWindow(select_window->view_widget),
                   highlight_gc,
                   highlight_inst->the_instance->o_wireframe,
                   highlight_inst->the_instance->o_main_verts,
                   &(select_window->viewport));
}



/*  void
**  Select_Highlight_Action(Widget w, XEvent *e, String *s, Cardinal *n)
**  Highlights the point closest to the pointer.
*/
void
Select_Highlight_Action(Widget w, XEvent *e, String *s, Cardinal *n)
{
    XPoint      pt;
    XRectangle  rect;

    if (( e->xmotion.state & ( Button2Mask | Button3Mask )) || ! highlight_inst )
        return;

    if ( e->xmotion.state & Button1Mask )
    {
        Select_Cycle_References(TRUE);
        return;
    }

    rect.width = rect.height = sced_resources.select_pt_width;

    /* Undraw the old one. */
    rect.x = highlight_vert.screen.x - sced_resources.select_pt_width / 2;
    rect.y = highlight_vert.screen.y - sced_resources.select_pt_width / 2;
    if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
        XFillRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        ready_gc, &rect, 1);
    else
        XDrawRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        ready_gc, &rect, 1);

    pt.x = e->xmotion.x;
    pt.y = e->xmotion.y;

    Select_Calc_Highlight(pt);

    /* Draw the new one. */
    rect.x = highlight_vert.screen.x - sced_resources.select_pt_width / 2;
    rect.y = highlight_vert.screen.y - sced_resources.select_pt_width / 2;
    if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
        XFillRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        ready_gc, &rect, 1);
    else
        XDrawRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        ready_gc, &rect, 1);
}


/*  Calculates the closest point for highlighting. Doesn't actually draw it.
*/
void
Select_Highlight_Closest(Window window)
{
    int             x, y;
    Window          root, child;
    int             dummyx, dummyy;
    unsigned int    dummymask;
    XPoint          pt;

    XQueryPointer(XtDisplay(select_window->view_widget), window,
                  &root, &child, &dummyx, &dummyy, &x, &y, &dummymask);

    pt.x = x; pt.y = y;

    Select_Calc_Highlight(pt);
}


/*  I lied. This one actually does the calculation. */
/*  Sets the globals highlight_inst and highlight_offset for the closest pt.
*/
static void
Select_Calc_Highlight(XPoint pt)
{
    double  x_dif, y_dif;
    double  temp_dist;
    InstanceList    temp_l = select_instances;
    int             i;

#define select_vert(o, i)   o->the_instance->o_main_verts[i].screen

    closest_dist = HUGE_VAL;
    highlight_inst = NULL;
    highlight_offset = 0;

    for ( ; temp_l ; temp_l = temp_l->next )
    {
        if ( ! ( temp_l->the_instance->o_flags & ObjVisible ) )
            continue;

        for ( i = 0 ; i < temp_l->the_instance->o_num_vertices ; i++ )
        {
            if ( ! temp_l->the_instance->o_main_verts[i].drawn )
                continue;

            x_dif = pt.x - select_vert(temp_l, i).x;
            y_dif = pt.y - select_vert(temp_l, i).y;
            temp_dist = x_dif * x_dif + y_dif * y_dif;
            if ( temp_dist < closest_dist )
            {
                closest_dist = temp_dist;
                highlight_inst = temp_l;
                highlight_offset = i;
            }
        }
    }
}


/*  void
**  Select_Point(XPoint pt, ConstraintSpecType nature)
**  Called to select a point.  Checks pt against all the available point
**  rectangles and if it matches, adds that point to those selected.
*/
void
Select_Point(XPoint pt, ConstraintSpecType nature)
{
    ConstraintSpec  selected_point;
    ObjectInstancePtr   obj;
    XPoint      min, max;
    XRectangle  rect;
    int         action;
    int         i;
    int         temp_i = ( sced_resources.select_pt_width +
                           sced_resources.select_pt_line_width ) / 2;

    if ( ! highlight_inst )
        return;

    obj = highlight_inst->the_instance;

    if ( select_highlight && Spec_Is_Dependent(nature) )
        Draw_All_Edges(XtDisplay(select_window->shell),
                       XtWindow(select_window->view_widget),
                       highlight_gc,
                       highlight_inst->the_instance->o_wireframe,
                       highlight_inst->the_instance->o_main_verts,
                       &(select_window->viewport));

    /* Check the click was inside the highlight pt. */
    min.x = highlight_vert.screen.x - temp_i;
    max.x = highlight_vert.screen.x + temp_i;
    min.y = highlight_vert.screen.y - temp_i;
    max.y = highlight_vert.screen.y + temp_i;
    if ( ! Point_In_Rect( pt, min, max ))
        return;

    selected_point.spec_type = nature;
    switch ( nature )
    {
        case absolute_spec:
            selected_point.spec_data = NULL;
            selected_point.spec_vector = obj->o_world_verts[highlight_offset];
            break;

        case offset_spec:
            selected_point.spec_data = NULL;
            VSub(obj->o_world_verts[highlight_offset],
                 select_center, selected_point.spec_vector);
            break;

        case reference_spec:
            if ( Obj_Is_Triangle(obj) ||
                 ( Obj_Is_Bezier(obj) && highlight_offset >= obj->o_num_real ) )
            {
                selected_point.spec_type = nature = vertex_spec;
                selected_point.spec_data = (void*)obj;
                if ( Obj_Is_Bezier(obj) )
                    selected_point.spec_vector.x =
                        (double)(highlight_offset - obj->o_num_real);
                else
                    selected_point.spec_vector.x = highlight_offset;
                selected_point.spec_vector.y = selected_point.spec_vector.z = 0;
            }
            else if ( Obj_Is_Bezier(obj) )
            {
                int     num_lines;
                double  step;

                /* Look for an identical control vertex. */
                for ( i = 0 ; i < 16 ; i++ )
                {
                    if ( obj->o_main_verts[highlight_offset].screen.x ==
                         obj->o_main_verts[obj->o_num_real + i].screen.x &&
                         obj->o_main_verts[highlight_offset].screen.y ==  
                         obj->o_main_verts[obj->o_num_real + i].screen.y )
                    {
                        selected_point.spec_type = nature = vertex_spec;
                        selected_point.spec_data = (void*)obj;
                        selected_point.spec_vector.x = (double)i;
                        selected_point.spec_vector.y =
                        selected_point.spec_vector.z = 0;
                        break;
                    }
                }
                if ( i < 16 )
                    break;

                selected_point.spec_type = nature = parameter_spec;
                selected_point.spec_data = (void*)obj;
                num_lines = (int)sqrt((double)obj->o_num_real);
                step = 1.0 / (double)(num_lines - 1);
                selected_point.spec_vector.x =
                    ( highlight_offset % num_lines ) * step;
                selected_point.spec_vector.y =
                    ( highlight_offset / num_lines ) * step;
                selected_point.spec_vector.z = 0;
            }
            else
            {
                selected_point.spec_data = (void*)obj;
                selected_point.spec_vector =
                    obj->o_wireframe->vertices[highlight_offset];
            }
            break;

        case vertex_spec:
        case parameter_spec:
        case feature_spec:
        case other_spec:
        case null_spec:;
    }
    

    action = select_callback(&selected_point);

    switch ( action )
    {
        case SELECT:
            /* Change the appearance of the selected vertex. */
            rect.width = rect.height = sced_resources.select_pt_width;
            rect.x = highlight_vert.screen.x -
                     sced_resources.select_pt_width / 2;
            rect.y = highlight_vert.screen.y -
                     sced_resources.select_pt_width / 2;
            if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
            {
                XFillRectangles(XtDisplay(select_window->shell),
                                XtWindow(select_window->view_widget),
                                ready_gc, &rect, 1);
                XFillRectangles(XtDisplay(select_window->shell),
                                XtWindow(select_window->view_widget),
                                selected_gc, &rect, 1);
            }
            else
                XDrawRectangles(XtDisplay(select_window->shell),
                                XtWindow(select_window->view_widget),
                                selected_gc, &rect, 1);
            chosen_specs[num_chosen] = selected_point;
            chosen_pts[num_chosen++] = highlight_vert.screen;
            break;
        case DESELECT:
            Select_Deselect_Point(&selected_point);
            break;
        case IGNORE:
            break;
    }
}


void
Select_Deselect_Point(ConstraintSpecPtr victim)
{
    XPoint      point;
    XRectangle  rect;
    Vector      diff;
    int i;

    /* Remove it from the chosen list. */
    for ( i = 0 ;
          i < num_chosen && ! Spec_Equal(chosen_specs[i], *victim, diff) ;
          i++ );
    if ( i < num_chosen )
    {
        point = chosen_pts[i];
        for ( i++ ; i < num_chosen ; i++ )
        {
            chosen_specs[i-1] = chosen_specs[i];
            chosen_pts[i-1] = chosen_pts[i];
        }
        num_chosen--;
    }
    else return;

    /* Undraw it. */
    rect.width = rect.height = sced_resources.select_pt_width;
    rect.x = point.x - sced_resources.select_pt_width / 2;
    rect.y = point.y - sced_resources.select_pt_width / 2;
    if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
    {
        XFillRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        ready_gc, &rect, 1);
        XFillRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        selected_gc, &rect, 1);
    }
    else
        XDrawRectangles(XtDisplay(select_window->shell),
                        XtWindow(select_window->view_widget),
                        selected_gc, &rect, 1);
}


void
Selection_Recalculate_Points(Viewport* viewport, short w, short h, double mag)
{
    Vector  location;
    Vertex  view_loc;
    int     i;

    for ( i = 0 ; i < num_chosen ; i++ )
    {
        location = Spec_Absolute(chosen_specs + i, select_center);
        Convert_World_To_View(&location, &view_loc, 1, viewport);
        Convert_View_To_Screen(&view_loc, 1, viewport, w, h, mag);
        chosen_pts[i] = view_loc.screen;
    }
}


static void
Draw_Control_Verts(Display *disp, Drawable draw, GC gc, Vertex *verts,
                   int num_verts)
{
    int     i;

    for ( i = 0 ; i < num_verts ; i++ )
    {
        XDrawLine(disp, draw, gc, verts[i].screen.x - 5, verts[i].screen.y,
                  verts[i].screen.x + 5, verts[i].screen.y);
        XDrawLine(disp, draw, gc, verts[i].screen.x, verts[i].screen.y - 5,
                  verts[i].screen.x, verts[i].screen.y + 5);
    }
}


/*  void
**  Draw_Selection_Points(Drawable draw)
**  Draws all the selection points.
*/
void
Draw_Selection_Points(Drawable draw)
{
    XRectangle  rect;
    int         i;
    InstanceList    temp_l;

    if ( ! highlight_inst )
        return;

    rect.width = rect.height = sced_resources.select_pt_width;

    for ( i = 0 ; i < num_chosen ; i++ )
    {
        rect.x = chosen_pts[i].x - sced_resources.select_pt_width / 2;
        rect.y = chosen_pts[i].y - sced_resources.select_pt_width / 2;

        if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
            XFillRectangles(XtDisplay(select_window->shell), draw,
                            selected_gc, &rect, 1);
        else
            XDrawRectangles(XtDisplay(select_window->shell), draw,
                            selected_gc, &rect, 1);
    }

    rect.x = highlight_vert.screen.x -
             sced_resources.select_pt_width / 2;
    rect.y = highlight_vert.screen.y -
             sced_resources.select_pt_width / 2;
    if (DefaultDepthOfScreen(XtScreen(select_window->shell)) == 1)
        XFillRectangles(XtDisplay(select_window->shell), draw,
                        ready_gc, &rect, 1);
    else
        XDrawRectangles(XtDisplay(select_window->shell), draw,
                        ready_gc, &rect, 1);

    
    for ( temp_l = select_instances ; temp_l ; temp_l = temp_l->next )
        if ( Obj_Is_Bezier(temp_l->the_instance) &&
             (temp_l->the_instance->o_flags & ObjVisible) )
            Draw_Control_Verts(XtDisplay(select_window->shell), draw,
                               control_gc,
                               temp_l->the_instance->o_main_verts +
                                temp_l->the_instance->o_num_real, 16);

}



void
Prepare_Selection_Drawing()
{
    Colormap    col_map;
    Pixel       foreground_pixel, background_pixel;
    XGCValues   gc_vals;
    char        *tmp;

    if ( ! initialized )
    {
        /* Get the colormap. */
        XtVaGetValues(select_window->view_widget,
                    XtNcolormap, &col_map,
                    XtNforeground, &foreground_pixel,
                    XtNbackground, &background_pixel, NULL);

        gc_vals.function = GXxor;

        if ( DefaultDepthOfScreen(XtScreen(select_window->shell) ) == 1)
        {
            gc_vals.foreground = foreground_pixel ^ background_pixel;
            control_gc =
            ready_gc = XtGetGC(select_window->shell, GCFunction | GCForeground,
                                &gc_vals);

            gc_vals.line_width = 3;
            highlight_gc = XtGetGC(select_window->shell,
                                   GCFunction | GCForeground | GCLineWidth,
                                   &gc_vals);


            if ( ! stippled_pixmap )
                stippled_pixmap =
                    XCreateBitmapFromData(XtDisplay(select_window->shell),
                            XtWindow(select_window->view_widget), stipple_bits,
                            stipple_width, stipple_height);
            gc_vals.fill_style = FillStippled;
            gc_vals.stipple = stippled_pixmap;
            selected_gc = XtGetGC(select_window->shell,
                    GCFunction | GCForeground | GCFillStyle | GCStipple,
                    &gc_vals);
        }
        else
        {
            /* Allocate the GCs. */
            gc_vals.line_width = sced_resources.select_pt_line_width;
            gc_vals.foreground =
                sced_resources.selected_pt_color ^ background_pixel;
            selected_gc = XtGetGC(select_window->shell,
                            GCFunction | GCForeground | GCLineWidth, &gc_vals);

            gc_vals.foreground = sced_resources.active_color ^ background_pixel;
            ready_gc = XtGetGC(select_window->shell,
                            GCFunction | GCForeground | GCLineWidth, &gc_vals);

            gc_vals.foreground =
                sced_resources.referenced_color ^ foreground_pixel;
            highlight_gc = XtGetGC(select_window->shell,
                                   GCFunction | GCForeground, &gc_vals);

            gc_vals.foreground = foreground_pixel;
            control_gc = XtGetGC(select_window->shell,
                                 GCFunction | GCForeground, &gc_vals);
        }

        select_cursor = XCreateFontCursor(XtDisplay(select_window->shell),
                                          XC_hand2);
        initialized = TRUE;
    }

    /* Save some resources: the text label and the string. */
    if ( allow_text_entry )
    {
        XtVaGetValues(select_window->text_label, XtNlabel, &tmp, NULL);
        strcpy(orig_label, tmp);
        strcpy(orig_string, select_window->text_string);

        /* Update the apply and text stuff. */
        Set_Prompt(select_window, "");

        XtVaSetValues(select_window->text_label, XtNlabel, "Point:", NULL);

        XtRemoveCallback(select_window->apply_button, XtNcallback,
                            Apply_Button_Callback, (XtPointer)select_window);
        XtAddCallback(select_window->apply_button, XtNcallback,
                        Select_Apply_Callback, (XtPointer)select_window);
    }

    num_chosen = 0;

    XDefineCursor(XtDisplay(select_window->shell),
                  XtWindow(select_window->view_widget), select_cursor);
}


void
Cleanup_Selection()
{
    /* Update the apply and text stuff. */
    if ( allow_text_entry )
    {
        Dimension   width;
        Set_Prompt(select_window, orig_string);
        XtVaGetValues(select_window->apply_button, XtNwidth, &width, NULL);
        XtVaSetValues(select_window->text_label,
                      XtNwidth, width, XtNlabel, orig_label, NULL);
        XtRemoveCallback(select_window->apply_button, XtNcallback,
                            Select_Apply_Callback, (XtPointer)select_window);
        XtAddCallback(select_window->apply_button, XtNcallback,
                        Apply_Button_Callback, (XtPointer)select_window);
    }

    num_chosen = 0;
    highlight_inst = NULL;

    XDefineCursor(XtDisplay(select_window->shell),
                  XtWindow(select_window->view_widget), None);
}


static void
Select_Apply_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ! allow_text_entry ) return;

    if (  sscanf(select_window->text_string, "%lg %lg %lg",
            &(entered_vect.x), &(entered_vect.y), &(entered_vect.z) )  != 3 )
        return;

    if ( ! prompt_offset )
    {
        Select_Point_Apply_Absolute(NULL, NULL, NULL);
        return;
    }

    if ( ! select_apply_dialog )
        Select_Apply_Create_Dialog();

    Position_Shell(select_apply_dialog, FALSE);
    XtPopup(select_apply_dialog, XtGrabExclusive);
}


static void
Select_Point_Apply_Absolute(Widget w, XtPointer cl, XtPointer ca)
{
    ConstraintSpec  selected_point;

    if ( w )
        XtPopdown(select_apply_dialog);

    selected_point.spec_type = absolute_spec;
    selected_point.spec_data = NULL;
    selected_point.spec_vector = entered_vect;

    select_callback(&selected_point);

    Set_Prompt(select_window, "");
}


static void
Select_Point_Apply_Offset(Widget w, XtPointer cl, XtPointer ca)
{
    ConstraintSpec  selected_point;

    XtPopdown(select_apply_dialog);

    selected_point.spec_type = offset_spec;
    VAdd(entered_vect, select_center, selected_point.spec_vector);
    selected_point.spec_data = NULL;

    select_callback(&selected_point);

    Set_Prompt(select_window, "");
}

static void
Select_Apply_Create_Dialog()
{
    Widget  dialog;
    Arg     args[5];
    int     n;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Point Type");      n++;
    select_apply_dialog = XtCreatePopupShell("pointTypeShell",
                            transientShellWidgetClass, select_window->shell,
                            args, n);

    n = 0;
    XtSetArg(args[n], XtNlabel, "Point Type?"); n++;
    dialog = XtCreateManagedWidget("pointTypeDialog", dialogWidgetClass,
                                    select_apply_dialog, args, n);

    XawDialogAddButton(dialog, "Absolute", Select_Point_Apply_Absolute, NULL);
    XawDialogAddButton(dialog, "Offset", Select_Point_Apply_Offset, NULL);

    XtVaSetValues(XtNameToWidget(dialog, "label"), XtNborderWidth, 0, NULL);

    XtRealizeWidget(select_apply_dialog);
}


Vector
Spec_Absolute(ConstraintSpecPtr spec, Vector center)
{
    Vector  result;

    switch ( spec->spec_type )
    {
        case absolute_spec:
            return spec->spec_vector;

        case offset_spec:
            VAdd(center, spec->spec_vector, result);
            return result;

        case reference_spec:
            Ref_Transform_Vector(spec_object(spec), spec->spec_vector, result);
            return result;

        case vertex_spec:
            Ref_Transform_Vector(spec_object(spec),
                                 control_part(spec_object(spec))->
                                    control_verts[(int)spec->spec_vector.x],
                                 result);
            return result;

        case parameter_spec:
            return Bezier_Calculate_Point(spec_object(spec),
                                          spec->spec_vector.x,
                                          spec->spec_vector.y);

        case feature_spec:
        case other_spec:
        case null_spec:;
    }

    return result;
}
