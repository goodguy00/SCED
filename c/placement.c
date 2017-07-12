/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/placement.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: placement.c,v $
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
**  placement.c : Functions regarding object placement.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <update.h>


static void Edit_Move_Object(EditInfoPtr);

static ConstraintData   view_feature;

static XPoint           drag_offset;
static Vector           start_center;
static Transformation   init_obj_transform;

static XPoint   constr_pt_1;
static XPoint   constr_pt_2;

void
Edit_Extract_Resultant_Data(EditInfoPtr info, int feature,
                            ConstraintData *view_data, XPoint *pt_1,
                            XPoint *pt_2)
{
    double val;
    Vector vect;
    Vertex  vert;

    switch ( info->features[feature].resultant.feature_1.c_type )
    {
        case plane_feature:
        case circle_feature:
            /* Test that the constraint plane is not perp to the screen. */
            val = VDot(
                    info->features[feature].resultant.feature_1.c_vector,
                    info->window->viewport.world_to_view.matrix.z);
            if ( IsZero(val) ) return;

            /* Bring the plane into view coords, to make the mapping simpler. */
            Convert_Plane_World_To_View(
                    &(info->features[feature].resultant.feature_1),
                    &(info->window->viewport), view_data);
            break;

        case line_feature:
            /* Test that the constraint line is not perp to the screen. */
            VCross(info->features[feature].resultant.feature_1.c_vector,
                   info->window->viewport.world_to_view.matrix.z, vect);
            if ( VZero(vect) ) return;

            /* Bring the line into view coords. */
            Convert_Line_World_To_View(
                    &(info->features[feature].resultant.feature_1),
                    &(info->window->viewport), view_data);

            break;

        case point_feature: {
            Convert_World_To_View(
                &(info->features[feature].resultant.feature_1.c_point),
                &vert, 1, &(info->window->viewport));
            Convert_View_To_Screen(&vert, 1, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            *pt_1 = vert.screen;

            Convert_World_To_View(
                &(info->features[feature].resultant.feature_2.c_point),
                &vert, 1, &(info->window->viewport));
            Convert_View_To_Screen(&vert, 1, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            *pt_2 = vert.screen;
            }
            break;

        case sphere_feature: {
            Convert_World_To_View(
                &(info->features[feature].resultant.feature_1.c_point),
                &vert, 1, &(info->window->viewport));
            Convert_View_To_Screen(&vert, 1, &(info->window->viewport),
                                   (short)info->window->width,
                                   (short)info->window->height,
                                   info->window->magnify);
            *pt_1 = vert.screen;
            }
            break;

        default:;
    }
}


Vector
Edit_Obtain_New_Point(EditInfoPtr info, int feature, XPoint screen_pt,
                      ConstraintData *view_data, XPoint *pt_1, XPoint *pt_2)
{
    Vector  result;
    int     preferred;

    switch ( info->features[feature].resultant.feature_1.c_type )
    {
        case plane_feature:
            result =
                Map_Point_Onto_Plane(&screen_pt, view_data,
                            &(info->window->viewport),
                            (short)info->window->width,
                            (short)info->window->height,
                            info->window->magnify);
            break;

        case line_feature:
            result =
                Map_Point_Onto_Line(&screen_pt, view_data,
                            &(info->window->viewport),
                            (short)info->window->width,
                            (short)info->window->height,
                            info->window->magnify);
            break;

        case point_feature:
            Get_Closest_Screen_Pt(&(info->features[feature].resultant),
                          pt_1, pt_2,
                          &screen_pt, &result, &preferred);
            if ( preferred == 1 )
            {
                info->obj->o_features[feature].flags &=
                    ( FeatureAll ^ FeaturePref2 );
                info->obj->o_features[feature].flags |= FeaturePref1;
            }
            else
            {
                info->obj->o_features[feature].flags &=
                    ( FeatureAll ^ FeaturePref1 );
                info->obj->o_features[feature].flags |= FeaturePref2;
            }
            break;

        case circle_feature:
            result =
                Map_Point_Onto_Circle(&screen_pt, view_data,
                        &(info->features[feature].resultant.feature_1),
                        &(info->window->viewport),
                        (short)info->window->width, (short)info->window->height,
                        info->window->magnify);
            break;

        case sphere_feature:
            result =
                Map_Point_Onto_Sphere(&screen_pt,
                        &(info->features[feature].resultant.feature_1),
                        pt_1,
                        &(info->window->viewport), info->window->magnify);
            break;

        default:;
    }

    return result;
}

/*  void
**  Edit_Start_Origin_Drag(XEvent *e, EditInfoPtr info)
**  Registers the start of a origin motion drag. Checks that such a drag
**  is possible, ie right constraints and no special conditions, then sets
**  appropriate flags.
*/
void
Edit_Start_Origin_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  dummy;

    if ( ! Interactive_Resultant(info->features[origin_feature].resultant) )
        return;

    Edit_Extract_Resultant_Data(info, origin_feature,
                                &view_feature, &constr_pt_1, &constr_pt_2);

    NewIdentityMatrix(info->drag_transform.matrix);
    VNew(0, 0, 0, info->drag_transform.displacement);

    drag_offset.x = info->features[origin_feature].view->screen.x -e->xbutton.x;
    drag_offset.y = info->features[origin_feature].view->screen.y -e->xbutton.y;

    /* Drag start is the position before the drag. */
    info->drag_start = info->features[origin_feature].world;

    start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
    init_obj_transform = info->obj->o_transform;

    Edit_Undo_Register_State(edit_drag_op, origin_feature, 0);

    info->drag_type = ORIGIN_DRAG;

    VNew(0, 0, 0, dummy);
    Edit_Set_Drag_Label(ORIGIN_DRAG, dummy, 0);
}


/*  void
**  Continue_Origin_Drag(XEvent *e, EditInfoPtr info)
**  Continues the drag.  Mostly just works out where the object is now and
**  redraws it.
*/
void
Edit_Continue_Origin_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  disp_moved;
    Vector  new_orig;
    XPoint  screen_pt;

    screen_pt.x = e->xmotion.x + drag_offset.x;
    screen_pt.y = e->xmotion.y + drag_offset.y;

    new_orig = Edit_Obtain_New_Point(info, origin_feature, screen_pt,
                                     &view_feature, &constr_pt_1, &constr_pt_2);

    /* Check to see if it's moved. */
    if ( VEqual(new_orig, info->features[origin_feature].world, disp_moved) )
        return;

    VSub(new_orig, info->features[origin_feature].world, disp_moved);

    /* Find out how far it's moved. */
    info->features[origin_feature].world = new_orig;

    VAdd(info->drag_transform.displacement, disp_moved,
         info->drag_transform.displacement);

    Edit_Set_Drag_Label(ORIGIN_DRAG, info->drag_transform.displacement, 0);

    Edit_Move_Object(info);
}



