/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/draw.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: draw.c,v $
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
**  draw.c : The various drawing functions.
**
**  Created: 19/03/94
**
**  External functions:
**  void
**  View_Update(WindowInfoPtr window, int type);
**  Updates the view in the view widget.
**
**  void
**  Draw_Edges(Display *disp, Drawable draw, GC visible_gc, GC hidden_gc,
**      WireframePtr wire, Vertex *vertices, Vector *world_verts,
**      Vector *normals, Viewport *viewport) 
**  Draws the edges associated with the wireframe wire using vertices vertices
**  with face normals normals.
**  visible_gc is used for front edges and hidden_gc for back edges.
**
**  void
**  Draw_Visible_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
**      Vertex *vertices, Vector *world_verts, Vector *normals,
**      Viewport *viewport)
**  Draws only the edges of obj which are visible with respect to viewport.
**
**  void
**  Draw_All_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
**      Vertex *vertices, Boolean);
**  Draws all the edges, regardless of visibility.
*/

#include <math.h>
#include <sced.h>
#include <edge_table.h>
#include <edit.h>
#include <select_point.h>
#include <View.h>

/* Local functions. */
static void Draw_Axes(Display*, Drawable, Viewport*, ObjectInstancePtr);
static void Draw_Camera(Display*, Drawable, WindowInfoPtr window, int);
static Boolean Is_Visible(WireframePtr, Vector*, Vector*, short, Viewport*);
static void Determine_Visible(WireframePtr, EdgeTable, Vector*, Vector*,
                              Viewport*);


static Pixmap       off_screen;
static GC           visible_gc;
static GC           highlight_gc;
static GC           hidden_gc;
static GC           highlight_hidden_gc;
static GC           clear_gc;
static GC           edit_gc;
static GC           constraint_gc;
static GC           lights_gc;
static GC           lights_hidden_gc;
static GC           lights_highlight_gc;
static GC           construct_gc;
static GC           construct_hidden_gc;
static GC           aliased_gc;
static GC           aliased_hidden_gc;
static GC           camera_gc;
static GC           camera_hidden_gc;
static GC           camera_highlight_gc;
static GC           spec_gc[null_spec];
static short        con_width[pt_last_feature];
XRectangle          area;
static XArc         point_arc;

#define origin_width 2
#define origin_height 2
static char origin_bits[] = { 0x03, 0x01 };

static EditInfoPtr  draw_info;



