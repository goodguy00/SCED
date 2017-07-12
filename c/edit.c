/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/edit.c,v 1.2 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: edit.c,v $
 * Revision 1.2  1997/06/11 21:12:52  schenney
 * Added vertex normal calculation function for control pt objects.
 *
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
**  edit.c : functions to edit the shape of an object.
**
*/

#include <math.h>
#include <sced.h>
#include <bezier.h>
#include <constraint.h>
#include <edit.h>
#include <instance_list.h>
#include <quaternion.h>
#include <select_point.h>
#include <torus.h>
#include <triangle.h>
#include <X11/cursorfont.h>

static EditInfoPtr  Edit_Push_Info(WindowInfoPtr, InstanceList);
static void Edit_Prepare_Instances(EditInfoPtr);
static void Edit_Calculate_Spec_Points(EditInfoPtr, ConstraintPtr, int,
                                       ConPoint**, int*, Boolean);

static Cursor       no_motion_cursor = 0;
static FeatureType  active_feature = pt_last_feature;
static Cursor       old_cursor = 0;

extern void Edit_Create_Command_Menu();
Widget      edit_command_menu = NULL;


static EditInfo *edit_info_stack = NULL;
static int      max_stack_size = 0;
static int      stack_pos = 0;


void
Edit_Instance(WindowInfoPtr window, InstanceList inst)
{
    EditInfoPtr info;

    if ( Edit_Obj_On_Stack(inst->the_instance) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return;
    }

    if ( stack_pos == 0 )
        Initiate_Object_Edit(window);
    else
        Edit_Complete(Edit_Get_Info());

    info = Edit_Push_Info(window, inst);

    Edit_Begin();
}

Boolean
Edit_Obj_On_Stack(ObjectInstancePtr obj)
{
    int i;

    for ( i = 0 ; i < stack_pos ; i++ )
        if ( edit_info_stack[i].obj == obj )
            return TRUE;

    return FALSE;
}

Boolean
Edit_Begin()
{
    EditInfoPtr info;

    if ( ! ( info = Edit_Get_Info() ) )
        return FALSE;

    /* There is something still on the stack. */

    /* Set up the selection instances and so on. */
    Edit_Prepare_Instances(info);

    /* Mark all the dependents. */
    DFS(info->obj, NULL, 0, TRUE, &topological_list, TRUE, TRUE);
    Edit_Initialize_Shell(info);
    View_Update(info->window, NULL, CalcView );

    return TRUE;
}

static EditInfoPtr
Edit_Push_Info(WindowInfoPtr window, InstanceList inst)
{
    EditInfoPtr info;
    int         i;

    if ( stack_pos == max_stack_size )
    {
        i = max_stack_size;
        max_stack_size += 5;
        edit_info_stack = More(edit_info_stack, EditInfo, max_stack_size);
        for ( ; i < max_stack_size ; i++ )
        {
            edit_info_stack[i].max_features = 0;
            edit_info_stack[i].undo_stack = NULL;
            edit_info_stack[i].redo_stack = NULL;
        }
    }

    info = edit_info_stack + stack_pos;
    stack_pos++;

    info->window = window;
    info->inst = inst;
    info->obj = inst->the_instance;

    if ( info->obj->o_num_features > minor_feature )
    {
        info->axes.x = info->obj->o_features[major_feature].location;
        info->axes.y = info->obj->o_features[minor_feature].location;
        VCross(info->axes.x, info->axes.y, info->axes.z);
    }
    else
        NewIdentityMatrix(info->axes);
    info->axes_inverse = MInvert(&(info->axes));

    if ( info->max_features < info->obj->o_num_features )
    {
        if ( info->max_features )
            info->features = More(info->features, ConstraintPoint,
                                  info->obj->o_num_features);
        else
            info->features = New(ConstraintPoint, info->obj->o_num_features);
        info->max_features = info->obj->o_num_features;
    }
    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
    {
        VAdd(info->obj->o_features[i].location,
             info->obj->o_transform.displacement,
             info->features[i].world);
        info->features[i].num_view = 0;
        info->features[i].view = NULL;
        info->features[i].def_pts.num_pts =
        info->features[i].def_pts.num_def_pts = 0;
    }

    info->selecting = info->modifying = FALSE;

    info->drag_type = NO_DRAG;

    return info;
}


void
Edit_Clear_Info(EditInfoPtr info)
{
    int i;

    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
    {
        if ( info->features[i].num_view )
            free(info->features[i].view);
        if ( info->features[i].def_pts.num_pts )
            free(info->features[i].def_pts.pts);
        if ( info->features[i].def_pts.num_def_pts )
            free(info->features[i].def_pts.def_pts);
    }

    stack_pos--;
}


