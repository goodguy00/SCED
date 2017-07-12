/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/maintain.c,v 1.2 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: maintain.c,v $
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
**  maintain.c: constraint maintenance functions.
*/

#include <math.h>
#include <sced.h>
#include <bezier.h>
#include <constraint.h>
#include <edit.h>
#include <instance_list.h>
#include <quaternion.h>
#include <triangle.h>
#include <update.h>
#include <X11/Xaw/Toggle.h>

static Boolean  Maintain_Align_Major(ObjectInstancePtr, Vector);
static Boolean  Maintain_Align_Minor(ObjectInstancePtr, Vector,Boolean,Vector);

Boolean do_maintenance = TRUE;

InstanceList    topological_list = NULL;


static Boolean
Maintain_Dynamic_Origin_Part(EditInfoPtr info, VectorPtr start_center)
{
    Vector          new_center;
    Transformation  align_trans;
    Matrix          transp;
    Vector          temp_v;
    int             i;

    /* Check alignment. */
    VAdd(info->drag_transform.displacement, *start_center, new_center);
    if ( Edit_Dynamic_Align(info, &align_trans, new_center) )
    {
        /* Rotate the axes. */
        MTrans(align_trans.matrix, transp);
        info->axes = MMMul(&(info->axes), &transp);
        info->obj->o_features[major_feature].location = info->axes.x;
        info->obj->o_features[minor_feature].location = info->axes.y;
        MTrans(info->axes, info->axes_inverse);

        /* Rotate the feature points. */
        for ( i = scale_feature ; i < info->obj->o_num_features ; i++ )
        {
            if ( ! info->obj->o_features[i].base )  continue;
            MVMul(align_trans.matrix, info->obj->o_features[i].location,temp_v);
            info->obj->o_features[i].location = temp_v;
        }

        /* Bring the drag transform up to date. */
        info->drag_transform.matrix = MMMul(&(align_trans.matrix),
                                            &(info->drag_transform.matrix));
        VAdd(info->drag_transform.displacement,
             align_trans.displacement,
             info->drag_transform.displacement);

        return TRUE;
    }

    return FALSE;
}

static Boolean
Maintain_Dynamic_Align_Part(EditInfoPtr info, VectorPtr start_center,
                            Transformation *init_transform,
                            VectorPtr new_center_return)
{
    Vector  new_center;
    int     i;

    /* Calculate the new center again. */
    VAdd(info->drag_transform.displacement, *start_center, new_center);

    /* Calculate new constraint pts. */
    for ( i = scale_feature ; i < info->obj->o_num_features ; i++ )
    {
        if ( ! info->obj->o_features[i].base )  continue;
        VAdd(info->obj->o_features[i].location, new_center,
             info->features[i].world);
    }

    VSub(info->features[origin_feature].world, new_center,
         info->obj->o_features[origin_feature].location);

    /* Apply the transform to the object. */
    Apply_Transform(*init_transform, info->drag_transform,
                    info->obj->o_transform);

    if ( new_center_return )
        *new_center_return = new_center;

    /* Dynamicly scale the object. */
    if ( info->obj->o_num_features > scale_feature )
        return Edit_Dynamic_Scale(info, new_center);

    return FALSE;
}


static void
Maintain_Dynamic_Scale_Part(EditInfoPtr info, VectorPtr start_center,
                            Transformation *init_transform,
                            VectorPtr new_center_return)
{
    Vector  new_center;
    int     i;

    Apply_Transform(*init_transform, info->drag_transform,
                    info->obj->o_transform);
    Transform_Vector(info->obj->o_transform,
        info->obj->o_wireframe->vertices[info->obj->o_num_vertices - 1],
        new_center);
    for ( i = radius_feature ; i < info->obj->o_num_features ; i++ )
    {
        if ( ! info->obj->o_features[i].base )  continue;
        VAdd(info->obj->o_features[i].location, new_center,
             info->features[i].world);
    }
    VSub(info->features[scale_feature].world, new_center,
         info->obj->o_features[scale_feature].location);
    VSub(info->features[origin_feature].world, new_center,
         info->obj->o_features[origin_feature].location);

    if ( new_center_return )
        *new_center_return = new_center;
}

