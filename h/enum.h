/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/enum.h,v 1.0 1997/05/06 20:41:47 schenney Exp $
 *
 * $Log: enum.h,v $
 * Revision 1.0  1997/05/06 20:41:47  schenney
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
**  sced: A Constraint Based Object Scene Editor
**
**  enum.h : A File containing typedefs for the various enumerated types.
**
**  Created: 26/03/94
*/

#ifndef _SCED_ENUM_
#define _SCED_ENUM_


/* The Renderers supported. */
typedef enum _Renderer {
    NoTarget,
    POVray,
    Rayshade,
    Radiance,
    Renderman,
    VRML,
    LastTarget
    } Renderer;


/* The generic objects. */
typedef enum _GenericObject {
    cube_obj,
    sphere_obj,
    cylinder_obj,
    cone_obj,
    triangle_obj,
    plane_obj,
    light_obj,
    spotlight_obj,
    arealight_obj,
    torus_obj,
    dirlight_obj,
    bezier_obj,
    csg_obj,
    wireframe_obj,
    aggregate_obj,
    last_obj
    } GenericObject;
#define NUM_GENERIC_OBJS csg_obj

/* The types of features. */
/* The order matters. Each object has a continuous set starting with origin. */
typedef enum _FeatureType {
    origin_feature,
    major_feature,
    minor_feature,
    scale_feature,
    radius_feature,
    last_feature
    } FeatureType;

typedef enum _PtFeatureType {
    pt_origin_feature,
    pt_major_feature,
    pt_minor_feature,
    pt_scale_feature,
    pt_dummy_feature,
    pt0_feature,
    pt1_feature,
    pt2_feature,
    pt3_feature,
    pt4_feature,
    pt5_feature,
    pt6_feature,
    pt7_feature,
    pt8_feature,
    pt9_feature,
    pt10_feature,
    pt11_feature,
    pt12_feature,
    pt13_feature,
    pt14_feature,
    pt15_feature,
    pt_last_feature
    } PtFeatureType;

typedef enum _TriFeatureType {
    tri_origin_feature,
    tri_major_feature,
    tri_minor_feature,
    tri_scale_feature,
    tri_dummy_feature,
    tri0_feature,
    tri1_feature,
    tri2_feature,
    tri_last_feature
    } TriFeatureType;

/*  The types of feature specifiers.    */
typedef enum _ConstraintSpecType {
    absolute_spec,
    offset_spec,
    reference_spec,
    feature_spec,
    other_spec,
    vertex_spec,
    parameter_spec,
    null_spec
    } ConstraintSpecType;

/*  The various type of constraints.    */
typedef enum _ConstraintType {
    null_feature,   /* No constraint.   */
    plane_feature,
    line_feature,
    point_feature,
    sphere_feature,
    circle_feature,
    inconsistent_feature,   /* An insatiable constraint.    */
    axis_feature,
    ratio_point_feature,
    ratio_plane_feature
    } ConstraintType;
#define NUM_CONSTRAINT_TYPES ( ratio_plane_feature + 1 )


#endif