/*  void
**  View_Update(WindowInfoPtr window, InstanceList instances, int type);
**  Updates the view in the view widget.  This can involve many things,
**  so a group of flags is used.
**  - ViewNone      Just do the drawing, and nothing else.
**  - CalcView      Recalculate the view coords.  This implies CalcScreen.
**  - CalcScreen    Recalculate the screen coords for the objects.
**  - RemoveHidden  Draw with hidden lines removed for each object.
**  - NewWindow     This is a new window.  Allocate new GCs etc.
*/
void
View_Update(WindowInfoPtr window, InstanceList instances, int flags)
{
    InstanceList        current_inst;
    Vertex              *view_verts;
    GC                  foreground_gc, hide_gc;
    Dimension           new_width, new_height;
    int                 i;
    static WindowInfoPtr    last_window = NULL;
    static double           last_magnify;
    static XPoint           last_center;

    if ( ( flags & JustExpose ) && last_window == window )
    {
        /* Copy the off screen on. */
        XCopyArea(XtDisplay(window->shell), window->off_screen,
                  XtWindow(window->view_widget), visible_gc,
                  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
                  area.x, area.y);

        if ( window->current_state & SelectPointState )
        {
            Select_Highlight_Closest(XtWindow(window->view_widget));
            Draw_Selection_Points(XtWindow(window->view_widget));
        }
        return;
    }

    if ( window->current_state & EditState )
        draw_info = Edit_Get_Info();

    XtVaGetValues(window->view_widget, XtNwidth, &new_width,
                XtNheight, &new_height, NULL);

    if ( (flags & NewSize) || ! window->off_screen )
    {
        /* Clean up the old stuff. */
        if ( window->off_screen )
            XFreePixmap(XtDisplay(window->shell), window->off_screen);

        /* Allocate a new off_screen pixmap. */
        window->off_screen = XCreatePixmap(XtDisplay(window->shell),
                                XtWindow(window->shell),
                                (unsigned)new_width, (unsigned)new_height,
                                DefaultDepthOfScreen(XtScreen(window->shell)));
    }

    off_screen = window->off_screen;
    area.x = area.y = 0;
    area.width = (short)window->width;
    area.height = (short)window->height;

    /* Clear the window. */
    XFillRectangles(XtDisplay(window->shell), off_screen, clear_gc, &area, 1);

    /* Deal with the axes. */
    /* Recalculate anyway.  It's little effort and saves worrying
    **  about which window it's in.
    */
    Convert_World_To_View(window->axes.o_world_verts,
            window->axes.o_main_verts, 7, &(window->viewport));
    Convert_View_To_Screen(window->axes.o_main_verts, 7,
            &(window->viewport), area.width, area.height, window->magnify);

    Draw_Axes(XtDisplay(window->shell), off_screen,
              &(window->viewport), &(window->axes));

    if ( window == &main_window )
        Draw_Camera(XtDisplay(window->shell), off_screen, window, flags);

    if ( flags & CalcView )
        for ( current_inst = instances ;
              current_inst != NULL ; current_inst = current_inst->next )
        {
            if ( ! ( current_inst->the_instance->o_flags & ObjVisible ) )
                continue;

            Convert_World_To_View(current_inst->the_instance->o_world_verts,
                current_inst->the_instance->o_main_verts,
                current_inst->the_instance->o_num_vertices,
                &(window->viewport));
        }

    if ( flags & ( CalcView | CalcScreen ) )
        for ( current_inst = instances ;
              current_inst != NULL ; current_inst = current_inst->next )
        {
            if ( ! ( current_inst->the_instance->o_flags & ObjVisible ) )
                continue;

            Convert_View_To_Screen(current_inst->the_instance->o_main_verts,
                current_inst->the_instance->o_num_vertices,
                &(window->viewport), area.width, area.height, window->magnify);
        }

    for ( current_inst = window->all_instances ;
          current_inst != NULL ; current_inst = current_inst->next )
    {
        if ( ! ( current_inst->the_instance->o_flags & ObjVisible ) )
            continue;

        view_verts = current_inst->the_instance->o_main_verts;

        /* Defaults. */
        foreground_gc = visible_gc;
        hide_gc = hidden_gc;

        /* Check for edit state. */
        if ( window->current_state & EditState )
        {
            if ( current_inst->the_instance == draw_info->obj )
                continue;
            if ( current_inst->the_instance->o_flags & ObjDepends )
            {
                foreground_gc = hidden_gc;
                hide_gc = hidden_gc;
            }
        }

        if ( ( current_inst->the_instance->o_flags & ObjSelected ) &&
             ! ( window->current_state & EditState ) )
        {
            foreground_gc = highlight_gc;
            hide_gc = highlight_hidden_gc;
        }

        if ( Obj_Is_Light(current_inst->the_instance) )
        {
            if ( foreground_gc == visible_gc )
                foreground_gc = lights_gc;
            else if ( foreground_gc == hidden_gc )
                foreground_gc = lights_hidden_gc;
            else
                foreground_gc = lights_highlight_gc;

            hide_gc = lights_hidden_gc;
        }

        if ( Obj_Is_Aliased(current_inst->the_instance, target_renderer) )
        {
            if ( foreground_gc == visible_gc )
                foreground_gc = aliased_gc;
            else if ( foreground_gc == hidden_gc )
                foreground_gc = aliased_hidden_gc;

            hide_gc = aliased_hidden_gc;
        }

        if ( Obj_Is_Construction(current_inst->the_instance) )
        {
            if ( foreground_gc == visible_gc )
                foreground_gc = construct_gc;
            else if ( foreground_gc == hidden_gc )
                foreground_gc = construct_hidden_gc;

            hide_gc = construct_hidden_gc;
        }

        /* Set all the drawn flags on vertices. */
        /* There's no easy way around it. I wish there was. */
        for ( i = 0 ; i < current_inst->the_instance->o_num_real ; i++ )
            view_verts[i].drawn = FALSE;
        for ( ; i < current_inst->the_instance->o_num_vertices ; i++ )
            view_verts[i].drawn =
                ( view_verts[i].view.z > - window->viewport.eye_distance );

        if ( ( ( flags & RemoveHidden ) ||
               ( window->viewport.draw_mode == DRAW_CULLED &&
                 ! ( current_inst->the_instance->o_flags & ObjSelected ) ) ) &&
             ! Obj_Is_Control(current_inst->the_instance) )
            Draw_Visible_Edges(XtDisplay(window->shell),
                off_screen, foreground_gc,
                current_inst->the_instance->o_wireframe,
                view_verts, current_inst->the_instance->o_world_verts,
                current_inst->the_instance->o_normals, &(window->viewport));
        else if ( window->viewport.draw_mode == DRAW_DASHED ||
                  ( window->viewport.draw_mode == DRAW_CULLED &&
                    ( ( current_inst->the_instance->o_flags & ObjSelected ) ||
                      Obj_Is_Control(current_inst->the_instance) ) ) )
            Draw_Edges(XtDisplay(window->shell),
                off_screen, foreground_gc, hide_gc,
                current_inst->the_instance->o_wireframe,
                view_verts, current_inst->the_instance->o_world_verts,
                current_inst->the_instance->o_normals, &(window->viewport));
        else
            Draw_All_Edges(XtDisplay(window->shell),
                off_screen, foreground_gc,
                current_inst->the_instance->o_wireframe, view_verts,
                &(window->viewport));

        if ( current_inst->the_instance->o_parent->b_class == light_obj )
        {
            XPoint  center;

            center =
            view_verts[current_inst->the_instance->o_num_vertices - 1].screen;

            XFillArc(XtDisplay(window->shell), off_screen, lights_gc,
                center.x - sced_resources.light_pt_rad,
                center.y - sced_resources.light_pt_rad,
                sced_resources.light_pt_rad << 1,
                sced_resources.light_pt_rad << 1, 0, 23040);
        }
    }

    if ( window->current_state & EditState )
    {
        Edit_Draw(window, flags, draw_info, FALSE);
        if ( ! draw_info->selecting )
        {
            Draw_Edit_Extras(window, flags, draw_info, FALSE);
            Draw_All_Constraints(window, flags, draw_info, FALSE);
        }
    }

    /* Copy the off screen on. */
    XCopyArea(XtDisplay(window->shell), off_screen,
              XtWindow(window->view_widget), visible_gc,
              area.x, area.y, (unsigned)area.width, (unsigned)area.height,
              area.x, area.y);

    if ( window->current_state & SelectPointState )
    {
        Select_Highlight_Closest(XtWindow(window->view_widget));
        if ( flags & ( CalcView | CalcScreen ) )
            Selection_Recalculate_Points(&(window->viewport), area.width,
                                         area.height, window->magnify);
        Draw_Selection_Points(XtWindow(window->view_widget));
    }

    last_center.x = window->width / 2;
    last_center.y = window->height / 2;
    last_magnify = window->magnify;
    last_window = window;
}