static void
Maintain_Dynamic_Do_Maintenance(EditInfoPtr info)
{
    /* Do maintenance and get the drawing done. */
    if ( ! do_maintenance ||
         ! Edit_Maintain_All_Constraints(info->obj, info->window, FALSE) )
    {
        /* Undraw the object and constraints. */
        Edit_Draw(info->window, ViewNone, info, FALSE);
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
        Draw_All_Constraints(info->window, ViewNone, info, FALSE);

        /* Draw on the new features. */
        Draw_Edit_Extras(info->window, CalcView, info, FALSE);
        Draw_All_Constraints(info->window, CalcView, info, FALSE);
        Edit_Draw(info->window, CalcView, info, TRUE);
    }
}

static void
Maintain_Dynamic_Common_Updates(EditInfoPtr info, Boolean do_normals)
{
    /* Transform vertices and normals. */
    Edit_Transform_Vertices(&(info->obj->o_transform), info);
    if ( do_normals )
        Edit_Transform_Normals(&(info->obj->o_transform), info);

    /* Update active constraints again, for drawing. */
    if ( Edit_Update_Active_Object_Cons(
                info->obj->o_features[origin_feature].constraints,
                info->obj->o_features[origin_feature].num_constraints, info,
                info->obj->o_world_verts + ( info->obj->o_num_vertices - 1)) )
        Constraint_Solve_System(info->obj,
                    info->obj->o_features[origin_feature].constraints,
                    info->obj->o_features[origin_feature].num_constraints,
                    &(info->features[origin_feature].resultant));

    Maintain_Dynamic_Do_Maintenance(info);
}

/* Does most of the work for dynamically updating constraints on an
** object currently being dragged.
**
** Assumes on entry that info->drag_transform has the current drag transform
** and that the origin, scaling and axes for the body are accurate.
** start_center and init_transform hold parameters from the start of the
** drag sequence.
*/
void
Maintain_Generic_Dynamic(EditInfoPtr info, int entry_point,
                    VectorPtr start_center, Transformation *init_transform)
{
    Boolean transform_normals = FALSE;

    switch ( entry_point )
    {
      case origin_feature:
        transform_normals = Maintain_Dynamic_Origin_Part(info, start_center);
        /* Fall through. */

      case major_feature:
      case minor_feature:
        if ( entry_point == major_feature || entry_point == minor_feature )
            transform_normals = TRUE;

        if ( ! Maintain_Dynamic_Align_Part(info, start_center, init_transform,
                                           NULL) )
            break;

      case scale_feature:
        Maintain_Dynamic_Scale_Part(info, start_center, init_transform, NULL);
        transform_normals = TRUE;
        break;

      default:;
    }

    /* This gets done for everything. */
    Maintain_Dynamic_Common_Updates(info, transform_normals);
}


void
Maintain_Torus_Dynamic(EditInfoPtr info, int entry_point,
                       VectorPtr start_center, Transformation *init_transform)
{
    Vector  new_center;
    Boolean transform_normals = FALSE;

    switch ( entry_point )
    {
      case origin_feature:
        /* Check alignment. */
        transform_normals = Maintain_Dynamic_Origin_Part(info, start_center);
        /* Fall through. */

      case major_feature:
      case minor_feature:
        if ( entry_point == major_feature || entry_point == minor_feature )
            transform_normals = TRUE;

        if ( ! Maintain_Dynamic_Align_Part(info, start_center, init_transform,
                                           &new_center) )
            goto do_radius;

      case scale_feature:
        Maintain_Dynamic_Scale_Part(info, start_center, init_transform,
                                    &new_center);
        transform_normals = TRUE;

do_radius:;
        if ( ! Edit_Dynamic_Set_Radius(info, new_center) )
            break;

      case radius_feature:
        if ( entry_point == radius_feature )
            new_center =
                info->obj->o_world_verts[info->obj->o_num_vertices - 1];

        /* May have resulted in a scaling transform. */
        Apply_Transform(*init_transform, info->drag_transform,
                        info->obj->o_transform);
        transform_normals = TRUE;
        VSub(info->features[radius_feature].world, new_center,
             info->obj->o_features[radius_feature].location);
        VAdd(info->obj->o_features[scale_feature].location, new_center,
             info->features[scale_feature].world);
        break;

        default:;
    }

    /* This gets done for everything. */
    Maintain_Dynamic_Common_Updates(info, transform_normals);
}

