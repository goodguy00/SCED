/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/control_pt.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: control_pt.c,v $
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

/*
**  control_pt.c: COde for manipulating control points.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <update.h>

static Transformation   init_obj_transform;
static ConstraintData   view_feature;
static XPoint           constr_pt_1, constr_pt_2;
static XPoint           drag_offset;
static Vector           start_loc;

void
Edit_Start_Control_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    if ( ! Interactive_Resultant(info->features[feature_index].resultant) )
        return;

    Edit_Extract_Resultant_Data(info, feature_index, &view_feature,
                                &constr_pt_1, &constr_pt_2);

    /* This won't change. */
    NewIdentityMatrix(info->drag_transform.matrix);
    VNew(0, 0, 0, info->drag_transform.displacement);
    init_obj_transform = info->obj->o_transform;

    drag_offset.x = info->features[feature_index].view->screen.x - e->xbutton.x;
    drag_offset.y = info->features[feature_index].view->screen.y - e->xbutton.y;

    start_loc = info->features[feature_index].world;

    Edit_Undo_Register_State(edit_drag_op, feature_index, 0);

    info->drag_type = CONTROL_DRAG;

    Edit_Set_Drag_Label(CONTROL_DRAG, info->drag_transform.displacement, 0);
}

void
Edit_Continue_Control_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    XPoint  screen_pt;
    Vector  disp;

    screen_pt.x = e->xmotion.x + drag_offset.x;
    screen_pt.y = e->xmotion.y + drag_offset.y;

    info->features[feature_index].world =
        Edit_Obtain_New_Point(info, feature_index, screen_pt,
                              &view_feature, &constr_pt_1, &constr_pt_2);

    Control_Dynamic_Vertex(info->obj, info->features[feature_index].world,
                           feature_index - pt0_feature,
                           info->obj->o_transform.displacement);

    VSub(info->features[feature_index].world,
         info->obj->o_transform.displacement,
         info->obj->o_features[feature_index].location);

    info->obj->o_dynamic_func(info, pt0_feature, NULL, &init_obj_transform);

    VSub(info->features[feature_index].world, start_loc, disp);
    Edit_Set_Drag_Label(CONTROL_DRAG, disp, 0);
}


void
Edit_Finish_Control_Drag(XEvent *e, int feature, EditInfoPtr info)
{
    Edit_Update_Constraints(info);
    Edit_Set_Drag_Label(NO_DRAG, info->obj->o_features[feature].location, 0);
}


void
Control_Dynamic_Vertex(ObjectInstancePtr obj, Vector new_world, int index,
                       Vector origin)
{
    /* Transform the new world vertex back into object space.
    ** It's that simple.
    */
    VSub(new_world, origin, obj->o_features[index + pt0_feature].location);
    MVMul(obj->o_inverse, obj->o_features[index + pt0_feature].location,
          control_part(obj)->control_verts[index]);
}

Boolean
Edit_Dynamic_Control(EditInfoPtr info, Vector new_center, int feature)
{
    if ( Edit_Update_Active_Object_Cons(
            info->obj->o_features[feature].constraints,
            info->obj->o_features[feature].num_constraints,
            info, &new_center) )
        Constraint_Solve_System(info->obj,
            info->obj->o_features[feature].constraints,
            info->obj->o_features[feature].num_constraints,
            &(info->features[feature].resultant));

    if ( ! Point_Satisfies_Constraint(&(info->features[feature].world),
                          &(info->features[feature].resultant), NULL) )
    {
        int     preference =
                    info->obj->o_features[feature].flags & FeaturePref1 ? 1 : 2;
        Vector  disp;

        disp = Find_Required_Motion(&(info->features[feature].world),
                    &(info->features[feature].resultant), TRUE, &preference);
        VAdd(info->features[feature].world,disp,info->features[feature].world);

        Control_Dynamic_Vertex(info->obj, info->features[feature].world,
                               feature - pt0_feature, new_center);

        return TRUE;
    }

    return FALSE;
}

Boolean
Edit_Dynamic_Control_Obj(EditInfoPtr info, Vector new_center, int start)
{
    Boolean result = FALSE;
    int     i;

    for ( i = start ; i < info->obj->o_num_features ; i++ )
        result = Edit_Dynamic_Control(info, new_center, i) || result;

    return result;
}



Boolean
Edit_Force_Control_Satisfaction(EditInfoPtr info, int feature, Boolean prefer)
{
    int new_pref;

    new_pref = Point_Satisfies_Constraint( &(info->features[feature].world),
                            &(info->features[feature].resultant), NULL);

    if ( ! new_pref )
    {
        int     preferred =
                    info->obj->o_features[feature].flags & FeaturePref1 ? 1 : 2;
        Vector  disp;

        disp =
            Find_Required_Motion(&(info->features[feature].world),
                         &(info->features[feature].resultant), prefer,
                         &preferred);

        if ( ! prefer )
        {
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
        }

        VAdd(info->features[feature].world, disp,info->features[feature].world);

        Control_Dynamic_Vertex(info->obj, info->features[feature].world,
                   feature - pt0_feature,
                   info->obj->o_world_verts[info->obj->o_num_vertices - 1]);

        init_obj_transform = info->obj->o_transform;
        info->obj->o_dynamic_func(info, pt0_feature, NULL, &init_obj_transform);

        return TRUE;
    }
    else if ( ! prefer )
    {
        if ( new_pref == 1 )
        {
            info->obj->o_features[feature].flags &= (FeatureAll ^ FeaturePref2);
            info->obj->o_features[feature].flags |= FeaturePref1;
        }
        else
        {
            info->obj->o_features[feature].flags &= (FeatureAll ^ FeaturePref1);
            info->obj->o_features[feature].flags |= FeaturePref2;
        }
    }

    return FALSE;
}