void
Edit_Draw(WindowInfoPtr window, int flags, EditInfoPtr info, Boolean copy)
{
    off_screen = window->off_screen;
    area.x = area.y = 0;
    area.width = (short)window->width;
    area.height = (short)window->height;


    if (flags & CalcView)
        Convert_World_To_View(info->obj->o_world_verts, info->obj->o_main_verts,
                              info->obj->o_num_vertices, &(window->viewport));

    if (flags & (CalcView | CalcScreen))
        Convert_View_To_Screen(info->obj->o_main_verts,
            info->obj->o_num_vertices,
            &(window->viewport), area.width, area.height, window->magnify);


    /* Draw the body. */
    if (flags & RemoveHidden)
        Draw_Visible_Edges(XtDisplay(window->shell), off_screen, edit_gc,
                            info->obj->o_wireframe, info->obj->o_main_verts,
                            info->obj->o_world_verts, info->obj->o_normals,
                            &(window->viewport));
    else
        Draw_All_Edges(XtDisplay(window->shell), off_screen, edit_gc,
                        info->obj->o_wireframe, info->obj->o_main_verts,
                        &(window->viewport));

    if ( copy )
        /* Copy the off screen on. */
        XCopyArea(XtDisplay(window->shell), off_screen,
                  XtWindow(window->view_widget), visible_gc,
                  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
                  area.x, area.y);
}


void
Draw_Feature_Point(int num, FeaturePtr feat, WindowInfoPtr window, int flags,
                   EditInfoPtr info)
{
    if ( flags & ( CalcView | CalcScreen ) )
        Feature_Calculate_Point_Display(num, feat->base, info);

    XFillArcs(XtDisplay(window->shell), window->off_screen,
              feat->base->draw_gc1, &(info->features[num].circle), 1);
}

void
Draw_Control_Point(int num, FeaturePtr feat, WindowInfoPtr window, int flags,
                   EditInfoPtr info)  
{
    int     x, y;
    char    str[4];

    if ( flags & ( CalcView | CalcScreen ) )
        Feature_Calculate_Point_Display(num, feat->base, info);

    /* Needed to get around a problem when an object is edited and
    ** it has forced constraints. In such a case, this function can
    ** be called before the view stuff has been initialized.
    */
    if ( ! info->features[num].num_view )
	return;

    XFillArcs(XtDisplay(window->shell), window->off_screen,
              feat->base->draw_gc1, &(info->features[num].circle), 1);

    /* Add labels. */
    x = info->features[num].view[0].screen.x;
    y = info->features[num].view[0].screen.y;
    x += ( x < 10 ? 10 : -10);
    y += ( y < 10 ? 10 : -10);
    sprintf(str, "%0d", num - pt0_feature);
    XDrawString(XtDisplay(window->shell), window->off_screen,
                feat->base->draw_gc2, x, y, str, strlen(str));
}


void
Draw_Major_Feature(int num, FeaturePtr feat, WindowInfoPtr window, int flags,
                   EditInfoPtr info)
{
    if ( flags & ( CalcView | CalcScreen ) )
        Feature_Calculate_Major_Display(num, feat->base, info);

    /* Needed to get around a problem when an object is edited and
    ** it has forced constraints. In such a case, this function can
    ** be called before the view stuff has been initialized.
    */
    if ( ! info->features[major_feature].num_view )
	return;

    XDrawLine(XtDisplay(window->shell), window->off_screen,
              feat->base->draw_gc1,
              info->features[major_feature].view[0].screen.x,
              info->features[major_feature].view[0].screen.y,
              info->features[major_feature].view[1].screen.x,
              info->features[major_feature].view[1].screen.y);

    /* Draw the rotate circle. */
    XDrawArcs(XtDisplay(window->shell), window->off_screen,
              feat->base->draw_gc2, &(info->features[major_feature].circle), 1);

}

void
Draw_Minor_Feature(int num, FeaturePtr feat, WindowInfoPtr window, int flags,
                   EditInfoPtr info)
{
    if ( flags & ( CalcView | CalcScreen ) )
        Feature_Calculate_Minor_Display(num, feat->base, info);

    /* Needed to get around a problem when an object is edited and
    ** it has forced constraints. In such a case, this function can
    ** be called before the view stuff has been initialized.
    */
    if ( ! info->features[minor_feature].num_view )
	return;

    XDrawLine(XtDisplay(window->shell), window->off_screen,
              feat->base->draw_gc1,
              info->features[minor_feature].view[0].screen.x,
              info->features[minor_feature].view[0].screen.y,
              info->features[minor_feature].view[1].screen.x,
              info->features[minor_feature].view[1].screen.y);
}


void
Draw_Edit_Extras(WindowInfoPtr window, int flags, EditInfoPtr info,Boolean copy)
{
    FeatureBasePtr  base;
    int i;

    area.x = area.y = 0;
    area.width = (short)window->width;
    area.height = (short)window->height;

    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
    {
        base = info->obj->o_features[i].base;
        if ( ! base )
            continue; /* Cope with dummy base. */
        if ( ! base->draw_initialised )
            base->draw_init_func(base);
        if ( base->draw_func )
            base->draw_func(i, info->obj->o_features + i, window, flags, info);
    }

    if ( copy )
        /* Copy the off screen on. */
        XCopyArea(XtDisplay(window->shell), off_screen,
                  XtWindow(window->view_widget), visible_gc,
                  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
                  area.x, area.y);
}