void
Maintain_Control_Obj_Dynamic(EditInfoPtr info, int entry_point,
                        VectorPtr start_center, Transformation *init_transform)
{
    Vector  new_center;
    Boolean transform_normal = FALSE;
    int     i;

    switch ( entry_point )
    {
      case origin_feature:
        /* Check alignment. */
        transform_normal = Maintain_Dynamic_Origin_Part(info, start_center);
        /* Fall through. */

      case major_feature:
      case minor_feature:
        if ( entry_point == major_feature || entry_point == minor_feature )
            transform_normal = TRUE;

        if ( ! Maintain_Dynamic_Align_Part(info, start_center, init_transform,
                                           &new_center) )
            goto do_vertices;

      case scale_feature:
        Maintain_Dynamic_Scale_Part(info, start_center, init_transform,
                                    &new_center);
        transform_normal = TRUE;

      case pt0_feature:
        if ( entry_point == pt0_feature )
            new_center = info->obj->o_world_verts[info->obj->o_num_vertices-1];

        transform_normal = TRUE;

do_vertices:;
        if ( entry_point < pt0_feature )    entry_point = pt0_feature;

        if ( ! Edit_Dynamic_Control_Obj(info, new_center, entry_point) )
            break;

        for ( i = pt0_feature ; i < info->obj->o_num_features ; i++ )
            VSub(info->features[i].world, new_center,
                 info->obj->o_features[i].location);

	break;

      default:;
    }

    /* This gets done for everything. */
    Maintain_Dynamic_Common_Updates(info, transform_normal);
}


void
Maintain_Dir_Dynamic(EditInfoPtr info, int entry_point, VectorPtr start_center,
                     Transformation *init_transform)
{
    Boolean transform_normals = FALSE;

    switch ( entry_point )
    {
      case origin_feature:
        /* Check alignment. */
        transform_normals = Maintain_Dynamic_Origin_Part(info, start_center);
        /* Fall through. */

      case major_feature:
      case minor_feature:
        if ( entry_point == major_feature || entry_point == minor_feature )
            transform_normals = TRUE;
        Maintain_Dynamic_Align_Part(info, start_center, init_transform, NULL);
        break;

      default:;
    }

    /* This gets done for everything. */
    Maintain_Dynamic_Common_Updates(info, transform_normals);
}



void
Maintain_Point_Dynamic(EditInfoPtr info, int entry_point,
                       VectorPtr start_center, Transformation *init_transform)
{
    Vector          new_center;

    VAdd(info->drag_transform.displacement, *start_center, new_center);

    VSub(info->features[origin_feature].world, new_center,
         info->obj->o_features[origin_feature].location);

    /* Apply the transform to the object. */
    Apply_Transform(*init_transform, info->drag_transform,
                    info->obj->o_transform);

    /* This gets done for everything. */
    Maintain_Dynamic_Common_Updates(info, FALSE);
}

void
Constrain_World_Objects(WindowInfoPtr window)
{
    InstanceList    world_topo_list = NULL;
    InstanceList    temp;

    /* Build a topologically sorted list of all objects. */
    for ( temp = window->all_instances ; temp ; temp = temp->next )
        DFS(temp->the_instance, NULL, 0, TRUE, &world_topo_list,
            temp == window->all_instances, FALSE);

    for ( temp = topological_list ; temp ; temp = temp->next )
        temp->the_instance->o_static_func(temp->the_instance);
}


Boolean
Edit_Maintain_All_Constraints(ObjectInstancePtr obj, WindowInfoPtr window,
                              Boolean do_obj)
{
    InstanceList    temp;
    Boolean         result = FALSE;

    if ( ! topological_list )
        DFS(obj, NULL, 0, TRUE, &topological_list, TRUE, TRUE);

    for ( temp = do_obj ? topological_list : topological_list->next ;
          temp ; temp = temp->next )
        result = temp->the_instance->o_static_func(temp->the_instance) ||
                 result;

    if ( result )
        View_Update(window, topological_list, CalcView );

    return result;
}


void
Edit_Maintain_Free_List()
{
    InstanceList    elmt;

    for ( elmt = topological_list ; elmt ; elmt = elmt->next )
        elmt->the_instance->o_flags &= ( ObjAll ^ ObjDepends );
    Free_Selection_List(topological_list);
    topological_list = NULL;
}


