/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/constr_update.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: constr_update.c,v $
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
**  constr_update.c : functions to update constraints.
**
*/

#include <math.h>
#include <sced.h>
#include <bezier.h>
#include <edit.h>
#include <update.h>
#include <add_constraint.h>
#include <torus.h>

/* Spec update functions. */
static void Update_Absolute_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                                 VectorPtr, VectorPtr);
static void Update_Offset_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                               VectorPtr, VectorPtr);
static void Update_Reference_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                                  VectorPtr, VectorPtr);
static void Update_Vertex_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                               VectorPtr, VectorPtr);
static void Update_Parameter_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                                  VectorPtr, VectorPtr);
static void Update_Feature_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                                VectorPtr, VectorPtr);
static void Update_Other_Spec(ConstraintSpecPtr, VectorPtr, FeaturePtr,
                              VectorPtr, VectorPtr);

Update_Spec_Func    update_spec_function[] =
                        { Update_Absolute_Spec, Update_Offset_Spec,
                          Update_Reference_Spec, Update_Feature_Spec,
                          Update_Other_Spec, Update_Vertex_Spec,
                          Update_Parameter_Spec };

/* Constraint update functions. */
static Boolean  Update_Plane_Constraint(ConstraintPtr, VectorPtr, VectorPtr); 
static Boolean  Update_Line_Constraint(ConstraintPtr, VectorPtr, VectorPtr); 
static Boolean  Update_Point_Constraint(ConstraintPtr, VectorPtr, VectorPtr); 
static Boolean  Update_Sphere_Constraint(ConstraintPtr, VectorPtr,VectorPtr); 
static Boolean  Update_Circle_Constraint(ConstraintPtr, VectorPtr, VectorPtr); 
static Boolean  Update_Axis_Constraint(ConstraintPtr, VectorPtr, VectorPtr); 
static Boolean  Update_Ratio_Point_Constraint(ConstraintPtr, VectorPtr,
                                              VectorPtr);
static Boolean  Update_Ratio_Plane_Constraint(ConstraintPtr, VectorPtr,
                                              VectorPtr);

Update_Constraint_Func  update_con_function[] =
                        { NULL, Update_Plane_Constraint,
                          Update_Line_Constraint, Update_Point_Constraint,
                          Update_Sphere_Constraint, Update_Circle_Constraint,
                          NULL, NULL, NULL, NULL };


static void
Update_Absolute_Spec(ConstraintSpecPtr spec, VectorPtr center,
                     FeaturePtr features, VectorPtr other_axis,
                     VectorPtr result)
{
    *result = spec->spec_vector;
}

static void
Update_Offset_Spec(ConstraintSpecPtr spec, VectorPtr center,
                    FeaturePtr features, VectorPtr other_axis, VectorPtr result)
{
    VAdd(spec->spec_vector, *center, *result);
}

static void
Update_Reference_Spec(ConstraintSpecPtr spec, VectorPtr center,
                FeaturePtr features, VectorPtr other_axis, VectorPtr result)
{
    Ref_Transform_Vector(spec_object(spec), spec->spec_vector, *result);
}


static void
Update_Vertex_Spec(ConstraintSpecPtr spec, VectorPtr center,
                   FeaturePtr features, VectorPtr other_axis, VectorPtr result)
{
    Ref_Transform_Vector(spec_object(spec),
      control_part(spec_object(spec))->control_verts[(int)spec->spec_vector.x],
      *result);
}


static void
Update_Parameter_Spec(ConstraintSpecPtr spec, VectorPtr center,
                      FeaturePtr features, VectorPtr other_axis,
                      VectorPtr result)
{
    *result = Bezier_Calculate_Point(spec_object(spec), spec->spec_vector.x,
                                     spec->spec_vector.y);
}


static void
Update_Feature_Spec(ConstraintSpecPtr spec, VectorPtr center,
                    FeaturePtr features, VectorPtr other_axis,
                    VectorPtr result)
{
    if ( (int)spec->spec_data == major_feature ||
         (int)spec->spec_data == minor_feature )
        spec->spec_vector = features[(int)spec->spec_data].location;
    else
        VAdd(*center, features[(int)spec->spec_data].location,
             spec->spec_vector);
    *result = spec->spec_vector;
}

static void
Update_Other_Spec(ConstraintSpecPtr spec, VectorPtr center,
                FeaturePtr features, VectorPtr other_axis, VectorPtr result)
{
    spec->spec_vector = *result = *other_axis;
}