static void
Draw_Set_Point_Arc(XPoint *pt, WindowInfoPtr window)
{
    point_arc.x = pt->x - sced_resources.point_con_rad;
    point_arc.y = pt->y - sced_resources.point_con_rad;
    point_arc.width = sced_resources.point_con_rad << 1;
    point_arc.height = sced_resources.point_con_rad << 1;
    point_arc.angle1 = 0;
    point_arc.angle2 = 23040;
}


static void
Draw_Constraint(WindowInfoPtr window, ResultantPtr resultant, XPoint *pts,
                int num, XArc *sphere_arc, Boolean recalc)
{
    off_screen = window->off_screen;

    if ( resultant->feature_1.c_type == null_feature )
        return;

    switch ( resultant->feature_1.c_type )
    {
        case plane_feature:
            XDrawLines(XtDisplay(window->shell), off_screen,
                    constraint_gc, pts, 4, CoordModeOrigin);
            XDrawLine(XtDisplay(window->shell), off_screen,
                    constraint_gc, pts[3].x, pts[3].y, pts[0].x, pts[0].y);
            XDrawLine(XtDisplay(window->shell), off_screen,
                    constraint_gc, pts[0].x, pts[0].y, pts[2].x, pts[2].y);
            XDrawLine(XtDisplay(window->shell), off_screen,
                    constraint_gc, pts[1].x, pts[1].y, pts[3].x, pts[3].y);
            break;

        case line_feature:
            XDrawLine(XtDisplay(window->shell), off_screen,
                    constraint_gc, pts[0].x, pts[0].y, pts[1].x, pts[1].y);
            break;

        case point_feature:
            Draw_Set_Point_Arc(pts, window);
            XDrawArcs(XtDisplay(window->shell), off_screen, constraint_gc,
                      &point_arc, 1);
            break;

        case sphere_feature:
            XDrawLines(XtDisplay(window->shell), off_screen,
                       constraint_gc, pts, ARC_DIVISIONS + 1, CoordModeOrigin);
            XDrawLines(XtDisplay(window->shell), off_screen,
                       constraint_gc, pts + ARC_DIVISIONS + 1,
                       ARC_DIVISIONS + 1, CoordModeOrigin);
            XDrawLines(XtDisplay(window->shell), off_screen,
                       constraint_gc, pts + 2 * ( ARC_DIVISIONS + 1 ),
                       ARC_DIVISIONS + 1, CoordModeOrigin);
            XDrawArcs(XtDisplay(window->shell), off_screen, constraint_gc,
                      sphere_arc, 1);
            break;

        case circle_feature:
            XDrawLines(XtDisplay(window->shell), off_screen,
                       constraint_gc, pts, num, CoordModeOrigin);
            break;

        case inconsistent_feature:
            XDrawLine(XtDisplay(window->shell), off_screen,
                constraint_gc, pts[0].x, pts[0].y, pts[1].x, pts[1].y);
            XDrawLine(XtDisplay(window->shell), off_screen,
                constraint_gc, pts[2].x, pts[2].y, pts[3].x, pts[3].y);
            break;

        default:;
    }

    if ( resultant->feature_2.c_type == point_feature )
    {
        Draw_Set_Point_Arc(pts + 1, window);
        XDrawArcs(XtDisplay(window->shell), off_screen, constraint_gc,
                  &point_arc, 1);
    }
}


void
Draw_All_Constraints(WindowInfoPtr window, int flags, EditInfoPtr info,
                     Boolean copy)
{
    int i;

    off_screen = window->off_screen;
    area.x = area.y = 0;
    area.width = (short)window->width;
    area.height = (short)window->height;

    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
    {
        if ( ! info->obj->o_features[i].base )  continue;
        if ( info->obj->o_features[i].base->draw_con_func )
            info->obj->o_features[i].base->draw_con_func(i,
                        info->obj->o_features + i, window, flags, info, FALSE);
    }

    if ( copy )
        XCopyArea(XtDisplay(window->shell), off_screen,
                  XtWindow(window->view_widget), visible_gc,
                  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
                  area.x, area.y);
}

void
Draw_Point_Constraints(int num, FeaturePtr feature, WindowInfoPtr window,
                        int flags, EditInfoPtr info, Boolean copy)
{
    int i;

    off_screen = window->off_screen;
    area.x = area.y = 0;
    area.width = (short)window->width;
    area.height = (short)window->height;

    if ( info->features[num].resultant.feature_1.c_type == null_feature )
    {
        if ( copy )
            XCopyArea(XtDisplay(window->shell), off_screen,
                      XtWindow(window->view_widget), visible_gc,
                      area.x, area.y, (unsigned)area.width,
                      (unsigned)area.height, area.x, area.y);
        return;
    }

    if ( flags & (CalcView | CalcScreen) )
        Edit_Calculate_Cons_Points(info, num);

    /* Needed to work around a problem where this function is called
    ** before the view calculations are done.
    */
    if ( ! info->features[num].def_pts.num_pts )
	return;

    Draw_Constraint(window, &(info->features[num].resultant),
                    info->features[num].def_pts.pts,
                    info->features[num].def_pts.num_pts,
                    &(info->features[num].def_pts.sphere_arc),
                    flags & ( CalcView | CalcScreen ));

    for ( i = 0 ; i < info->features[num].def_pts.num_def_pts ; i++ )
        XDrawRectangle(XtDisplay(window->shell), off_screen,
                spec_gc[info->features[num].def_pts.def_pts[i].type],
                info->features[num].def_pts.def_pts[i].pt.x - con_width[num],
                info->features[num].def_pts.def_pts[i].pt.y - con_width[num],
                con_width[num] << 1, con_width[num] << 1);

    if ( copy )
        /* Copy the off screen on. */
        XCopyArea(XtDisplay(window->shell), off_screen,
                  XtWindow(window->view_widget), visible_gc,
                  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
                  area.x, area.y);
}