static Boolean
Maintain_Static_Origin_Part(ObjectInstancePtr obj, VectorPtr origin)
{
    Resultant       resultant;
    Transformation  transform;
    Boolean         new_cons = FALSE;
    int             preference;
    int             i;

    /* Update all the origin constraints. */
    VAdd(obj->o_features[origin_feature].location,
         obj->o_transform.displacement, *origin);

    for ( i = 0 ; i < obj->o_features[origin_feature].num_constraints ; i++ )
        new_cons = Edit_Update_Constraint_Specs(
                        obj->o_features[origin_feature].constraints + i,
                        &obj->o_transform.displacement,
                        obj->o_features, NULL) || new_cons;

    /* Resolve for them. */
    if ( new_cons )
    {
        Constraint_Solve_System(obj,
                                obj->o_features[origin_feature].constraints,
                                obj->o_features[origin_feature].num_constraints,
                                &resultant);

        if ( ! Point_Satisfies_Constraint(origin, &resultant, NULL) )
        {
            NewIdentityMatrix(transform.matrix);
            preference =
                obj->o_features[origin_feature].flags & FeaturePref1 ? 1 : 2;
            transform.displacement =
                Find_Required_Motion(origin, &resultant, TRUE,&preference);
            Transform_Instance(obj, &transform, FALSE);
            VAdd(obj->o_features[origin_feature].location,
                 obj->o_transform.displacement, *origin);
            return TRUE;
        }
    }

    return FALSE;
}


static Boolean
Maintain_Static_Align_Part(ObjectInstancePtr obj)
{
    Resultant   resultant;
    Boolean     new_cons = FALSE;
    Boolean     did_major = FALSE;
    Boolean     changed = FALSE;
    Vector      obj_axes_z;
    int         i;

    /* Redo the alignment. Do it before scaling because scaling depends
    ** on alignment.
    */
    for ( i = 0 ; i < obj->o_features[major_feature].num_constraints ; i++ )
        new_cons = Edit_Update_Constraint_Specs(
                    obj->o_features[major_feature].constraints + i,
                    &obj->o_transform.displacement,
                    obj->o_features, NULL) || new_cons;
    for ( i = 0 ; i < obj->o_features[minor_feature].num_constraints ; i++ )
        new_cons = Edit_Update_Constraint_Specs(
                    obj->o_features[minor_feature].constraints + i,
                    &obj->o_transform.displacement,
                    obj->o_features, NULL) || new_cons;
    if ( new_cons )
    {

        Constraint_Solve_System(obj, obj->o_features[major_feature].constraints,
                                obj->o_features[major_feature].num_constraints,
                                &resultant);
        if ( resultant.feature_1.c_type == line_feature )
        {
            changed = Maintain_Align_Major(obj, resultant.feature_1.c_vector);

            /* Update defaults. */
            Edit_Update_Constraint_Specs(
                                    obj->o_features[major_feature].constraints,
                                    &obj->o_transform.displacement,
                                    obj->o_features, NULL);
            Edit_Update_Constraint_Specs(
                                    obj->o_features[minor_feature].constraints,
                                    &obj->o_transform.displacement,
                                    obj->o_features, NULL);
            did_major = TRUE;
        }

        Constraint_Solve_System(obj, obj->o_features[minor_feature].constraints,
                                obj->o_features[minor_feature].num_constraints,
                                &resultant);
        if ( resultant.feature_1.c_type == line_feature )
        {
            VCross(obj->o_features[major_feature].location,
                   obj->o_features[minor_feature].location, obj_axes_z);

            changed = Maintain_Align_Minor(obj, resultant.feature_1.c_vector,
                                           did_major, obj_axes_z) || changed;

            /* Update defaults. */
            Edit_Update_Constraint_Specs(
                                    obj->o_features[major_feature].constraints,
                                    &obj->o_transform.displacement,
                                    obj->o_features, NULL);
            Edit_Update_Constraint_Specs(
                                    obj->o_features[minor_feature].constraints,
                                    &obj->o_transform.displacement,
                                    obj->o_features, NULL);
        }
    }

    return changed;
}