static Boolean
Update_Plane_Constraint(ConstraintPtr cons, VectorPtr endpoints,
                        VectorPtr org)
{
    Vector  temp_v1, temp_v2, temp_v3;
    Vector  norm;
    double  temp_d;

    if ( cons->c_spec_flags & RatioPlane )
        return Update_Ratio_Plane_Constraint(cons, endpoints, org);

    if ( cons->c_spec_flags & NormPlane )
        norm = endpoints[1];
    else if ( cons->c_spec_flags & ConstNorm )
        norm = cons->c_vector;
    else if ( ( cons->c_spec_flags & TwoisNorm ) &&
              ( cons->c_spec_flags & ThreeisNorm ) &&
              ( cons->c_spec_flags & HavePt4 ) )
    {
        VSub(endpoints[2], endpoints[1], temp_v1);
        VSub(endpoints[3], endpoints[1], temp_v2);
        VCross(temp_v1, temp_v2, norm);
    }
    else if ( ( cons->c_spec_flags & TwoisNorm ) &&
              ( cons->c_spec_flags & ThreeisNorm ) )
        VSub(endpoints[2], endpoints[1], norm);
    else if ( cons->c_spec_flags & TwoisNorm )
    {
        VSub(endpoints[2], endpoints[0], temp_v1);
        VSub(endpoints[1], endpoints[0], temp_v2);
        VCross(temp_v1, temp_v2, temp_v3);
        VCross(temp_v3, temp_v1, norm);
    }
    else if ( cons->c_spec_flags & ThreeisNorm )
    {
        VSub(endpoints[1], endpoints[0], temp_v1);
        VSub(endpoints[2], endpoints[0], temp_v2);
        VCross(temp_v1, temp_v2, temp_v3);
        VCross(temp_v3, temp_v1, norm);
    }
    else
    {
        VSub(endpoints[1], endpoints[0], temp_v1);
        VSub(endpoints[2], endpoints[0], temp_v2);
        VCross(temp_v1, temp_v2, norm);
    }

    if ( VZero(norm) )
        return FALSE;
    VUnit(norm, temp_d, cons->c_vector);
    cons->c_point = endpoints[0];
    cons->c_value = VDot(cons->c_vector, cons->c_point);

    return TRUE;
}

static Boolean
Update_Line_Constraint(ConstraintPtr cons, VectorPtr endpoints, VectorPtr org)
{
    Vector  dir;
    double  temp_d;

    if ( cons->c_spec_flags & Axis )
        return Update_Axis_Constraint(cons, endpoints, org);

    if ( cons->c_spec_flags & HavePt3 )
        VSub(endpoints[2], endpoints[1], dir);
    else
        VSub(endpoints[1], endpoints[0], dir);
    if ( VZero(dir) )
        return FALSE;
    VUnit(dir, temp_d, cons->c_vector);
    cons->c_point = endpoints[0];

    return TRUE;
}

static Boolean
Update_Point_Constraint(ConstraintPtr cons, VectorPtr endpoints, VectorPtr org)
{
    if ( cons->c_spec_flags & RatioPoint )
        return Update_Ratio_Point_Constraint(cons, endpoints, org);

    cons->c_point = endpoints[0];

    return TRUE;
}

static Boolean
Update_Sphere_Constraint(ConstraintPtr cons, VectorPtr endpoints, VectorPtr org)
{
    Vector  rad_v;

    if ( cons->c_spec_flags & HavePt3 )
        VSub(endpoints[2], endpoints[1], rad_v);
    else
        VSub(endpoints[1], endpoints[0], rad_v);

    cons->c_radius = VMod(rad_v);
    cons->c_point = endpoints[0];

    return TRUE;
}

static Boolean
Update_Circle_Constraint(ConstraintPtr cons, VectorPtr endpoints, VectorPtr org)
{
    Vector  rad_v;

    if ( ! Update_Plane_Constraint(cons, endpoints, org) )
        return FALSE;

    if ( ( cons->c_spec_flags & HavePt6 ) && ( cons->c_spec_flags & HavePt5 ) )
        VSub(endpoints[5], endpoints[4], rad_v);
    else if ( cons->c_spec_flags & HavePt5 )
        VSub(endpoints[4], endpoints[0], rad_v);
    else if ( cons->c_spec_flags & HavePt6 )
        VSub(endpoints[5], endpoints[0], rad_v);
    else
        VSub(endpoints[1], endpoints[0], rad_v);

    cons->c_radius = VMod(rad_v);

    return TRUE;
}