void
Draw_Axis_Constraints(int num, FeaturePtr feature, WindowInfoPtr window,
                      int flags, EditInfoPtr info, Boolean copy)
{
    int i;

    off_screen = window->off_screen;
    area.x = area.y = 0;
    area.width = (short)window->width;
    area.height = (short)window->height;

    if ( flags & (CalcView | CalcScreen) )
        Edit_Calculate_Axis_Cons_Points(info, num);

    /* Needed to work around a problem where this function is called
    ** before the view calculations are done.
    */
    if ( ! info->features[num].def_pts.num_pts )
	return;

    /* Draw any rotation constraint. */
    if ( info->features[num].def_pts.num_pts )
        XDrawLines(XtDisplay(window->shell), off_screen,
                    constraint_gc, info->features[num].def_pts.pts,
                    info->features[num].def_pts.num_pts,
                    CoordModeOrigin);

    /* Draw any spec points. */
    for ( i = 0 ; i < info->features[num].def_pts.num_def_pts ; i++ )
        XDrawRectangle(XtDisplay(window->shell), off_screen,
                spec_gc[info->features[num].def_pts.def_pts[i].type],
                info->features[num].def_pts.def_pts[i].pt.x -
                    sced_resources.rotate_con_width,
                info->features[num].def_pts.def_pts[i].pt.y -
                    sced_resources.rotate_con_width,
                sced_resources.rotate_con_width << 1, 
                sced_resources.rotate_con_width << 1);

    if ( copy )
        /* Copy the off screen on. */
        XCopyArea(XtDisplay(window->shell), off_screen,
                  XtWindow(window->view_widget), visible_gc,
                  area.x, area.y, (unsigned)area.width, (unsigned)area.height,
                  area.x, area.y);
}


/*
**  Draws the edges associated with the wireframe wire using vertices vertices
**  with face normals normals.
**  visible_gc is used for front edges and hidden_gc for back edges.
*/
void
Draw_Edges(Display *disp, Drawable draw, GC visible_gc, GC hidden_gc,
        WireframePtr wire, Vertex *vertices,
        Vector *world_verts, Vector *normals, Viewport *viewport) 
{
    EdgeTable   edges = wire->edges;
    int         i, j;
    GC          gc;

    Determine_Visible(wire, edges, world_verts, normals, viewport);

    for ( i = 0 ; i < wire->num_real_verts ; i++ )
        for ( j = 0 ; j < edges[i].num_edges ; j++ )
            if ( vertices[i].view.z > - viewport->eye_distance ||
                 vertices[edges[i].edges[j].endpt].view.z >
                  - viewport->eye_distance )
            {
                vertices[i].drawn =
                vertices[edges[i].edges[j].endpt].drawn = TRUE;
                gc = edges[i].edges[j].val ? visible_gc : hidden_gc;
                XDrawLine(disp, draw, gc,
                        vertices[i].screen.x, vertices[i].screen.y,
                        vertices[edges[i].edges[j].endpt].screen.x,
                        vertices[edges[i].edges[j].endpt].screen.y);
            }
}


/*
**  Draws only the edges of obj which are visible with respect to viewport.
*/
void
Draw_Visible_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
                    Vertex *vertices, Vector *world_verts,
                    Vector *normals, Viewport *viewport)
{
    EdgeTable   edges = wire->edges;
    int         i, j;

    Determine_Visible(wire, edges, world_verts, normals, viewport);

    for ( i = 0 ; i < wire->num_real_verts ; i++ )
        for ( j = 0 ; j < edges[i].num_edges ; j++ )
            if ( edges[i].edges[j].val &&
                 ( vertices[i].view.z > - viewport->eye_distance ||
                   vertices[edges[i].edges[j].endpt].view.z >
                   - viewport->eye_distance ) )
            {
                vertices[i].drawn =
                vertices[edges[i].edges[j].endpt].drawn = TRUE;
                XDrawLine(disp, draw, gc,
                        vertices[i].screen.x, vertices[i].screen.y,
                        vertices[edges[i].edges[j].endpt].screen.x,
                        vertices[edges[i].edges[j].endpt].screen.y);
            }
}


/*
**  Draws all the edges, regardless of visibility.
*/
void
Draw_All_Edges(Display *disp, Drawable draw, GC gc, WireframePtr wire,
                Vertex *vertices, ViewportPtr viewport)
{
    EdgeTable   edges = wire->edges;
    int         i, j;

    for ( i = 0 ; i < wire->num_real_verts ; i++ )
        for ( j = 0 ; j < edges[i].num_edges ; j++ )
            if ( vertices[i].view.z > - viewport->eye_distance ||
                 vertices[edges[i].edges[j].endpt].view.z >
                 - viewport->eye_distance )
            {
                vertices[i].drawn =
                vertices[edges[i].edges[j].endpt].drawn = TRUE;
                XDrawLine(disp, draw, gc,
                        vertices[i].screen.x, vertices[i].screen.y,
                        vertices[edges[i].edges[j].endpt].screen.x,
                        vertices[edges[i].edges[j].endpt].screen.y);
            }
}