static Boolean
Maintain_Static_Scale_Part(ObjectInstancePtr obj, VectorPtr origin,
                           Boolean always)
{
    Resultant       resultant;
    Transformation  transform;
    Vector          reference;
    int             preference;
    int             i;
    Vector          obj_axes_z;
    Vector          orig_ref;
    Vector          disp;
    Matrix          obj_axes;
    Boolean         new_cons = FALSE;

    VCross(obj->o_features[major_feature].location,
           obj->o_features[minor_feature].location, obj_axes_z);

    VAdd(obj->o_features[scale_feature].location,
         obj->o_transform.displacement, reference);

    for ( i = 0 ; i < obj->o_features[scale_feature].num_constraints ; i++ )
        new_cons = Edit_Update_Constraint_Specs(
                    obj->o_features[scale_feature].constraints + i,
                    &obj->o_transform.displacement,
                    obj->o_features, &(obj_axes_z)) || new_cons;

    if ( new_cons || always )
    {
        VSub(reference, *origin, disp);
        obj_axes.x = obj->o_features[major_feature].location;
        obj_axes.y = obj->o_features[minor_feature].location;
        obj_axes.z = obj_axes_z;
        MVMul(obj_axes, disp, orig_ref);

        /* Resolve for the scaling resultant. */
        Constraint_Solve_System(obj, obj->o_features[scale_feature].constraints,
                                obj->o_features[scale_feature].num_constraints,
                                &resultant);

        if ( ! Point_Satisfies_Constraint(&reference, &resultant, NULL) )
        {
            Matrix  axes_inverse = MInvert(&obj_axes);

            preference =
                obj->o_features[scale_feature].flags & FeaturePref1 ? 1 : 2;
            disp =
                Find_Required_Motion(&reference, &resultant, TRUE, &preference);
            VAdd(disp, reference, reference);

            Scale_Calculate_Transform(&transform, reference, orig_ref,
                                    obj->o_features[origin_feature].location,
                                    *origin, &(obj_axes), &axes_inverse, FALSE);

            Transform_Instance(obj, &transform, FALSE);

            VSub(*origin, obj->o_transform.displacement,
                 obj->o_features[origin_feature].location);
            VSub(reference, obj->o_transform.displacement,
                 obj->o_features[scale_feature].location);

            for ( i = radius_feature ; i < obj->o_num_features ; i++ )
                Transform_Vector(transform, obj->o_features[i].location,
                                 obj->o_features[i].location);
            for ( i = 0 ;
                  i < obj->o_features[scale_feature].num_constraints ; i++ )
                Edit_Update_Constraint_Specs(
                                obj->o_features[scale_feature].constraints + i,
                                &obj->o_transform.displacement,
                                obj->o_features, &(obj_axes_z));
            return TRUE;
        }
    }

    return FALSE;
}

Boolean
Maintain_Generic_Static(ObjectInstancePtr obj)
{
    Vector  origin;
    Boolean changed = FALSE;

    changed = Maintain_Static_Origin_Part(obj, &origin);
    changed = Maintain_Static_Align_Part(obj) || changed;
    changed = Maintain_Static_Scale_Part(obj, &origin, changed) || changed;

    return changed;
}


Boolean
Maintain_Point_Static(ObjectInstancePtr obj)
{
    Vector  origin;
    return Maintain_Static_Origin_Part(obj, &origin);
}


Boolean
Maintain_Dir_Static(ObjectInstancePtr obj)
{
    Vector          origin;
    Boolean         changed = FALSE;

    changed = Maintain_Static_Origin_Part(obj, &origin);
    changed = Maintain_Static_Align_Part(obj) || changed;

    return changed;
}