static Boolean
Update_Axis_Constraint(ConstraintPtr cons, VectorPtr endpoints, VectorPtr org)
{
    Vector  temp_v1, temp_v2;
    Vector  dir;
    double  temp_d;

    if ( cons->c_spec_flags & DirAxis )
        dir = endpoints[0];
    else if ( cons->c_spec_flags & HavePt3 )
    {
        VSub(endpoints[1], endpoints[0], temp_v1);
        VSub(endpoints[2], endpoints[0], temp_v2);
        VCross(temp_v1, temp_v2, dir);
    }
    else
        VSub(endpoints[1], endpoints[0], dir);

    if ( VZero(dir) )
        return FALSE;
    VUnit(dir, temp_d, cons->c_vector);
    cons->c_point = *org;

    return TRUE;
}

static Boolean
Update_Ratio_Point_Constraint(ConstraintPtr cons, VectorPtr endpoints,
                              VectorPtr org)
{
    double  ratio;
    Vector  diff;

    if ( IsZero(cons->c_radius + cons->c_ratio) )
        return FALSE;

    ratio = cons->c_radius / ( cons->c_radius + cons->c_ratio );

    VSub(endpoints[1], endpoints[0], diff);
    VScalarMul(diff, ratio, diff);
    VAdd(diff, endpoints[0], cons->c_point);

    return TRUE;
}

static Boolean
Update_Ratio_Plane_Constraint(ConstraintPtr cons, VectorPtr endpoints,
                              VectorPtr org)
{
    Vector  temp_v1;
    double  ratio;
    double  temp_d;

    VSub(endpoints[1], endpoints[0], temp_v1);
    if ( VZero(temp_v1) )
        return FALSE;
    VUnit(temp_v1, temp_d, cons->c_vector);

    if ( IsZero(cons->c_radius + cons->c_ratio) )
        return FALSE;

    ratio = cons->c_radius / ( cons->c_radius + cons->c_ratio );

    VScalarMul(temp_v1, ratio, temp_v1);
    VAdd(temp_v1, endpoints[0], cons->c_point);

    cons->c_value = VDot(cons->c_vector, cons->c_point);

    return TRUE;
}


/*  Boolean
**  Edit_Update_Constraint_Specs(...)
**  Updates all the specifications for the given constraint, and then updates
**  the constraint. Returns TRUE if the constraint changes.
*/
Boolean
Edit_Update_Constraint_Specs(ConstraintPtr cons, VectorPtr center,
                             FeaturePtr features, VectorPtr other_axis)
{
    Vector  org;
    Vector  orig_vect, orig_point;
    double  orig_radius;
    Vector  endpoints[6];   /* The most reqd. */
    Vector  diff;
    int     i;

    orig_vect = cons->c_vector;
    orig_point = cons->c_point;
    orig_radius = cons->c_radius;

    for ( i = 0 ; i < cons->c_num_specs ; i++ )
        update_spec_function[cons->c_specs[i].spec_type](cons->c_specs + i,
                            center, features, other_axis, endpoints + i);

    VAdd(*center, features[origin_feature].location, org);

    if ( ! ( update_con_function[cons->c_type](cons, endpoints, &org) ) )
        return FALSE;

    return ( ! VEqual(orig_vect, cons->c_vector, diff) ||
             ! VEqual(orig_point, cons->c_point, diff) ||
             ! DEqual(orig_radius, cons->c_radius) );
}


Boolean
Edit_Update_Object_Constraints(ConstraintPtr cons, int num, EditInfoPtr info)
{
    int i;
    Boolean result = FALSE;

    for ( i = 0 ; i < num ; i++ )
        result = Edit_Update_Constraint_Specs(cons + i,
                    info->obj->o_world_verts + (info->obj->o_num_vertices - 1),
                    info->obj->o_features, &(info->axes.z))
                || result;

    return result;
}


Boolean
Edit_Update_Active_Object_Cons(ConstraintPtr cons, int num, EditInfoPtr info,
                               VectorPtr center)
{
    int     i;
    Boolean result = FALSE;

    for ( i = 0 ; i < num ; i++ )
        if ( cons[i].c_status || cons[i].c_forced )
        {
            result = Edit_Update_Constraint_Specs(cons + i, center,
                        info->obj->o_features, &(info->axes.z))
                    || result;
        }

    return result;
}


Boolean
Edit_Update_Constraints(EditInfoPtr info)
{
    Boolean     res;
    FeatureType feat;

    res = FALSE;
    for ( feat = origin_feature ; feat < info->obj->o_num_features ; feat++ )
        res = Edit_Update_Object_Constraints(
                info->obj->o_features[feat].constraints,
                info->obj->o_features[feat].num_constraints, info) || res;

    return res;
}