/*  Draws the coordinate axes in the appropriate window.
*/
static void
Draw_Axes(Display *disp, Drawable draw, Viewport* viewport,
          ObjectInstancePtr axes)
{
    int     x, y, i;
    char    str[2];

    str[1] = '\0';
    for ( i = 0 ; i < 3 ; i++ )
    {
        if ( axes->o_main_verts[i*2 + 1].view.z <= - viewport->eye_distance ||
             axes->o_main_verts[i*2 + 2].view.z <= - viewport->eye_distance )
        {
            axes->o_main_verts[i*2 + 1].drawn =
            axes->o_main_verts[i*2 + 2].drawn = FALSE;
            continue;
        }
        
        axes->o_main_verts[i*2 + 1].drawn =
        axes->o_main_verts[i*2 + 2].drawn = TRUE;
        XDrawLine(disp, draw, axis_gcs[i],
                  axes->o_main_verts[i*2 + 1].screen.x,
                  axes->o_main_verts[i*2 + 1].screen.y,
                  axes->o_main_verts[i*2 + 2].screen.x,
                  axes->o_main_verts[i*2 + 2].screen.y);

        /* Add labels. */
        x = axes->o_main_verts[i*2 + 1].screen.x;
        y = axes->o_main_verts[i*2 + 1].screen.y;
        x += ( x == 0 ? 10 : -10);
        y += ( y == 0 ? 10 : -10);
        str[0] = 'X' + i;
        XDrawString(disp, draw, axis_gcs[i], x, y, str, 1);
    }

    axes->o_main_verts[0].drawn =
        ( axes->o_main_verts[0].view.z > - viewport->eye_distance );
}


/*  Draw the camera. */
static void
Draw_Camera(Display *disp, Drawable draw, WindowInfoPtr window, int flags)
{
    GC  foreground_gc, hide_gc;

    if ( ! ( camera_object.o_flags & ObjVisible ) )
        return;

    if ( flags & CalcView )
        Convert_World_To_View(camera_object.o_world_verts,
                              camera_object.o_main_verts,
                              camera_object.o_num_vertices,
                              &(window->viewport));

    if ( flags & ( CalcView | CalcScreen ) )
        Convert_View_To_Screen(camera_object.o_main_verts,
                               camera_object.o_num_vertices,
                               &(window->viewport), area.width, area.height,
                               window->magnify);

    if ( ( camera_object.o_flags & ObjSelected ) &&
         ! ( window->current_state & EditState ) )
    {
        foreground_gc = camera_highlight_gc;
        hide_gc = highlight_hidden_gc;
    }
    else
    {
        foreground_gc = camera_gc;
        hide_gc = camera_hidden_gc;
    }

    if ( window->current_state & EditState )
    {
        if ( &camera_object == draw_info->obj )
            return;
        if ( camera_object.o_flags & ObjDepends )
        {
            foreground_gc = camera_hidden_gc;
            hide_gc = camera_hidden_gc;
        }
    }

    if ( ( flags & RemoveHidden ) ||
         ( window->viewport.draw_mode == DRAW_CULLED &&
           ! ( camera_object.o_flags & ObjSelected ) ) )
        Draw_Visible_Edges(XtDisplay(window->shell),
            off_screen, foreground_gc,
            camera_object.o_wireframe,
            camera_object.o_main_verts, camera_object.o_world_verts,
            camera_object.o_normals, &(window->viewport));
    else if ( window->viewport.draw_mode == DRAW_DASHED ||
              ( window->viewport.draw_mode == DRAW_CULLED &&
                ( camera_object.o_flags & ObjSelected ) ) )
        Draw_Edges(XtDisplay(window->shell),
            off_screen, foreground_gc, hide_gc,
            camera_object.o_wireframe,
            camera_object.o_main_verts, camera_object.o_world_verts,
            camera_object.o_normals, &(window->viewport));
    else
        Draw_All_Edges(XtDisplay(window->shell),
            off_screen, foreground_gc,
            camera_object.o_wireframe, camera_object.o_main_verts,
            &(window->viewport));
}


/*
**  Returns TRUE if face index from wireframe wire is visible wrt viewport.
*/
static Boolean
Is_Visible(WireframePtr wire, Vector *vertices, Vector *normals, short index,
            Viewport *viewport)
{
    Vector      temp_v;

    if ( wire->faces[index].num_vertices == 0 )
        return FALSE;

    VSub(viewport->eye_position, vertices[wire->faces[index].vertices[0]],
        temp_v);
    return ( VDot(temp_v, normals[index]) > 0.0 );
}