Boolean
Maintain_Torus_Static(ObjectInstancePtr obj)
{
    Resultant       resultant;
    Transformation  transform;
    Vector          origin;
    Vector          radius_world;
    Boolean         changed = FALSE;
    Boolean         new_cons = FALSE;
    int             preference;
    int             i;
    Vector          obj_axes_z;

    changed = Maintain_Static_Origin_Part(obj, &origin);
    changed = Maintain_Static_Align_Part(obj) || changed;
    changed = Maintain_Static_Scale_Part(obj, &origin, changed) || changed;

    VCross(obj->o_features[major_feature].location,
           obj->o_features[minor_feature].location, obj_axes_z);

    for ( i = 0 ; i < obj->o_features[radius_feature].num_constraints ; i++ )
        new_cons = Edit_Update_Constraint_Specs(
                    obj->o_features[radius_feature].constraints + i,
                    &obj->o_transform.displacement,
                    obj->o_features, &(obj_axes_z)) || new_cons;

    if ( new_cons || changed )
    {
        /* Resolve for the radius resultant. */
        Constraint_Solve_System(obj,
                obj->o_features[radius_feature].constraints,
                obj->o_features[radius_feature].num_constraints, &resultant);

        VAdd(obj->o_features[radius_feature].location,
             obj->o_transform.displacement, radius_world);

        if ( ! Point_Satisfies_Constraint(&radius_world, &resultant, NULL) )
        {
            Vector  disp;
            Vector  start_world;
            Matrix  obj_axes;

            start_world = radius_world;
            preference =
                obj->o_features[radius_feature].flags & FeaturePref1 ? 1:2;
            disp = Find_Required_Motion(&radius_world, &resultant, TRUE,
                                        &preference);
            VAdd(disp, radius_world, radius_world);

            obj_axes.x = obj->o_features[major_feature].location;
            obj_axes.y = obj->o_features[minor_feature].location;
            obj_axes.z = obj_axes_z;

            /* To find out whether scaling needs to be maintained. */
            Constraint_Solve_System(obj,
                        obj->o_features[scale_feature].constraints,
                        obj->o_features[scale_feature].num_constraints,
                        &resultant);

            VNew(0, 0, 0, transform.displacement);
            Radius_Modify_Radius(start_world, torus_part(obj)->major_radius,
                    obj->o_features[scale_feature].location, radius_world,
                    &(obj->o_transform),
                    &(obj_axes), &(torus_part(obj)->major_radius),
                    &(obj->o_features[scale_feature].location),
                    &(transform.matrix),
                    resultant.feature_1.c_type != null_feature);

            Transform_Instance(obj, &transform, FALSE);

            VSub(radius_world, obj->o_transform.displacement,
                 obj->o_features[radius_feature].location);
            for ( i = 0 ;
                  i < obj->o_features[radius_feature].num_constraints ; i++ )
                Edit_Update_Constraint_Specs(
                            obj->o_features[radius_feature].constraints + i,
                            &obj->o_transform.displacement,
                            obj->o_features, &(obj_axes_z));
            changed = TRUE;
        }
    }

    return changed;
}


static Boolean
Maintain_Static_Control_Part(ObjectInstancePtr obj, int feature, Boolean always)
{
    Resultant   resultant;
    Vector      disp;
    Boolean     new_cons = FALSE;
    int         preference;
    Vector      obj_axes_z;
    Vector      control_world;
    int         i;


    VCross(obj->o_features[major_feature].location,
           obj->o_features[minor_feature].location, obj_axes_z);

    VAdd(obj->o_features[feature].location,
         obj->o_transform.displacement, control_world);

    for ( i = 0 ; i < obj->o_features[feature].num_constraints ; i++ )
        new_cons = Edit_Update_Constraint_Specs(
                        obj->o_features[feature].constraints + i,
                        &obj->o_transform.displacement,
                        obj->o_features, &(obj_axes_z)) || new_cons;

    /* Resolve for them. */
    if ( new_cons || always )
    {
        Constraint_Solve_System(obj,
                                obj->o_features[feature].constraints,
                                obj->o_features[feature].num_constraints,
                                &resultant);

        if ( ! Point_Satisfies_Constraint(&control_world, &resultant, NULL) )
        {
            preference = obj->o_features[feature].flags & FeaturePref1 ? 1 : 2;
            disp = Find_Required_Motion(&control_world, &resultant, TRUE,
                                        &preference);

            VAdd(control_world, disp, control_world);
            VSub(control_world, obj->o_transform.displacement,
                 obj->o_features[feature].location);
            MVMul(obj->o_inverse, obj->o_features[feature].location,
                  control_part(obj)->control_verts[feature - pt0_feature]);
            return TRUE;
        }
    }

    return FALSE;
}