static void
Edit_Prepare_Instances(EditInfoPtr info)
{
    InstanceList    temp_list = NULL;
    InstanceList    axis_list = NULL;

    /* Sets up the instance lists for point selection. */

    /* The reference_available list is the object itself, but with new
    ** vertices.
    */
    Insert_Element(&temp_list, info->obj);
    info->reference_available = temp_list;

    /* The all_available list is a list of all visible objects + the
    ** axes object.
    */
    Insert_Element(&axis_list, &(info->window->axes));
    if ( info->window == &main_window &&
         ( camera_object.o_flags & ObjVisible ) )
        Insert_Element(&axis_list, &camera_object);
    info->other_available = Merge_Selection_Lists(axis_list,
                                info->window->all_instances);
    Free_Selection_List(axis_list);

    /* The all list currently has the old object in it, which we
    ** don't want.
    */
    if ( ( temp_list =
           Find_Object_In_Instances(info->obj, info->other_available)))
    {
        if ( temp_list == info->other_available )
            info->other_available = info->other_available->next;
        Delete_Instance(temp_list);
    }

    /* We wish to add the obj itself at the front of the list. */
    temp_list = NULL;
    Insert_Element(&temp_list, info->obj);
    Append_Instance_List(&temp_list, info->other_available);
    info->all_available = temp_list;
}



EditInfoPtr
Edit_Get_Info()
{
    if ( ! stack_pos )
        return NULL;

    return edit_info_stack + ( stack_pos - 1);
}


void
Edit_Draw_Selection_Points(EditInfoPtr info)
{
    Draw_All_Constraints(info->window, ViewNone, info, FALSE);
    Draw_Edit_Extras(info->window, ViewNone, info, TRUE);

    Prepare_Selection_Drawing();

    Select_Highlight_Closest(XtWindow(info->window->view_widget));
    Draw_Selection_Points(XtWindow(info->window->view_widget));
}


void
Edit_Cleanup_Selection(EditInfoPtr info, Boolean draw)
{
    info->selecting = FALSE;

    Cancel_Select_Operation(info->window);

    Edit_Sensitize_Buttons(TRUE, SenEditAll);
    if ( draw )
    {
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
        Draw_All_Constraints(info->window, ViewNone, info, TRUE);
    }
}


void
Edit_Transform_Vertices(Transformation *trans, EditInfoPtr info)
{
    Vector  *orig;
    int     i;

    if ( Obj_Is_Torus(info->obj) )
    {
        Torus_Calculate_Vertices(info->obj->o_world_verts,
                                 info->obj->o_num_vertices,
                                 ((TorusPtr)info->obj->o_hook)->major_radius);
        Transform_Vertices(*trans, info->obj->o_world_verts,
                           info->obj->o_num_vertices);
    }
    else if ( Obj_Is_Control(info->obj) )
    {
        control_part(info->obj)->calc_verts(info->obj, info->obj->o_world_verts,
					    info->obj->o_num_real,
					    info->obj->o_num_vertices);
        Transform_Vertices(*trans, info->obj->o_world_verts,
                           info->obj->o_num_vertices);
    }
    else
    {
        orig = info->obj->o_wireframe->vertices;
        for ( i = 0 ; i < info->obj->o_num_vertices ; i++ )
            Transform_Vector(*trans, orig[i], info->obj->o_world_verts[i])
    }
}


void
Edit_Transform_Normals(Transformation *trans, EditInfoPtr info)
{
    Matrix  transp;
    Face    *face;
    double  temp_d;
    int     i;
    Vector  normal;

    info->obj->o_inverse = MInvert(&(trans->matrix));
    MTrans(info->obj->o_inverse, transp);
    if ( Obj_Is_Triangle(info->obj) )
    {
        control_part(info->obj)->calc_face_norms(info->obj, &normal);
        MVMul(transp, normal, info->obj->o_normals[0]);
        if ( ! VZero(info->obj->o_normals[0]) )
            VUnit(info->obj->o_normals[0], temp_d, info->obj->o_normals[0]);
    }
    else if ( Obj_Is_Bezier(info->obj) )
        control_part(info->obj)->calc_face_norms(info->obj,
						 info->obj->o_normals);
    else
    {
        face = info->obj->o_wireframe->faces;
        for ( i = 0 ; i < info->obj->o_num_faces ; i++ )
        {
            MVMul(transp, face[i].normal, info->obj->o_normals[i]);
            if ( ! VZero(info->obj->o_normals[i]) )
                VUnit(info->obj->o_normals[i], temp_d, info->obj->o_normals[i]);
        }
    }
}


static void
Edit_Calc_Line_Points(Vector *verts, Vector point, Vector dir, double mag)
{
    VScalarMul(dir, sced_resources.line_con_length * 50 / mag, dir);
    VAdd(point, dir, verts[0]);
    VSub(point, dir, verts[1]);
}