/*  void
**  Determine_Visible( ... )
**  Determines which of the faces of obj are visible wrt viewport.
**  Works only for convex objects.
*/
static void
Determine_Visible(WireframePtr wire, EdgeTable edges, Vector *vertices,
                  Vector *normals, Viewport *viewport)
{
    int i, j;

    for ( i = 0 ; i < wire->num_real_verts ; i++ )
        for ( j = 0 ; j < edges[i].num_edges ; j++ )
            edges[i].edges[j].val = NULL;

    /* For each face, check to see if it's visible and if so set draw.  */
    for ( i = 0 ; i < wire->num_faces ; i++ )
    {
        if ( Is_Visible(wire, vertices, normals, i, viewport) )
        {
            Edge_Table_Get(edges, wire->faces[i].vertices[0],
                wire->faces[i].vertices[wire->faces[i].num_vertices-1])->val =
                (void*)1;
            for ( j = 1 ; j < wire->faces[i].num_vertices ; j++ )
                Edge_Table_Get(edges, wire->faces[i].vertices[j-1],
                               wire->faces[i].vertices[j])->val = (void*)1;
        }
    }
}



void
Draw_Initialize()
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;
    Pixmap      origin_stipple;
    int         i;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);

    /* Allocate new GC's. */
    gc_vals.function = GXcopy;
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    visible_gc = XtGetGC(main_window.shell, GCFunction | GCForeground |
                         GCBackground, &gc_vals);

    gc_vals.line_style = LineOnOffDash;
    hidden_gc = XtGetGC(main_window.shell,
        GCFunction | GCForeground |GCBackground |GCLineStyle, &gc_vals);

    gc_vals.foreground = window_background;
    clear_gc = XtGetGC(main_window.shell, GCFunction | GCForeground, &gc_vals);

    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;

    if (DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
    {
        gc_vals.line_width = 2;
        highlight_gc = XtGetGC(main_window.shell,
            GCFunction | GCForeground | GCBackground | GCLineWidth, &gc_vals);

        gc_vals.line_style = LineOnOffDash;
        highlight_hidden_gc = XtGetGC(main_window.shell,
            GCFunction | GCForeground | GCBackground | GCLineStyle, &gc_vals);

        gc_vals.function = GXxor;
        gc_vals.foreground = window_foreground ^ window_background;
        edit_gc = XtGetGC(main_window.shell, GCFunction | GCForeground,
                            &gc_vals);

        origin_stipple =
            XCreateBitmapFromData(XtDisplay(main_window.shell),
                        XtWindow(main_window.view_widget), origin_bits,
                        origin_width, origin_height);
        gc_vals.fill_style = FillStippled;
        gc_vals.stipple = origin_stipple;
        spec_gc[0] = spec_gc[1] = spec_gc[2] =
        constraint_gc = XtGetGC(main_window.shell,
                GCFunction | GCForeground  | GCBackground | GCFillStyle
                | GCStipple, &gc_vals);

        gc_vals.function = GXcopy;
        gc_vals.foreground = window_foreground;
        gc_vals.background = window_background;
        aliased_gc =
        construct_gc =
        camera_gc =
        lights_gc = XtGetGC(main_window.shell, GCFunction | GCForeground |
                             GCBackground, &gc_vals);

        gc_vals.line_width = 2;
        aliased_hidden_gc =
        construct_hidden_gc =
        camera_highlight_gc =
        lights_highlight_gc = XtGetGC(main_window.shell,
            GCFunction | GCForeground | GCBackground|GCLineWidth, &gc_vals);

        gc_vals.line_style = LineOnOffDash;
        camera_hidden_gc =
        lights_hidden_gc = XtGetGC(main_window.shell,
            GCFunction | GCForeground |GCBackground |GCLineStyle, &gc_vals);
    }
    else
    {
        gc_vals.foreground = sced_resources.selected_color;
        gc_vals.line_width = sced_resources.selected_width;
        highlight_gc = XtGetGC(main_window.shell,
            GCFunction | GCForeground | GCBackground | GCLineWidth, &gc_vals);

        gc_vals.line_style = LineOnOffDash;
        highlight_hidden_gc = XtGetGC(main_window.shell,
            GCFunction | GCForeground | GCBackground | GCLineStyle, &gc_vals);

        gc_vals.function = GXxor;
        gc_vals.foreground = sced_resources.object_color ^ window_background;
        edit_gc = XtGetGC(main_window.shell,
                    GCFunction | GCForeground | GCBackground, &gc_vals);

        gc_vals.foreground =
            sced_resources.constraint_color ^ window_background;
        constraint_gc = XtGetGC(main_window.shell, GCFunction | GCForeground,
                                &gc_vals);

        gc_vals.foreground = sced_resources.absolute_color ^ window_background;
        spec_gc[absolute_spec] = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);
        gc_vals.foreground = sced_resources.offset_color ^ window_background;
        spec_gc[offset_spec] = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);
        gc_vals.foreground = sced_resources.reference_color ^ window_background;
        spec_gc[reference_spec] = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);
        gc_vals.foreground = sced_resources.vertex_color ^ window_background;
        spec_gc[vertex_spec] = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);
        spec_gc[parameter_spec] = spec_gc[reference_spec];

        gc_vals.foreground = sced_resources.light_color;
        lights_gc = XtGetGC(main_window.shell, GCForeground, &gc_vals);

        gc_vals.line_style = LineOnOffDash;
        lights_hidden_gc = XtGetGC(main_window.shell,
            GCForeground | GCLineStyle, &gc_vals);

        gc_vals.foreground = sced_resources.construct_color;
        construct_gc = XtGetGC(main_window.shell, GCForeground, &gc_vals);

        gc_vals.line_style = LineOnOffDash;
        construct_hidden_gc = XtGetGC(main_window.shell,
            GCForeground | GCLineStyle, &gc_vals);

        gc_vals.foreground = sced_resources.aliased_color;
        aliased_gc = XtGetGC(main_window.shell, GCForeground, &gc_vals);

        gc_vals.line_style = LineOnOffDash;
        aliased_hidden_gc = XtGetGC(main_window.shell,
            GCForeground | GCLineStyle, &gc_vals);

        gc_vals.foreground = sced_resources.selected_color;
        gc_vals.line_width = sced_resources.selected_width;
        camera_highlight_gc =
        lights_highlight_gc = XtGetGC(main_window.shell,
            GCForeground | GCLineWidth, &gc_vals);

        gc_vals.foreground = sced_resources.camera_color;
        camera_gc = XtGetGC(main_window.shell, GCForeground, &gc_vals);
        camera_hidden_gc = XtGetGC(main_window.shell,
            GCForeground | GCLineStyle, &gc_vals);

    }

    con_width[origin_feature] = sced_resources.origin_con_width;
    con_width[major_feature] = sced_resources.rotate_con_width;
    con_width[minor_feature] = sced_resources.rotate_con_width;
    con_width[scale_feature] = sced_resources.scale_con_width;
    con_width[radius_feature] = sced_resources.torus_con_width;
    for ( i = pt1_feature ; i < pt_last_feature ; i++ )
        con_width[i] = sced_resources.control_con_width;
}


