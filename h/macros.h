/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/h/RCS/macros.h,v 1.2 1997/06/12 21:59:57 schenney Exp $
 *
 * $Log: macros.h,v $
 * Revision 1.2  1997/06/12 21:59:57  schenney
 * Fixed error in Ref_Transform_Normal
 *
 * Revision 1.1  1997/06/09 19:51:04  schenney
 * Added macros for applying transformations
 *
 * Revision 1.0.1.1  1997/06/03 20:31:13  schenney
 * Added macros for multiplying transformations.
 *
 * Revision 1.0  1997/05/06 20:42:22  schenney
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
**  macros.h : Assorted macros for global use.
**
**  Created: 13/03/94
*/

#ifndef __SCED_MACROS__
#define __SCED_MACROS__

/* Memory allocation macros. */
#define New(t, n) (t*)EMalloc(sizeof(t) * (n))
#define More(x, t, n) (t*)ERealloc((char*)x, sizeof(t) * (n))
#define Strdup(x) strcpy(New(char, strlen(x) + 2), (x))

/* Macros for comparing doubles. */
#define DEqual(a, b)    (((a) > (b) - EPSILON) && ((a) < (b) + EPSILON))
#define IsZero(x)       ((x) < EPSILON && (x) > -EPSILON)

#define MIsIdentity(m) \
                (DEqual((m).x.x, 1.0) && IsZero((m).x.y) && IsZero((m).x.z) && \
                 IsZero((m).y.x) && DEqual((m).y.y, 1.0) && IsZero((m).y.z) && \
                 IsZero((m).z.x) && IsZero((m).z.y) && DEqual((m).z.z, 1.0))

/* Standard min and max macros. */
#define max(a,b)    ((a) > (b) ? (a) : (b))
#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max3(a,b,c) ((a) < (b) ? ( (b) < (c) ? (c) : (b) ) : \
                                 ( (a) < (c) ? (c) : (a) ) )

/* A macro to produce the identity matrix. */
#define NewIdentityMatrix(m) { Vector v1,v2,v3; \
                                VNew(1.0, 0.0, 0.0, v1); \
                                VNew(0.0, 1.0, 0.0, v2); \
                                VNew(0.0, 0.0, 1.0, v3); \
                                MNew(v1, v2, v3, (m)); }

#define Identity_Transform(t) {\
            VNew(0, 0, 0, t.displacement); \
            NewIdentityMatrix(t.matrix); }

#define Point_In_Rect(pt, min, max) \
    ((pt).x >= (min).x && (pt).x <= (max).x && \
     (pt).y >= (min).y && (pt).y <= (max).y)

#define Obj_Is_Light(obj)   ( obj->o_parent && \
                              ( obj->o_parent->b_class == light_obj || \
                                obj->o_parent->b_class == spotlight_obj || \
                                obj->o_parent->b_class == arealight_obj || \
                                obj->o_parent->b_class == dirlight_obj ) )

#define Obj_Is_Point(obj)   ( obj->o_parent && \
                              obj->o_parent->b_class == light_obj )
#define Obj_Is_Dir(obj)     ( obj->o_parent && \
                              obj->o_parent->b_class == dirlight_obj )
#define Obj_Is_Spot(obj)    ( obj->o_parent && \
                              obj->o_parent->b_class == dirlight_obj )
#define Obj_Is_Area(obj)    ( obj->o_parent && \
                              obj->o_parent->b_class == dirlight_obj )

#define Obj_Is_Torus(obj)   ( obj->o_parent && \
                              obj->o_parent->b_class == torus_obj )
#define torus_part(obj)     ( (TorusPtr)(obj)->o_hook )

#define Obj_Is_Triangle(obj)    ( obj->o_parent && \
                                  obj->o_parent->b_class == triangle_obj )
#define Obj_Is_Bezier(obj)      ( obj->o_parent && \
                                  obj->o_parent->b_class == bezier_obj )
#define control_part(obj)       ( (ControlHookPtr)(obj)->o_hook )

#define Obj_Is_Control(obj)     ( obj->o_parent && \
                                  ( obj->o_parent->b_class == triangle_obj || \
                                    obj->o_parent->b_class == bezier_obj ) )

#define Obj_Is_Aggregate(obj)   ( obj->o_parent && \
                                  ( obj->o_parent->b_class == aggregate_obj ) )

#define Obj_Is_Derived(obj)	( obj->o_parent && \
				  ( obj->o_parent->b_class == aggregate_obj || \
				    obj->o_parent->b_class == wireframe_obj || \
				    obj->o_parent->b_class == csg_obj ) )


#define Obj_Is_Construction(obj)    ( obj->o_layer == 1 )

#define Obj_Is_Aliased(obj, rend)   ( obj->o_aliases && obj->o_aliases[rend] )

#define Obj_Is_CSGable(obj) ( obj->o_parent && \
                              ! ( obj->o_parent->b_class == triangle_obj || \
                                  obj->o_parent->b_class == plane_obj || \
                                  obj->o_parent->b_class == bezier_obj || \
                                  obj->o_parent->b_class == aggregate_obj || \
                                  Obj_Is_Light(obj) ) )

#define spec_object(spec)   ((ObjectInstancePtr)(spec)->spec_data)
#define Spec_Is_Dependent(spec) ( spec == reference_spec || \
                                  spec == vertex_spec || \
                                  spec == parameter_spec )

#define Transform_Vector(trans, vect, res) \
            { \
                Vector  _temp_v; \
                MVMul((trans).matrix, (vect), _temp_v); \
                VAdd((trans).displacement, _temp_v, (res)); \
            }

#define Ref_Transform_Vector(obj, vect, res) \
            { \
                if ( Obj_Is_Torus(obj) ) \
                    (res) = Torus_Transform_Vertex(obj, &(vect)); \
                else Transform_Vector((obj)->o_transform, vect, res) \
            }

#define Ref_Transform_Normal(obj, vect, res) \
            { \
                Vector  _temp_v; \
                Matrix  _temp_m; \
		double	_temp_d; \
                MTrans((obj)->o_inverse, _temp_m); \
                MVMul(_temp_m, (vect), _temp_v); \
		VUnit(_temp_v, _temp_d, (res)); \
            }

#define Apply_Transform(orig, new, res) \
            (res).matrix = MMMul(&((new).matrix), &((orig).matrix)); \
            VAdd((orig).displacement, (new).displacement, (res).displacement)

#define Concat_Transform(orig, new, res) \
            (res).matrix = MMMul(&((new).matrix), &((orig).matrix)); \
	    MVMul((new).matrix, (orig).displacement, (res).displacement); \
            VAdd((res).displacement, (new).displacement, (res).displacement)

#define Transform_Vertices(trans, vects, num) \
            { \
                int _i; \
                for ( _i = 0 ; _i < num ; _i++ ) \
                    Transform_Vector(trans, vects[_i], vects[_i]) \
            }

#define Interactive_Resultant(res) \
            ( (res).feature_1.c_type == plane_feature || \
              (res).feature_1.c_type == line_feature || \
              (res).feature_1.c_type == sphere_feature || \
              (res).feature_1.c_type == circle_feature || \
              ( (res).feature_1.c_type == point_feature && \
                (res).feature_2.c_type == point_feature ) )

#define WindowFromWidget(w) \
    ( w == main_window.view_widget ? &main_window : \
      w == agg_window.view_widget ? &agg_window : \
      w == csg_window.view_widget ? &csg_window : NULL)

#endif /* SCED_MACROS */