static void
Edit_Calc_Plane_Points(Vector *verts, Vector point, Vector norm, double mag)
{
    Vector  axis;
    Vector  dir1, dir2;
    double  temp_d;

    /* Need 2 vectors perp to the normal and each other. */
    VNew(1, 0, 0, axis);
    VCross(norm, axis, dir1);
    if ( VZero(dir1) )
    {
        VNew(0, 1, 0, axis);
        VCross(norm, axis, dir1);
    }
    VCross(dir1, norm, dir2);
    VUnit(dir1, temp_d, dir1);
    VUnit(dir2, temp_d, dir2);
    VScalarMul(dir1, sced_resources.plane_con_length * 50 / mag, dir1);
    VScalarMul(dir2, sced_resources.plane_con_length * 50 / mag, dir2);

    VAdd(point, dir1, verts[0]);
    VAdd(point, dir2, verts[1]);
    VSub(point, dir1, verts[2]);
    VSub(point, dir2, verts[3]);
}

static void
Edit_Calculate_Arc_Points(WindowInfoPtr window, Vector axis,
                          int center_x, int center_y, double radius,
                          XPoint **pts, int *num_pts, Boolean remove_back,
                          Boolean append, Boolean arcball)
{
    double  angle_diff;
    Vector  view_axis;
    Matrix  inverse_trans;
    Vector  start_pt;
    static Vertex   *arc_views = NULL;
    Matrix  rotation;
    Quaternion  rot_quat;
    XPoint  offset;
    double  temp_d;
    int     start_num;
    int     i;

    /* Get the axis in view.  It's the same as transforming a normal. */
    MTrans(window->viewport.view_to_world.matrix, inverse_trans);
    MVMul(inverse_trans, axis, view_axis);

    VUnit(view_axis, temp_d, view_axis);

    if ( ! arc_views )
        arc_views = New(Vertex, ARC_DIVISIONS + 1);

    /* Find a point on the sphere perp to the axis through the center. */
    temp_d = sqrt(1.0 - view_axis.z * view_axis.z);
    if ( IsZero(temp_d) )
    {
        start_pt.x = 1.0;
        start_pt.y = 0.0;
        start_pt.z = 0.0;
    }
    else
    {
        start_pt.x = view_axis.y / temp_d;
        start_pt.y = -view_axis.x / temp_d;
        start_pt.z = 0.0;
    }

    /* Find all the points. */
    angle_diff = 2 * M_PI / ARC_DIVISIONS;
    rot_quat.real_part = cos(angle_diff / 2);
    VScalarMul(view_axis, -1.0, rot_quat.vect_part);
    temp_d = sin(angle_diff / 2);
    VScalarMul(rot_quat.vect_part, temp_d, rot_quat.vect_part);
    rotation = Quaternion_To_Matrix(rot_quat);

    arc_views[0].view = start_pt;
    for ( i = 1 ; i < ARC_DIVISIONS ; i++ )
        MVMul(rotation, arc_views[i-1].view, arc_views[i].view);
    arc_views[i].view = start_pt;

    if ( arcball )
        for ( i = 0 ; i <= ARC_DIVISIONS ; i++ )
        {
            arc_views[i].screen.x = ( window->width >> 1 ) +
                                    arc_views[i].view.x * radius;
            arc_views[i].screen.y = ( window->height >> 1 ) -
                                    arc_views[i].view.y * radius;
        }
    else
        Convert_View_To_Screen(arc_views, ARC_DIVISIONS + 1,
                    &(window->viewport), (short)window->width,
                    (short)window->height, radius);

    if ( append )
    {
        if ( *num_pts == 0 )
            *pts = New(XPoint, ARC_DIVISIONS + 1);
        else
            *pts = More(*pts, XPoint, *num_pts + ARC_DIVISIONS + 1);
        start_num = *num_pts;
    }
    else
    {
        if ( *num_pts == 0 )
            *pts = New(XPoint, ARC_DIVISIONS + 1);
        else if ( *num_pts < ARC_DIVISIONS + 1 )
            *pts = More(*pts, XPoint, ARC_DIVISIONS + 1);
        *num_pts = 0;
        start_num = 0;
    }

    if ( remove_back )
    {
        if ( arc_views[0].view.z < arc_views[1].view.z )
        {
            for ( i = ARC_DIVISIONS - 1 ; i >= 0 ; i-- )
                if ( arc_views[i].view.z <= 0.0 )
                    (*pts)[(*num_pts)++] = arc_views[i].screen;
        }
        else
        {
            for ( i = 0 ; i < ARC_DIVISIONS + 1 ; i++ )
                if ( arc_views[i].view.z <= 0.0 )
                    (*pts)[(*num_pts)++] = arc_views[i].screen;
        }
        (*num_pts)--;
    }
    else
    {
        for ( i = 0 ; i < ARC_DIVISIONS + 1 ; i++ )
            (*pts)[(*num_pts)++] = arc_views[i].screen;
    }

    offset.x = center_x - window->width / 2;
    offset.y = center_y - window->height / 2;
    for ( i = start_num ; i < (*num_pts) ; i++ )
    {
        (*pts)[i].x += offset.x;
        (*pts)[i].y += offset.y;
    }
}