void
Origin_Draw_Init(FeatureBasePtr feature)
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;
    Pixmap      origin_stipple;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    gc_vals.function = GXxor;

    if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
    {
        origin_stipple =
            XCreateBitmapFromData(XtDisplay(main_window.shell),
                        XtWindow(main_window.view_widget), origin_bits,
                        origin_width, origin_height);
        gc_vals.fill_style = FillStippled;
        gc_vals.stipple = origin_stipple;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                GCFunction | GCForeground  | GCBackground | GCFillStyle
                | GCStipple, &gc_vals);
    }
    else
    {
        gc_vals.foreground = sced_resources.origin_color ^ window_background;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);

    }

    feature->draw_initialised = TRUE;
}


void
Scale_Draw_Init(FeatureBasePtr feature)
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    gc_vals.function = GXxor;

    if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
        feature->draw_gc1 = XtGetGC(main_window.shell,
                            GCFunction | GCForeground  | GCBackground,
                            &gc_vals);
    else
    {
        gc_vals.foreground = sced_resources.scaling_color ^ window_background;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);
    }

    feature->draw_initialised = TRUE;
}


void
Major_Draw_Init(FeatureBasePtr feature)
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    gc_vals.line_width = sced_resources.obj_axis_width;
    gc_vals.function = GXxor;

    if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
    {
        feature->draw_gc1 = XtGetGC(main_window.shell,
                            GCFunction | GCForeground  | GCBackground,
                            &gc_vals);
        gc_vals.line_style = LineOnOffDash;
        feature->draw_gc2 = XtGetGC(main_window.shell,
                            GCFunction | GCForeground  | GCBackground |
                            GCLineStyle, &gc_vals);
    }
    else
    {
        gc_vals.foreground =
            sced_resources.obj_x_axis_color ^ window_background;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                    GCFunction | GCForeground | GCLineWidth, &gc_vals);

        gc_vals.foreground = sced_resources.arcball_color ^ window_background;
        gc_vals.line_style = LineOnOffDash;
        feature->draw_gc2 = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCLineStyle | GCBackground,
                &gc_vals);

    }

    feature->draw_initialised = TRUE;
}

void
Minor_Draw_Init(FeatureBasePtr feature)
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    gc_vals.line_width = sced_resources.obj_axis_width;
    gc_vals.function = GXxor;

    if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
        feature->draw_gc1 = XtGetGC(main_window.shell,
                            GCFunction | GCForeground  | GCBackground,
                            &gc_vals);
    else
    {
        gc_vals.foreground =
            sced_resources.obj_y_axis_color ^ window_background;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                    GCFunction | GCForeground | GCLineWidth, &gc_vals);
    }

    feature->draw_initialised = TRUE;
}


void
Radius_Draw_Init(FeatureBasePtr feature)
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    gc_vals.function = GXxor;

    if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
        feature->draw_gc1 = XtGetGC(main_window.shell,
                            GCFunction | GCForeground  | GCBackground,
                            &gc_vals);
    else
    {
        gc_vals.foreground = sced_resources.torus_color ^ window_background;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground, &gc_vals);
    }

    feature->draw_initialised = TRUE;
}


void
Control_Draw_Init(FeatureBasePtr feature)
{
    Pixel       window_foreground, window_background;
    XGCValues   gc_vals;

    XtVaGetValues(main_window.view_widget,
                XtNforeground, &window_foreground,
                XtNbackground, &window_background, NULL);
    gc_vals.foreground = window_foreground;
    gc_vals.background = window_background;
    gc_vals.function = GXxor;
    gc_vals.font = sced_resources.control_font->fid;

    if ( DefaultDepthOfScreen(XtScreen(main_window.shell)) == 1)
        feature->draw_gc1 = XtGetGC(main_window.shell,
                            GCFunction | GCForeground  | GCBackground | GCFont,
                            &gc_vals);
    else
    {
        gc_vals.foreground = sced_resources.control_color ^ window_background;
        feature->draw_gc1 = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground | GCFont, &gc_vals);
        gc_vals.foreground = sced_resources.ctext_color ^ window_background;
        feature->draw_gc2 = XtGetGC(main_window.shell,
                GCFunction | GCForeground | GCBackground | GCFont, &gc_vals);
    }

    feature->draw_initialised = TRUE;
}