/*  void
**  Finish_Origin_Drag(XEvent *e, EditInfoPtr info)
**  Redraws all the constraints and updates the constraint dependencies.
*/
void
Edit_Finish_Origin_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  dummy;

    /* Rework the available options constraints. */
    /* The active ones were updated as the thing moved. */
    Edit_Update_Constraints(info);

    /* Update the current transform. */
    info->obj->o_transform.matrix = MMMul(&(info->drag_transform.matrix),
                                          &(init_obj_transform.matrix));
    VAdd(init_obj_transform.displacement,
         info->drag_transform.displacement,
         info->obj->o_transform.displacement);

    Edit_Set_Drag_Label(NO_DRAG, dummy, 0);
}


/*  void
**  Edit_Move_Object(EditInfoPtr info)
**  Moves the object from its old position to a new one based on the
**  drag_displacement.
*/
static void
Edit_Move_Object(EditInfoPtr info)
{
    info->obj->o_dynamic_func(info, origin_feature, &start_center,
                              &init_obj_transform);
}



/*
**  Edit_Force_Origin_Satisfaction(EditInfoPtr info)
**  Checks to see whether the origin constraint is satisfied, and if not
**  satisfies it by moving the object.
*/
Boolean
Edit_Force_Origin_Satisfaction(EditInfoPtr info, int n, Boolean prefer)
{
    int preferred =
        info->obj->o_features[origin_feature].flags & FeaturePref1 ? 1 : 2;
    int new_pref;

    new_pref = Point_Satisfies_Constraint(
                            &(info->features[origin_feature].world),
                            &(info->features[origin_feature].resultant), NULL);
    if ( ! new_pref )
    {
        NewIdentityMatrix(info->drag_transform.matrix);
        info->drag_start = info->features[origin_feature].world;
        info->drag_transform.displacement =
            Find_Required_Motion(&(info->features[origin_feature].world),
                        &(info->features[origin_feature].resultant), prefer,
                        &preferred);
        if ( ! prefer )
        {
            if ( preferred == 1 )
            {
                info->obj->o_features[origin_feature].flags &=
                    ( FeatureAll ^ FeaturePref2 );
                info->obj->o_features[origin_feature].flags |= FeaturePref1;
            }
            else
            {
                info->obj->o_features[origin_feature].flags &=
                    ( FeatureAll ^ FeaturePref1 );
                info->obj->o_features[origin_feature].flags |= FeaturePref2;
            }
        }

        VAdd(info->features[origin_feature].world,
             info->drag_transform.displacement,
             info->features[origin_feature].world);

        start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
        init_obj_transform = info->obj->o_transform;
        Edit_Move_Object(info);

        Edit_Finish_Origin_Drag(NULL, 0, info);

        return TRUE;
    }
    else if ( ! prefer )
    {
        if ( new_pref == 1 )
        {
            info->obj->o_features[origin_feature].flags &=
                ( FeatureAll ^ FeaturePref2 );
            info->obj->o_features[origin_feature].flags |= FeaturePref1;
        }
        else
        {
            info->obj->o_features[origin_feature].flags &=
                ( FeatureAll ^ FeaturePref1 );
            info->obj->o_features[origin_feature].flags |= FeaturePref2;
        }
    }

    return FALSE;
}