static void
Edit_Calculate_Constraint_Points(EditInfoPtr info, ResultantPtr resultant,
                                 Vector center, int center_x, int center_y,
                                 XPoint **pts, int *num_pts, XArc *sphere_arc)
{
    Vector  worlds[4];
    Vertex  views[4];
    int     i;

    /* Calculate the 4 screen points that define an origin constraint. */
    /* If the resultant constraint is a line, then 0 & 2 and 1 & 3 will be
    ** identical. */

    switch ( resultant->feature_1.c_type )
    {
        case plane_feature:
            if ( *num_pts < 4 )
            {
                if ( *num_pts )
                    *pts = More(*pts, XPoint, 4);
                else
                    *pts = New(XPoint, 4);
            }
            *num_pts = 4;
            Edit_Calc_Plane_Points(worlds, center,
                                   resultant->feature_1.c_vector,
                                   info->window->magnify);
            Convert_World_To_View(worlds, views, 4,
                                  &(info->window->viewport));
            Convert_View_To_Screen(views, 4, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            for ( i = 0 ; i < 4 ; i++ )
                (*pts)[i] = views[i].screen;
            break;

        case line_feature:
            if ( *num_pts < 2 )
            {
                if ( *num_pts )
                    *pts = More(*pts, XPoint, 2);
                else
                    *pts = New(XPoint, 2);
            }
            *num_pts = 2;
            Edit_Calc_Line_Points(worlds, center,
                                  resultant->feature_1.c_vector,
                                  info->window->magnify);
            Convert_World_To_View(worlds, views, 2,
                                  &(info->window->viewport));
            Convert_View_To_Screen(views, 2, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            for ( i = 0 ; i < 2 ; i++ )
                (*pts)[i] = views[i].screen;
            break;

        case point_feature:
            if ( *num_pts < 2 )
            {
                if ( *num_pts )
                    *pts = More(*pts, XPoint, 2);
                else
                    *pts = New(XPoint, 2);
            }
            *num_pts = 2;
            Convert_World_To_View(&(resultant->feature_1.c_point), views, 1,
                                  &(info->window->viewport));
            Convert_View_To_Screen(views, 1, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            (*pts)[0] = views->screen;
            if ( resultant->feature_2.c_type == point_feature )
            {
                Convert_World_To_View(&(resultant->feature_2.c_point), views, 1,
                                      &(info->window->viewport));
                Convert_View_To_Screen(views, 1, &(info->window->viewport),
                                       (short)info->window->width,
                                       (short)info->window->height,
                                       info->window->magnify);
                (*pts)[1] = views->screen;
            }
            break;

        case circle_feature: {
            double  screen_rad;

            Convert_World_To_View(&(resultant->feature_1.c_point), views, 1,
                                  &(info->window->viewport));
            Convert_View_To_Screen(views, 1, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            screen_rad = resultant->feature_1.c_radius *
                         info->window->magnify *
                         info->window->viewport.eye_distance /
                         ( info->window->viewport.eye_distance +
                           views->view.z );
            Edit_Calculate_Arc_Points(info->window,
                                      resultant->feature_1.c_vector,
                                      views[0].screen.x, views[0].screen.y,
                                      screen_rad, pts, num_pts, FALSE, FALSE,
                                      FALSE);
            }
            break;

        case sphere_feature: {
            Vector  axis;
            double  screen_rad;

            Convert_World_To_View(&(resultant->feature_1.c_point), views, 1,
                                  &(info->window->viewport));
            Convert_View_To_Screen(views, 1, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            screen_rad = resultant->feature_1.c_radius *
                         info->window->magnify *
                         info->window->viewport.eye_distance /
                         ( info->window->viewport.eye_distance +
                           views->view.z );
            VNew(1, 0, 0, axis);
            Edit_Calculate_Arc_Points(info->window, axis,
                                      views[0].screen.x, views[0].screen.y,
                                      screen_rad, pts, num_pts, FALSE, FALSE,
                                      FALSE);
            VNew(0, 1, 0, axis);
            Edit_Calculate_Arc_Points(info->window, axis,
                                      views[0].screen.x, views[0].screen.y,
                                      screen_rad, pts, num_pts, FALSE, TRUE,
                                      FALSE);
            VNew(0, 0, 1, axis);
            Edit_Calculate_Arc_Points(info->window, axis,
                                      views[0].screen.x, views[0].screen.y,
                                      screen_rad, pts, num_pts, FALSE, TRUE,
                                      FALSE);

            sphere_arc->x = views[0].screen.x - (int)screen_rad;
            sphere_arc->y = views[0].screen.y - (int)screen_rad;
            sphere_arc->width = 2 * (int)screen_rad;
            sphere_arc->height = 2 * (int)screen_rad;
            sphere_arc->angle1 = 0;
            sphere_arc->angle2 = 23040;

            }
            break;

        case inconsistent_feature:
            if ( *num_pts < 4 )
            {
                if ( *num_pts )
                    *pts = More(*pts, XPoint, 4);
                else
                    *pts = New(XPoint, 4);
            }
            *num_pts = 4;
            (*pts)[0].x = center_x + sced_resources.incon_con_length;
            (*pts)[0].y = center_y + sced_resources.incon_con_length;
            (*pts)[1].x = center_x - sced_resources.incon_con_length;
            (*pts)[1].y = center_y - sced_resources.incon_con_length;
            (*pts)[2].x = center_x + sced_resources.incon_con_length;
            (*pts)[2].y = center_y - sced_resources.incon_con_length;
            (*pts)[3].x = center_x - sced_resources.incon_con_length;
            (*pts)[3].y = center_y + sced_resources.incon_con_length;
            break;

        default:;
    }
}


void
Edit_Calculate_Cons_Points(EditInfoPtr info, int num)
{
    Edit_Calculate_Constraint_Points(info, &(info->features[num].resultant),
                                     info->features[num].world,
                                     (int)info->features[num].view->screen.x,
                                     (int)info->features[num].view->screen.y,
                                     &(info->features[num].def_pts.pts),
                                     &(info->features[num].def_pts.num_pts),
                                     &(info->features[num].def_pts.sphere_arc));

    /* Build all the constraint description points. */
    Edit_Calculate_Spec_Points(info,
        info->obj->o_features[num].constraints + 3,
        info->obj->o_features[num].num_constraints - 3,
        &(info->features[num].def_pts.def_pts),
        &(info->features[num].def_pts.num_def_pts), TRUE);
}

void
Edit_Calculate_Axis_Cons_Points(EditInfoPtr info, int num)
{
    if ( info->features[num].resultant.feature_1.c_type == line_feature )
        Edit_Calculate_Arc_Points(info->window,
                    info->features[num].resultant.feature_1.c_vector,
                    info->features[origin_feature].view->screen.x,
                    info->features[origin_feature].view->screen.y,
                    info->features[major_feature].circle.width >> 1,
                    &(info->features[num].def_pts.pts),
                    &(info->features[num].def_pts.num_pts),
                    TRUE, FALSE, TRUE);
    else
    {
        if ( info->features[num].def_pts.num_pts )
            free(info->features[num].def_pts.pts);
        info->features[num].def_pts.num_pts = 0;
    }

    Edit_Calculate_Spec_Points(info,
        info->obj->o_features[num].constraints + 1,
        info->obj->o_features[num].num_constraints - 1,
        &(info->features[num].def_pts.def_pts),
        &(info->features[num].def_pts.num_def_pts), TRUE);
}


static void
Edit_Add_Spec_Vert(EditInfoPtr info, ConstraintSpecPtr spec, Vertex **verts,
                ConstraintSpecType **types, int *num_verts, int *max_num_verts)
{
    Vector  world;

    switch ( spec->spec_type )
    {
        case absolute_spec:
            world = spec->spec_vector;
            break;
        case offset_spec:
            VAdd(spec->spec_vector,
                 info->obj->o_world_verts[info->obj->o_num_vertices-1],
                 world);
            break;
        case reference_spec:
            Ref_Transform_Vector(spec_object(spec), spec->spec_vector, world);
            break;
        case vertex_spec:
            Ref_Transform_Vector(spec_object(spec), 
                                 control_part(spec_object(spec))->
                                    control_verts[(int)spec->spec_vector.x],
                                 world);
            break;
        case parameter_spec:
            world = Bezier_Calculate_Point(spec_object(spec),
                                           spec->spec_vector.x,
                                           spec->spec_vector.y);
            break;
        default:
            return;
    }

    if ( *num_verts >= *max_num_verts )
    {
        if ( *max_num_verts )
        {
            *max_num_verts += 10;
            *verts = More((*verts), Vertex, (*max_num_verts));
            *types = More((*types), ConstraintSpecType, (*max_num_verts));
        }
        else
        {
            *max_num_verts = 10;
            *verts = New(Vertex, (*max_num_verts));
            *types = New(ConstraintSpecType, (*max_num_verts));
        }
    }

    Convert_World_To_View(&(world), *verts + *num_verts, 1,
                          &(info->window->viewport));
    (*types)[(*num_verts)++] = spec->spec_type;
}


/*
**  Edit_Add_Spec_Point(...)
**  Adds the defining points for the constraint to the list of verts.
*/
static void
Edit_Add_Spec_Point(EditInfoPtr info, ConstraintPtr feat, Vertex **verts,
                ConstraintSpecType **specs, int *num_verts, int *max_num_verts)
{
    int i;
    for ( i = 0 ; i < feat->c_num_specs ; i++ )
        Edit_Add_Spec_Vert(info, feat->c_specs + i, verts, specs, num_verts,
                            max_num_verts);
}


/*  void
**  Edit_Calculate_Spec_Points(...)
**  Calculates the locations on screen of the points used to specify
**  constraints.
*/
static void
Edit_Calculate_Spec_Points(EditInfoPtr info, ConstraintPtr feats, int num_feats,
                           ConPoint **pts, int *num_pts, Boolean replace)
{
    static Vertex   *verts;
    static ConstraintSpecType   *specs;
    static int      num_verts;
    static int      max_num_verts = 0;
    int             i, j;
    Boolean         found;


    if ( replace )
    {
        if ( *num_pts )
            free(*pts);
        *num_pts = 0;
    }

    num_verts = 0;
    for ( i = 0 ; i < num_feats ; i++ )
    {
        if ( feats[i].c_status || feats[i].c_forced )
            Edit_Add_Spec_Point(info, feats + i, &verts, &specs, &num_verts,
                                &max_num_verts);
    }

    if ( ! num_verts )
        return;

    Convert_View_To_Screen(verts, num_verts, &(info->window->viewport),
                           (short)info->window->width,
                           (short)info->window->height,
                           info->window->magnify);

    if ( *num_pts )
        *pts = More(*pts, ConPoint, num_verts + *num_pts);
    else
        *pts = New(ConPoint, num_verts);
    for ( i = 0 ; i < num_verts ; i++ )
    {
        found = FALSE;
        for ( j = 0 ; ! found && j < *num_pts ; j++ )
            if ( (*pts)[j].pt.x == verts[i].screen.x &&
                 (*pts)[j].pt.y == verts[i].screen.y )
                found = TRUE;
        if ( found ) continue;
        (*pts)[(*num_pts)].pt = verts[i].screen;
        (*pts)[(*num_pts)++].type = specs[i];
    }
}

/*  Vector
**  Map_Point_Onto_Plane(XPoint *pt, ConstraintPtr plane, Viewport *vp,
**                       short width, short height, int mag)
**  Maps the point pt onto the plane plane, specified in view.  Returns the
**  world coordinates of pt assuming it lies in the plane plane.
*/
Vector
Map_Point_Onto_Plane(XPoint *pt, ConstraintPtr plane, Viewport *vp, short width,
                     short height, int mag)
{
    Vector  result;
    short   mid_x = width / 2;
    short   mid_y = height / 2;
    double  temp_d;

    result.x = (double)(pt->x - mid_x) / (double)mag;
    result.y = (double)(mid_y - pt->y) / (double)mag;

    temp_d = result.x * plane->c_vector.x + result.y * plane->c_vector.y;
    result.z = vp->eye_distance *
                ( VDot(plane->c_vector, plane->c_point ) - temp_d ) /
                (temp_d + vp->eye_distance * plane->c_vector.z );
    temp_d = (vp->eye_distance + result.z) / vp->eye_distance;
    result.x *= temp_d;
    result.y *= temp_d;

    Transform_Vector(vp->view_to_world, result, result);

    return result;
}



/*  Vector
**  Map_Point_Onto_Line(XPoint *pt, ConstraintPtr line, Viewport *vp,
**                       short width, short height, int mag)
**  Maps the point pt onto the line line, specified in screen.  Returns the
**  world coordinates of pt assuming it lies on the line.
**  The mapping drops a perpendicular from the point onto the line in
**  screen, and returns the point of intersection of the perpendicular
**  with the line converted back into world.
*/
Vector
Map_Point_Onto_Line(XPoint *pt, ConstraintPtr line, Viewport *vp, short width,
                     short height, int mag)
{
    Vector  scr_line_dir;
    Vector  scr_line_pt;
    Vector  view_pt_1;
    Vector  view_pt_2;
    Vector  screen_pt_1;
    Vector  screen_pt_2;
    Vector  perp_dir;
    Vector  perp_pt;
    Vector  result;
    double  temp_d1;
    Vector  temp_v1;
    double  mid_x = width / 2;
    double  mid_y = height / 2;

    /* Convert the line in view into screen.*/
    VScalarMul(line->c_vector, 10, temp_v1);
    VAdd(line->c_point, temp_v1, view_pt_1);
    view_pt_2 = line->c_point;
    temp_d1 = fabs(vp->eye_distance / (vp->eye_distance + view_pt_1.z)) *
                (double)mag;
    screen_pt_1.x = mid_x + view_pt_1.x * temp_d1;
    screen_pt_1.y = mid_y - view_pt_1.y * temp_d1;
    screen_pt_1.z = view_pt_1.z * temp_d1;
    temp_d1 = fabs(vp->eye_distance / (vp->eye_distance + view_pt_2.z)) *
                (double)mag;
    screen_pt_2.x = mid_x + view_pt_2.x * temp_d1;
    screen_pt_2.y = mid_y - view_pt_2.y * temp_d1;
    screen_pt_2.z = view_pt_2.z * temp_d1;

    VSub(screen_pt_1, screen_pt_2, scr_line_dir);
    scr_line_pt = screen_pt_2;

    /* Find the direction of the perpendicular plane normal. */
    perp_dir.x = scr_line_dir.x;
    perp_dir.y = scr_line_dir.y;
    perp_dir.z = 0.0;
    VNew(pt->x, pt->y, 0, perp_pt);

    /* Intersect the perp with the line. */
    temp_d1 = ( VDot(perp_dir, perp_pt) - VDot(perp_dir, scr_line_pt) ) /
              VDot(perp_dir, scr_line_dir);
    VScalarMul(scr_line_dir, temp_d1, result);
    VAdd(result, scr_line_pt, screen_pt_1);

    /* Result is in screen. Take it back to view. */
    result.z = ( screen_pt_1.z * vp->eye_distance ) /
               ( vp->eye_distance * (double)mag - screen_pt_1.z );
    temp_d1 = ( vp->eye_distance + result.z ) /
              ( vp->eye_distance * (double)mag );
    result.x = temp_d1 * ( screen_pt_1.x - mid_x );
    result.y = temp_d1 * ( mid_y - screen_pt_1.y );

    Transform_Vector(vp->view_to_world, result, result);

    return result;
}


Boolean
Get_Closest_Screen_Pt(ResultantPtr res, XPoint *pt1, XPoint *pt2,
                      XPoint *new_pt, Vector *new_world, int *preferred)
{
    Vector  diff;
    double  dist1, dist2;

    dist1 = ( pt1->x - new_pt->x ) * ( pt1->x - new_pt->x ) +
            ( pt1->y - new_pt->y ) * ( pt1->y - new_pt->y );
    dist2 = ( pt2->x - new_pt->x ) * ( pt2->x - new_pt->x ) +
            ( pt2->y - new_pt->y ) * ( pt2->y - new_pt->y );

    if ( dist1 <= dist2 )
    {
        *preferred = 1;
        if ( VEqual(*new_world, res->feature_1.c_point, diff) )
            return FALSE;
        else
            *new_world = res->feature_1.c_point;
    }
    else
    {
        *preferred = 2;
        if ( VEqual(*new_world, res->feature_2.c_point, diff) )
            return FALSE;
        else
            *new_world = res->feature_2.c_point;
    }

    return TRUE;
}


Vector
Map_Point_Onto_Circle(XPoint *pt, ConstraintPtr circle,
                      ConstraintPtr world_circle,
                      Viewport *vp, short width, short height, int mag)
{
    Vector  plane_pt = Map_Point_Onto_Plane(pt, circle, vp, width, height, mag);
    Vector  pt_dir;
    Vector  result;
    double  temp_d;

    /* Project the plane point onto the circle. */
    VSub(plane_pt, world_circle->c_point, pt_dir);

    if ( VZero(pt_dir) )
    {
        Vector  temp_v;

        VNew(1, 0, 0, temp_v);
        VCross(temp_v, world_circle->c_vector, pt_dir);
        if ( VZero(pt_dir) )
        {
            VNew(0, 1, 0, temp_v);
            VCross(temp_v, world_circle->c_vector, pt_dir);
        }
    }

    VUnit(pt_dir, temp_d, pt_dir);
    VScalarMul(pt_dir, world_circle->c_radius, result);
    VAdd(result, world_circle->c_point, result);

    return result;
}


Vector
Map_Point_Onto_Sphere(XPoint *pt, ConstraintPtr sphere, XPoint *center_pt,
                      Viewport *vp, int mag)
{
    Vector  sphere_pt;
    double  pt_rad, pt_rad_sq;
    Vector  result;

    /* We wish to map the screen point onto the sphere centered on
    ** the screen point of the center of the constraint sphere, with
    ** radius = radius * mag. */

    sphere_pt.x = ( pt->x - center_pt->x ) * 2 / (double)mag;
    sphere_pt.y = ( center_pt->y - pt->y ) * 2 / (double)mag;
    pt_rad_sq = sphere_pt.x * sphere_pt.x + sphere_pt.y * sphere_pt.y;

    pt_rad = sqrt(pt_rad_sq);
    if ( pt_rad > 2 * sphere->c_radius )
    {
        sphere_pt.x = sphere_pt.y = 0.0;
        sphere_pt.z = sphere->c_radius;
    }
    else if ( pt_rad <= sphere->c_radius )
        sphere_pt.z = - sqrt( sphere->c_radius * sphere->c_radius - pt_rad_sq );
    else
    {
        double  new_rad;

        new_rad = 2 * sphere->c_radius - pt_rad;
        pt_rad_sq = new_rad * new_rad;
        sphere_pt.z = sqrt( sphere->c_radius * sphere->c_radius - pt_rad_sq );
        sphere_pt.x *= ( new_rad / pt_rad );
        sphere_pt.y *= ( new_rad / pt_rad );
    }

    MVMul(vp->view_to_world.matrix, sphere_pt, result);

    VAdd(result, sphere->c_point, result);

    result = Closest_Sphere_Point(&result, NULL, &(sphere->c_point), NULL,
                                  &(sphere->c_radius));
    return result;
}


void
Edit_Start_Drag(Widget w, XEvent *e, String *s, Cardinal *c)
{
    EditInfoPtr info = Edit_Get_Info();
    int         min_priority = 0x7FFFFFFF;
    FeatureType menu_feature, temp_f;

    switch ( e->xbutton.button )
    {
      case 1:
        if ( active_feature != pt_last_feature )
            info->obj->o_features[active_feature].base->drag_start_func(e,
                                                        active_feature, info);
        break;
      case 2:
      case 3:
        menu_feature = pt_last_feature ;
        for ( temp_f = origin_feature ;
              temp_f < info->obj->o_num_features ;
              temp_f++ )
        {
            if ( ! info->obj->o_features[temp_f].base ) continue;
            if ( info->obj->o_features[temp_f].base->select_priority <
                    min_priority
                 && info->obj->o_features[temp_f].base->covers_func(
                        e->xbutton.x, e->xbutton.y, temp_f, info) )
            {
                menu_feature = temp_f;
                min_priority =
                    info->obj->o_features[temp_f].base->select_priority;
            }
        }
        if ( menu_feature == pt_last_feature )
        {
            if ( ! edit_command_menu )
                Edit_Create_Command_Menu();
            Position_Shell(edit_command_menu, FALSE);
            XtPopup(edit_command_menu, XtGrabExclusive);
            break;
        }
        info->current_feature = menu_feature;
        if ( e->xbutton.button == 2 )
        {
          if ( info->obj->o_features[menu_feature].base->change_shell )
          {
            Position_Shell(
                info->obj->o_features[menu_feature].base->change_shell, TRUE);
            XtPopup(info->obj->o_features[menu_feature].base->change_shell,
                    XtGrabExclusive);
          }
        }
        else
        {
            Position_Shell(
                info->obj->o_features[menu_feature].base->add_shell, TRUE);
            XtPopup(info->obj->o_features[menu_feature].base->add_shell,
                    XtGrabExclusive);
        }
        break;
    }
}


void
Edit_Continue_Drag(Widget w, XEvent *e, String *s, Cardinal *c)
{
    EditInfoPtr info = Edit_Get_Info();

    if ( info->drag_type != NO_DRAG && active_feature != pt_last_feature )
        info->obj->o_features[active_feature].base->drag_func(e, active_feature,
                                                              info);
    else
        Edit_Set_Cursor_Action(w, e, s, c);
}


void
Edit_Finish_Drag(Widget w, XEvent *e, String *s, Cardinal *c)
{
    EditInfoPtr info = Edit_Get_Info();

    if ( info->drag_type != NO_DRAG && active_feature != pt_last_feature )
        info->obj->o_features[active_feature].base->drag_end_func(e,
                                                        active_feature, info);

    info->drag_type = NO_DRAG;
}


void
Edit_Set_Cursor_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
    EditInfoPtr info = Edit_Get_Info();
    Cursor      new_cursor;
    int         min_priority = 0x7FFFFFFF;
    FeatureType temp_f;

    if ( info->selecting ) return;

    if ( ! no_motion_cursor )
        no_motion_cursor = XCreateFontCursor(XtDisplay(info->window->shell),
                                             XC_circle);

    /* Set the cursor according to what the resultant action will be */
    active_feature = pt_last_feature ;
    for ( temp_f = origin_feature ;
          temp_f < info->obj->o_num_features ;
          temp_f++ )
    {
        if ( ! info->obj->o_features[temp_f].base ) continue;
        if ( info->obj->o_features[temp_f].base->select_priority < min_priority
             && info->obj->o_features[temp_f].base->select_func(
                    e->xbutton.x, e->xbutton.y, temp_f, info) &&
             info->obj->o_features[temp_f].base->select_active(temp_f, info) )
        {
            active_feature = temp_f;
            min_priority = info->obj->o_features[temp_f].base->select_priority;
        }
    }

    if ( active_feature == pt_last_feature )
        new_cursor = no_motion_cursor;
    else
        new_cursor = info->obj->o_features[active_feature].base->select_cursor;

    if ( old_cursor != new_cursor )
    {
        old_cursor = new_cursor;
        XDefineCursor(XtDisplay(info->window->shell),
                    XtWindow(info->window->view_widget), new_cursor);
    }
}

void
Edit_Clear_Cursor(WindowInfoPtr window)
{
    XDefineCursor(XtDisplay(window->shell), XtWindow(window->view_widget),None);
    old_cursor = 0;
}