Boolean
Maintain_Control_Obj_Static(ObjectInstancePtr obj)
{
    Vector  origin;
    Boolean changed = FALSE;
    Boolean vert_changed = FALSE;
    Vector  normal;
    Matrix  transp;
    double  temp_d;
    int     i;

    changed = Maintain_Static_Origin_Part(obj, &origin);
    changed = Maintain_Static_Align_Part(obj) || changed;
    changed = Maintain_Static_Scale_Part(obj, &origin, changed) || changed;

    for ( i = pt0_feature ; i < obj->o_num_features ; i++ )
        vert_changed =
            Maintain_Static_Control_Part(obj, i, changed) || vert_changed;

    if ( vert_changed )
    {
        if ( Obj_Is_Triangle(obj) )
        {
            control_part(obj)->calc_verts(obj, obj->o_world_verts, 3, 4);
            Transform_Vertices(obj->o_transform, obj->o_world_verts,
                               obj->o_num_vertices);
            control_part(obj)->calc_face_norms(obj, &normal);
            MTrans(obj->o_inverse, transp);
            MVMul(transp, normal, obj->o_normals[0]);
            if ( ! VZero(obj->o_normals[0]) )
                VUnit(obj->o_normals[0], temp_d, obj->o_normals[0]);
        }
        else if ( Obj_Is_Bezier(obj) )
        {
            control_part(obj)->calc_verts(obj, obj->o_world_verts,
					  obj->o_num_real, obj->o_num_vertices);
            Transform_Vertices(obj->o_transform, obj->o_world_verts,
                               obj->o_num_vertices);
            control_part(obj)->calc_face_norms(obj, obj->o_normals);
        }
    }

    return ( changed || vert_changed );
}



static void
Maintain_Align(ObjectInstancePtr obj, Quaternion rot_quat)
{
    Transformation  rot_trans;
    Vector  org_to_cent;
    Vector  origin;
    Vector  new_center;
    Vector  temp_v;
    int     i;

    /* Work out the matrix. */
    rot_trans.matrix = Quaternion_To_Matrix(rot_quat);

    VAdd(obj->o_features[origin_feature].location,
         obj->o_transform.displacement, origin);

    /* Rotate the center to find the displacement. */
    VScalarMul(obj->o_features[origin_feature].location, -1, org_to_cent);
    MVMul(rot_trans.matrix, org_to_cent, new_center);
    VAdd(origin, new_center, new_center);
    VSub(new_center, obj->o_transform.displacement, rot_trans.displacement);

    Transform_Instance(obj, &rot_trans, FALSE);

    VSub(origin, obj->o_transform.displacement,
         obj->o_features[origin_feature].location);
    for ( i = scale_feature ; i < obj->o_num_features ; i++ )
    {
        if ( ! obj->o_features[i].base )  continue;

        MVMul(rot_trans.matrix, obj->o_features[i].location, temp_v);
        obj->o_features[i].location = temp_v;
    }

    /* Rotate the axes. */
    MVMul(rot_trans.matrix, obj->o_features[major_feature].location, temp_v);
    obj->o_features[major_feature].location = temp_v;
    MVMul(rot_trans.matrix, obj->o_features[minor_feature].location, temp_v);
    obj->o_features[minor_feature].location = temp_v;
}


static Boolean
Maintain_Align_Major(ObjectInstancePtr obj, Vector new_dir)
{
    Vector  axis;
    double  temp_d;

    /* Build a quaternion for the rotation. */
    VCross(obj->o_features[major_feature].location, new_dir, axis);

    if ( VZero(axis) ) return FALSE;    /* Already aligned. */

    VUnit(axis, temp_d, axis);

    Maintain_Align(obj,
            Build_Rotate_Quaternion(axis,
                    VDot(new_dir, obj->o_features[major_feature].location)));

    return TRUE;
}


static Boolean
Maintain_Align_Minor(ObjectInstancePtr obj, Vector new_dir,
                     Boolean preserve_major, Vector other_axis)
{
    Vector  new_other, unit_new, axis;
    double  cos_angle;
    double  temp_d;

    if ( preserve_major )
    {
        VCross(obj->o_features[major_feature].location, new_dir, new_other);

        if ( VZero(new_other) ) return FALSE;   /* Can't do it. */

        VUnit(new_other, temp_d, unit_new);

        VCross(other_axis, unit_new, axis);
        if ( VZero(axis) ) return FALSE;    /* Already aligned. */
        VUnit(axis, temp_d, axis);
        cos_angle = VDot(unit_new, other_axis);
    }
    else
    {
        VCross(obj->o_features[minor_feature].location, new_dir, axis);
        cos_angle = VDot(obj->o_features[minor_feature].location, new_dir);
    }

    Maintain_Align(obj, Build_Rotate_Quaternion(axis, cos_angle));

    return TRUE;
}


void
Maintain_Toggle_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();

    XtVaGetValues(w, XtNstate, &do_maintenance, NULL);

    if ( do_maintenance &&
         ( ( main_window.current_state & EditState ) ||
           ( csg_window.current_state & EditState ) ||
           ( agg_window.current_state & EditState ) ) )
        Edit_Maintain_All_Constraints(info->obj, info->window, FALSE);
}


