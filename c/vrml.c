/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/vrml.c,v 1.1 1997/06/09 20:02:15 schenney Exp schenney $
 *
 * $Log: vrml.c,v $
 * Revision 1.1  1997/06/09 20:02:15  schenney
 * Added code to export base objects
 *
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
**  vrml.c : Functions for exporting VRML files.
*/


#include <math.h>
#include <sced.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <csg.h>
#include <hash.h>
#include <quaternion.h>
#include <time.h> 
#include <torus.h>
#include <Vector4.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

#if ( ! defined(MAXPATHLEN) )
#define MAXPATHLEN 1024
#endif

typedef struct _BaseData {
    Boolean	    non_empty;
    WireframePtr    unattributed;
    } BaseData, *BaseDataPtr;

static void VRML_Export_Camera(FILE*, Camera*);
static void VRML_Export_Lights(FILE*, InstanceList);
static void VRML_Export_Light(FILE*, ObjectInstancePtr);
static void VRML_Export_Instances(FILE*, InstanceList);

static HashTable    wire_hash;

#define Attribs_Defined(a)  ( (a) && ( (a)->defined ) )

static Widget	vrml_base_shell = NULL;
static Widget	solid_button;
static Widget	normal_button;
static Boolean	vrml_done;

int
Export_VRML(FILE *outfile, ScenePtr scene)
{
    BaseDataPtr	base_data;
    char    *time_s;
    time_t  current_time;

    time(&current_time);

    wire_hash = Hash_New_Table();

    /* Required VRML header. */
    fprintf(outfile, "#VRML V2.0 utf8\n");

    /* Comments. */
    fprintf(outfile, "WorldInfo {\n");
    if ( io_file_name )
	fprintf(outfile, "\ntitle \"%s\"\n", io_file_name);
    fprintf(outfile, "\tinfo [\n");
    fprintf(outfile, "\t\t\"File produced by Sced version "VERSION"\"\n");
    time_s = ctime(&current_time);
    time_s[strlen(time_s) - 1] = '\0';
    fprintf(outfile, "\t\t\"%s\"\n", time_s);
    fprintf(outfile, "\t]\n}\n\n");

    /* The scene. */
    fprintf(outfile, "Group { children [\n");

    /* Camera. */
    VRML_Export_Camera(outfile, &(scene->camera));

    /* Lights. */
    if ( scene->light )
        VRML_Export_Light(outfile, scene->light);
    /* Export an ambient only light. */
    fprintf(outfile,
	"PointLight { ambientIntensity 1.0 radius 10000.0 intensity 0.0 }\n");
    VRML_Export_Lights(outfile, scene->instances);

    /* Instances. */
    VRML_Export_Instances(outfile, scene->instances);

    base_data = (BaseDataPtr)Hash_Traverse(wire_hash, TRUE);
    while ( base_data )
    {
	if ( base_data->unattributed )
	    Wireframe_Destroy(base_data->unattributed);
	free(base_data);
	base_data = (BaseDataPtr)Hash_Traverse(wire_hash, FALSE);
    }
    Hash_Free(wire_hash);

    return fprintf(outfile, "] }\n");
}


/* Derived from the unmatrix routine in graphics gems. */
static void
VRML_Decompose_Transform(Transformation *transform, Vector *translation,
			 Vector *scale1, Vector4 *rotate1,
			 Vector *scale2, Vector4 *rotate2,
			 Vector *scale3, Vector4 *rotate3,
			 Vector *scale4, Vector4 *rotate4)
{
    Matrix  colms;
    double  shear_xy, shear_xz, shear_yz;
    double  rotate_x, rotate_y, rotate_z;
    double  theta, gamma, scale_x, scale_y, rho;
    double  sin_t, cos_t;
    Quaternion	q1, q2, rotate_q;
    double  temp_d;
    Vector  temp_v;

    /* Translation is easy. */
    *translation = transform->displacement;

    MTrans(transform->matrix, colms);

    scale4->x = VMod(colms.x);
    VUnit(colms.x, temp_d, colms.x);

    shear_xy = VDot(colms.x, colms.y);
    VScalarMul(colms.x, -shear_xy, temp_v);
    VAdd(temp_v, colms.y, colms.y);

    scale4->y = VMod(colms.y);
    VUnit(colms.y, temp_d, colms.y);
    shear_xy /= scale4->y;

    shear_xz = VDot(colms.x, colms.z);
    VScalarMul(colms.x, -shear_xz, temp_v);
    VAdd(temp_v, colms.z, colms.z);
    shear_yz = VDot(colms.y, colms.z);
    VScalarMul(colms.y, -shear_yz, temp_v);
    VAdd(temp_v, colms.z, colms.z);

    scale4->z = VMod(colms.z);
    VUnit(colms.z, temp_d, colms.z);
    shear_xz /= scale4->z;
    shear_yz /= scale4->z;

    VCross(colms.y, colms.z, temp_v);
    if ( VDot(colms.x, temp_v) < 0.0 )
    {
	VScalarMul(*scale4, -1.0, *scale4);
	MScalarMul(colms, -1.0, colms);
    }

    rotate_y = asin(-colms.x.z);
    if ( cos(rotate_y) != 0 )
    {
	rotate_x = atan2(colms.y.z, colms.z.z);
	rotate_z = atan2(colms.x.y, colms.x.x);
    }
    else
    {
	rotate_x = atan2(colms.y.x, colms.y.y);
	rotate_z = 0;
    }

    /* Decompose the shear_xy matrix. */
    if ( IsZero(shear_xy) )
    {
	theta = 0.0;
	gamma = 0.0;
	scale_x = scale_y = rho = 1.0;
    }
    else
    {
	theta = 0.5 * atan2(2.0, shear_xy);
	scale_x = tan(theta);
	scale_y = 1 / scale_x;
	sin_t = sin(theta);
	cos_t = cos(theta);
	gamma = atan2(-scale_y * sin_t, scale_x * cos_t);
	rho = scale_y * scale_y * sin_t * sin_t +
	      scale_x * scale_x * cos_t * cos_t;
    }

    V4New(0, 0, 1, theta, *rotate4);
    VNew(scale_x * rho, scale_y * rho, 1.0, *scale3);
    if ( gamma < 0.0 )
	VNew(0, 0, -1, temp_v);
    else
	VNew(0, 0, 1, temp_v);
    q1 = Build_Rotate_Quaternion(temp_v, cos(gamma));

    /* Decompose the shear_xz matrix. */
    if ( IsZero(shear_xz) )
    {
	theta = 0.0;
	gamma = 0.0;
	scale_x = scale_y = rho = 1.0;
    }
    else
    {
	theta = -0.5 * atan2(2.0, shear_xz);
	scale_x = tan(theta);
	scale_y = 1 / scale_x;
	sin_t = sin(theta);
	cos_t = cos(theta);
	gamma = atan2(-scale_y * sin_t, scale_x * cos_t);
	rho = scale_y * scale_y * sin_t * sin_t +
	      scale_x * scale_x * cos_t * cos_t;
    }

    if ( theta < 0.0 )
	VNew(0, -1, 0, temp_v);
    else
	VNew(0, 1, 0, temp_v);
    q2 = Build_Rotate_Quaternion(temp_v, cos(theta));
    VNew(scale_x * rho, 1.0, scale_y * rho, *scale2);

    rotate_q = QQMul(&q2, &q1);
    Quaternion_To_Axis_Angle(&rotate_q, &temp_v, &(rotate3->w));
    VNew(temp_v.x, temp_v.y, temp_v.z, *rotate3);

    if ( gamma < 0.0 )
	VNew(0, -1, 0, temp_v);
    else
	VNew(0, 1, 0, temp_v);
    q1 = Build_Rotate_Quaternion(temp_v, cos(gamma));

    /* Decompose the shear_yz matrix. */
    if ( IsZero(shear_yz) )
    {
	theta = 0.0;
	gamma = 0.0;
	scale_x = scale_y = rho = 1.0;
    }
    else
    {
	theta = 0.5 * atan2(2.0, shear_yz);
	scale_x = tan(theta);
	scale_y = 1 / scale_x;
	sin_t = sin(theta);
	cos_t = cos(theta);
	gamma = atan2(-scale_y * sin_t, scale_x * cos_t);
	rho = scale_y * scale_y * sin_t * sin_t +
	      scale_x * scale_x * cos_t * cos_t;
    }

    if ( theta < 0.0 )
	VNew(-1, 0, 0, temp_v);
    else
	VNew(1, 0, 0, temp_v);
    q2 = Build_Rotate_Quaternion(temp_v, cos(theta));
    VNew(1.0, scale_x * rho, scale_y * rho, *scale1);

    rotate_q = QQMul(&q2, &q1);
    Quaternion_To_Axis_Angle(&rotate_q, &temp_v, &(rotate2->w));
    VNew(temp_v.x, temp_v.y, temp_v.z, *rotate2);

    if ( gamma < 0.0 )
	VNew(-1, 0, 0, temp_v);
    else
	VNew(1, 0, 0, temp_v);
    q1 = Build_Rotate_Quaternion(temp_v, cos(gamma));

    /* Apply all the rotations. */
    if ( rotate_x < 0.0 )
	VNew(-1, 0, 0, temp_v);
    else
	VNew(1, 0, 0, temp_v);
    q2 = Build_Rotate_Quaternion(temp_v, cos(rotate_x));
    rotate_q = QQMul(&q2, &q1);

    if ( rotate_y < 0.0 )
	VNew(0, -1, 0, temp_v);
    else
	VNew(0, 1, 0, temp_v);
    q1 = Build_Rotate_Quaternion(temp_v, cos(rotate_y));
    rotate_q = QQMul(&q1, &rotate_q);
    
    if ( rotate_z < 0.0 )
	VNew(0, 0, -1, temp_v);
    else
	VNew(0, 0, 1, temp_v);
    q1 = Build_Rotate_Quaternion(temp_v, cos(rotate_z));
    rotate_q = QQMul(&q1, &rotate_q);

    Quaternion_To_Axis_Angle(&rotate_q, &temp_v, &(rotate1->w));
    VNew(temp_v.x, temp_v.y, temp_v.z, *rotate1);
}
			 

static void
VRML_Export_Camera(FILE *outfile, Camera *cam)
{
    Viewport    cam_view;
    Quaternion  rot_quat;
    Vector      rot_axis;
    double      rot_angle;
    Vector      norm_axis;
    Quaternion  norm_quat;
    Quaternion  inv_norm_quat;
    Quaternion  y_quat, new_y_quat, rot_y_quat;
    Vector      new_y;
    double      temp_d;

    Vector      temp_v;

    Camera_To_Viewport(cam, &cam_view);

    VNew(cam_view.world_to_view.matrix.z.y, -cam_view.world_to_view.matrix.z.x,
         0, norm_axis);
    if ( VZero(norm_axis) )
    {
        if ( cam_view.world_to_view.matrix.z.z > 0.0 )
        {
            norm_quat.real_part = 0.0;
            VNew(0, 1, 0, norm_quat.vect_part);
        }
        else
        {
            norm_quat.real_part = 1;
            VNew(0, 0, 0, norm_quat.vect_part);
        }
    }
    else
    {
        VUnit(norm_axis, temp_d, norm_axis);
        norm_quat = Build_Rotate_Quaternion(norm_axis,
                                            -cam_view.world_to_view.matrix.z.z);
    }

    /* Need to rotate the world y vector to see where it ends up. */
    inv_norm_quat.real_part = norm_quat.real_part;
    VScalarMul(norm_quat.vect_part, -1, inv_norm_quat.vect_part);

    y_quat.real_part = 0.0;
    VNew(0, 1, 0, y_quat.vect_part);
    new_y_quat = QQMul(&norm_quat, &y_quat);
    new_y_quat = QQMul(&new_y_quat, &inv_norm_quat);
    new_y = new_y_quat.vect_part;

    /* Now need to find out how much to rotate about n to line up y. */
    VCross(new_y, cam_view.world_to_view.matrix.y, temp_v);
    if ( ! VZero(temp_v) )
        VUnit(temp_v, temp_d, temp_v);
    rot_y_quat = Build_Rotate_Quaternion(temp_v,
                                VDot(new_y, cam_view.world_to_view.matrix.y));

    rot_quat = QQMul(&rot_y_quat, &norm_quat);
    Quaternion_To_Axis_Angle(&rot_quat, &rot_axis, &rot_angle);

    fprintf(outfile, "Viewpoint {\n");
    fprintf(outfile, "\tposition %1.5g %1.5g %1.5g\n",
            cam->location.x, cam->location.y, cam->location.z);
    fprintf(outfile, "\torientation %1.5g %1.5g %1.5g %1.5g\n",
            rot_axis.x, rot_axis.y, rot_axis.z, rot_angle);
    fprintf(outfile, "\tfieldOfView %1.5g\n",
	    max(cam->vert_fov, cam->horiz_fov) * M_PI / 180.0);
    fprintf(outfile, "}\n");
}

static int
VRML_Export_Transform(FILE *outfile, Transformation *transform)
{
    Vector  translation;
    Vector4 rotate1;
    Vector  scale1;
    Vector4 rotate2;
    Vector  scale2;
    Vector4 rotate3;
    Vector  scale3;
    Vector4 rotate4;
    Vector  scale4;
    Vector  identity_scale;
    Vector  diff;
    int	    count = 0;

    VNew(1, 1, 1, identity_scale);

    /* Have to decompose the transformation matrix. */
    VRML_Decompose_Transform(transform, &translation,
			     &scale1, &rotate1, &scale2, &rotate2,
			     &scale3, &rotate3, &scale4, &rotate4);

    /* Check for the most common case. */
    if ( IsZero(rotate1.w) && IsZero(rotate2.w) && IsZero(rotate3.w) &&
	 VEqual(scale2, identity_scale, diff) &&
	 VEqual(scale3, identity_scale, diff) &&
	 VEqual(scale4, identity_scale, diff) )
    {
	fprintf(outfile, "Transform {\n");
	fprintf(outfile, "\tscale %f %f %f\n", scale1.x, scale1.y, scale1.z);
	fprintf(outfile, "\trotation %f %f %f %f\n",
		rotate4.x, rotate4.y, rotate4.z, rotate4.w);
	fprintf(outfile, "\ttranslation %f %f %f\n",
		translation.x, translation.y, translation.z);
	fprintf(outfile, "children [\n");
	return 1;
    }

    fprintf(outfile, "Transform {\n");

    count++;

    fprintf(outfile, "\tscale %f %f %f\n", scale1.x, scale1.y, scale1.z);
    fprintf(outfile, "\trotation %f %f %f %f\n",
	    rotate1.x, rotate1.y, rotate1.z, rotate1.w);
    fprintf(outfile, "\ttranslation %f %f %f\n",
	    translation.x, translation.y, translation.z);

    if ( ! VEqual(scale2, identity_scale, diff) || ! IsZero(rotate2.w) )
    {
	fprintf(outfile, "\nchildren [ Transform {\n");
	fprintf(outfile, "\tscale %f %f %f\n", scale2.x, scale2.y, scale2.z);
	fprintf(outfile, "\trotation %f %f %f %f\n",
		rotate2.x, rotate2.y, rotate2.z, rotate2.w);
	count++;
    }

    if ( ! VEqual(scale3, identity_scale, diff) || ! IsZero(rotate3.w) )
    {
	fprintf(outfile, "\nchildren [ Transform {\n");
	fprintf(outfile, "\tscale %f %f %f\n", scale3.x, scale3.y, scale3.z);
	fprintf(outfile, "\trotation %f %f %f %f\n",
		rotate3.x, rotate3.y, rotate3.z, rotate3.w);
	count++;
    }

    if ( ! VEqual(scale4, identity_scale, diff) || ! IsZero(rotate4.w) )
    {
	fprintf(outfile, "\nchildren [ Transform {\n");
	fprintf(outfile, "\tscale %f %f %f\n", scale4.x, scale4.y, scale4.z);
	fprintf(outfile, "\trotation %f %f %f %f\n",
		rotate4.x, rotate4.y, rotate4.z, rotate4.w);
	count++;
    }

    fprintf(outfile, "children [\n");

    return count;
}


static void
VRML_Export_Light(FILE *outfile, ObjectInstancePtr obj)
{
    Vector  color;
    double  intensity, temp_d;
    Vector  vect1, vect2;

    intensity = max(obj->o_attribs->intensity.red,
                    obj->o_attribs->intensity.green);
    intensity = max(intensity, obj->o_attribs->intensity.blue);
    color.x = obj->o_attribs->intensity.red;
    color.y = obj->o_attribs->intensity.green;
    color.z = obj->o_attribs->intensity.blue;
    if ( ! IsZero(intensity) )
    {
        temp_d = 1 / intensity;
        VScalarMul(color, temp_d, color);
    }

    switch ( obj->o_parent->b_class )
    {
        case arealight_obj:
            /* These aren't supported, so just export as a point. */
        case light_obj:
            fprintf(outfile, "PointLight {\n");
            fprintf(outfile, "\tintensity %f\n", intensity);
            fprintf(outfile, "\tcolor %g %g %g\n", color.x, color.y, color.z);
            fprintf(outfile, "\tlocation %1.5g %1.5g %1.5g\n",
                    obj->o_transform.displacement.x,
                    obj->o_transform.displacement.y,
                    obj->o_transform.displacement.z);
            fprintf(outfile, "}\n");
            break;

        case spotlight_obj:
            {
                double  radius;
                double  cos_rad;
                Vector  direction;

                /* Calculate the radius. */
                VSub(obj->o_world_verts[0], obj->o_world_verts[9], vect1);
                VSub(obj->o_world_verts[8], obj->o_world_verts[9], vect2);
                VUnit(vect1, radius, vect1);
                VUnit(vect2, radius, vect2);
                cos_rad = VDot(vect1, vect2);
                radius = acos(cos_rad);

                if ( obj->o_attribs->invert )
                {
                    /* Invert it. */
                    /* vect2 still points toward direction. */
                    VScalarMul(vect2, -1.0, vect2);
                    VAdd(vect2, obj->o_world_verts[9], direction);

                    radius += M_PI_2;
                }
                else
                    direction = obj->o_world_verts[8];

                fprintf(outfile, "SpotLight {\n");
                fprintf(outfile, "\tintensity %g\n", intensity);
                fprintf(outfile, "\tcolor %g %g %g\n",color.x,color.y,color.z);
                fprintf(outfile, "\tlocation %1.5g %1.5g %1.5g\n",
                        obj->o_transform.displacement.x,
                        obj->o_transform.displacement.y,
                        obj->o_transform.displacement.z);
                fprintf(outfile, "\tdirection %1.15g %1.15g %1.15g\n",
                        direction.x, direction.y, direction.z);
                fprintf(outfile, "\tdropOffRate 0\n");
                fprintf(outfile, "\tcutOffAngle %f\n", radius);
                fprintf(outfile, "}\n");
            }
            break;

        case dirlight_obj:
            VSub(obj->o_world_verts[5], obj->o_world_verts[0], vect1);
            fprintf(outfile, "DirectionalLight {\n");
            fprintf(outfile, "\tintensity %g\n", intensity);
            fprintf(outfile, "\tcolor %g %g %g\n", color.x, color.y, color.z);
            fprintf(outfile, "\tdirection ");   VPrint(outfile, vect1);
            fprintf(outfile, "}\n");
            break;

        default:;
    }
    fprintf(outfile, "\n");
}

static void
VRML_Export_Lights(FILE *outfile, InstanceList insts)
{
    InstanceList    elmt;
    int		    num_trans, i;

    /* Go through all the instances looking for lights. */
    for ( elmt = insts ; elmt ; elmt = elmt->next )
    {
        if ( Obj_Is_Light(elmt->the_instance) &&
             ! Obj_Is_Construction(elmt->the_instance) )
            VRML_Export_Light(outfile, elmt->the_instance);
	else if ( Obj_Is_Aggregate(elmt->the_instance) )
	{
	    num_trans = VRML_Export_Transform(outfile,
					    &(elmt->the_instance->o_transform));
	    VRML_Export_Lights(outfile,
	      ((AggregatePtr)elmt->the_instance->o_parent->b_struct)->children);
	    for ( i = 0 ; i < num_trans ; i++ )
		fprintf(outfile, "] } ");
	    fprintf(outfile, "\n\n");
	}
    }
}


static void
VRML_Export_Attributes(FILE *outfile, AttributePtr attribs)
{
    if ( ! Attribs_Defined(attribs) )
        return;

    if ( attribs->use_extension && attribs->extension[VRML] )
	/* Extension should be a valid appearance node. */
        fprintf(outfile, "%s\n", attribs->extension[VRML]);
    else
    {
        fprintf(outfile, "appearance Appearance { material Material {\n");
        fprintf(outfile, "\tambientIntensity 1.0\n");
        fprintf(outfile, "\tdiffuseColor %g %g %g\n",
                attribs->diff_coef * attribs->color.red,
                attribs->diff_coef * attribs->color.green, 
                attribs->diff_coef * attribs->color.blue);
        fprintf(outfile, "\tspecularColor %g %g %g\n",
                attribs->spec_coef * attribs->color.red,
                attribs->spec_coef * attribs->color.green,
                attribs->spec_coef * attribs->color.blue);
        fprintf(outfile, "\tshininess %g\n", attribs->reflect_coef);
        fprintf(outfile, "\ttransparency %g\n", attribs->transparency);
        fprintf(outfile, "} }\n");
    }
}

static void
VRML_Export_Wireframe_Geom(FILE *outfile, WireframePtr wire, Boolean solid,
			   Boolean do_normals)
{
    int	i;

    fprintf(outfile, "IndexedFaceSet {\n");
    if ( ! solid )
	fprintf(outfile, "\tsolid FALSE\n");
    fprintf(outfile, "\tcoord Coordinate { point [\n");
    for ( i = 0 ; i < wire->num_vertices ; i++ )
	fprintf(outfile, "\t\t%1.5g %1.5g %1.5g\n",
		wire->vertices[i].x, wire->vertices[i].y, wire->vertices[i].z);
    fprintf(outfile, "\t] }\n");
    if ( do_normals )
    {
	fprintf(outfile, "\tnormal Normal { vector [\n");
	for ( i = 0 ; i < wire->num_normals ; i++ )
	    fprintf(outfile, "\t\t%1.5g %1.5g %1.5g\n",
		    wire->normals[i].x, wire->normals[i].y, wire->normals[i].z);
	fprintf(outfile, "\t] }\n");
    }
    fprintf(outfile, "\tcoordIndex [\n");
    for ( i = 0 ; i < wire->num_faces ; i++ )
	fprintf(outfile, "\t\t%d %d %d -1\n", wire->faces[i].vertices[2],
		wire->faces[i].vertices[1], wire->faces[i].vertices[0]);
    fprintf(outfile, "\t]\n");
    if ( do_normals )
    {
	fprintf(outfile, "\tnormalIndex [\n");
	for ( i = 0 ; i < wire->num_faces ; i++ )
	    fprintf(outfile, "\t\t%d %d %d -1\n", wire->faces[i].normals[2],
		    wire->faces[i].normals[1], wire->faces[i].normals[0]);
	fprintf(outfile, "\t]\n");
    }
    fprintf(outfile, "}\n");
}


static int
VRML_Compare_Func(const void *a, const void *b)
{
    AttributePtr    a_val, b_val;

    if ( ((FacePtr)a)->face_attribs && ((FacePtr)a)->face_attribs->defined )
        a_val = ((FacePtr)a)->face_attribs;
    else
        a_val = NULL;

    if ( ((FacePtr)b)->face_attribs && ((FacePtr)b)->face_attribs->defined )
        b_val = ((FacePtr)b)->face_attribs;
    else
        b_val = NULL;

    if ( a_val == b_val )
        return 0;
    else if ( a_val < b_val )
        return -1;
    else
        return 1;

    return 1; /* To keep compilers happy. */
}


static void
VRML_Export_Base_Wireframe(FILE *outfile, WireframePtr wire,
			   AttributePtr attribs, Boolean do_def, Boolean solid,
			   Boolean do_normals)
{
    BaseDataPtr	    base_data = New(BaseData, 1);
    short	    *v_map, *n_map;
    int		    num_f;
    int		    i, j, k;
    int 	    start_i;
    int		    count;
    AttributePtr    last_attribs;

    base_data->non_empty = FALSE;
    base_data->unattributed = NULL;

    Hash_Insert(wire_hash, (unsigned long)wire, (void*)base_data);

    if ( ! wire->num_faces )
        return;

    /* We wish to sort faces based on attributes. This is so we
    ** can easily export faces with identical attributes in one block.
    */
    qsort((void*)wire->faces, wire->num_faces, sizeof(Face), VRML_Compare_Func);

    if ( wire->faces[0].face_attribs == NULL ||
	 ! wire->faces[0].face_attribs->defined )
    {
	/* There are unattributed faces. */
	base_data->unattributed = Wireframe_Copy(wire);
	for ( num_f = 0 ; num_f < wire->num_faces &&
	      ( wire->faces[num_f].face_attribs == NULL ||
		! wire->faces[num_f].face_attribs->defined ) ;
	      num_f++ );
	base_data->unattributed->num_faces = num_f;

	fprintf(outfile, "Shape {\n");

	VRML_Export_Attributes(outfile, attribs);
	if ( do_def )
	    fprintf(outfile, "geometry DEF Wire_Un_%ld\n",
		    (unsigned long)base_data->unattributed);
	else
	    fprintf(outfile, "geometry\n");
	VRML_Export_Wireframe_Geom(outfile, base_data->unattributed, solid,
				   do_normals);

	fprintf(outfile, "}\n");

	if ( num_f == wire->num_faces )
	    return;
    }
    else
	num_f = 0;

    base_data->non_empty = TRUE;

    v_map = New(short, wire->num_vertices);
    n_map = New(short, wire->num_normals);

    if ( do_def )
	fprintf(outfile,
		"DEF Wire_%ld Group { children [\n", (unsigned long)wire);
    else
	fprintf(outfile, "Group { children [\n");

    for ( i = num_f, start_i = num_f ; start_i < wire->num_faces ; start_i = i )
    {
	last_attribs = wire->faces[i].face_attribs;
	for ( last_attribs = wire->faces[i].face_attribs ;
	      i < wire->num_faces &&
	        wire->faces[i].face_attribs == last_attribs ; i++ );

	fprintf(outfile, "Shape {\n");
	VRML_Export_Attributes(outfile, last_attribs);

	for ( j = 0 ; j < wire->num_vertices ; j++ )
	    v_map[j] = -1;
	for ( j = 0 ; j < wire->num_normals ; j++ )
	    n_map[j] = -1;

	fprintf(outfile, "\tgeometry IndexedFaceSet {\n");
	if ( ! solid )
	    fprintf(outfile, "\t\tsolid FALSE\n");
	fprintf(outfile, "\t\tcoord Coordinate { point [ \n");
	count = 0;
	for ( j = start_i ; j < i ; j++ )
	{
	    for ( k = 0 ; k < 3 ; k++ )
		if ( v_map[wire->faces[j].vertices[k]] == -1 )
		{
		    v_map[wire->faces[j].vertices[k]] = count;
		    fprintf(outfile, "\t\t%f %f %f\n",
			    wire->vertices[wire->faces[j].vertices[k]].x,
			    wire->vertices[wire->faces[j].vertices[k]].y,
			    wire->vertices[wire->faces[j].vertices[k]].z);
		    count++;
		}
	}
	fprintf(outfile, "\t\t] }\n");
	if ( do_normals )
	{
	    fprintf(outfile, "\t\tnormal Normal { vector [ \n");
	    count = 0;
	    for ( j = start_i ; j < i ; j++ )
	    {
		for ( k = 0 ; k < 3 ; k++ )
		    if ( n_map[wire->faces[j].normals[k]] == -1 )
		    {
			n_map[wire->faces[j].normals[k]] = count;
			fprintf(outfile, "\t\t%f %f %f\n",
				wire->normals[wire->faces[j].normals[k]].x,
				wire->normals[wire->faces[j].normals[k]].y,
				wire->normals[wire->faces[j].normals[k]].z);
			count++;
		    }
	    }
	    fprintf(outfile, "\t\t] }\n");
	}
	fprintf(outfile, "\t\tcoordIndex [\n");
	for ( j = start_i ; j < i ; j++ )
	    fprintf(outfile, "%d %d %d -1\n",
		    v_map[wire->faces[j].vertices[2]],
		    v_map[wire->faces[j].vertices[1]],
		    v_map[wire->faces[j].vertices[0]]);
	fprintf(outfile, "\t\t]");
	if ( do_normals )
	{
	    fprintf(outfile, "\t\tnormalIndex [\n");
	    for ( j = start_i ; j < i ; j++ )
		fprintf(outfile, "%d %d %d -1\n",
			n_map[wire->faces[j].normals[2]],
			n_map[wire->faces[j].normals[1]],
			n_map[wire->faces[j].normals[0]]);
	    fprintf(outfile, "\t\t]");
	}

        fprintf(outfile, " } }\n");    /* End of IndexedFaceSet & shape. */
    }

    fprintf(outfile, "] }\n\n");    /* End of children and group */

    free(v_map);
    free(n_map);

    return;
}


static void
VRML_Export_Wireframe_Shape(FILE *outfile, WireframePtr wire,
			    AttributePtr attribs, Boolean base_obj,
			    Boolean solid, Boolean do_normals)
{
    BaseDataPtr	    base_data;
    WireframePtr    unattributed_wire = NULL;

    if ( base_obj )
    {
	/* Check if it has already been defined. */
	base_data = (BaseDataPtr)Hash_Get_Value(wire_hash, (unsigned long)wire);
	if ( base_data == (BaseDataPtr)-1 )
	{
	    VRML_Export_Base_Wireframe(outfile, wire, attribs, TRUE, solid,
				       do_normals);
	    return;
	}

	if ( base_data->non_empty )
	    fprintf(outfile, "USE Wire_%ld\n", (unsigned long)wire);
	if ( base_data->unattributed )
	{
	    fprintf(outfile, "Shape {\n");
	    VRML_Export_Attributes(outfile, attribs);
	    fprintf(outfile, "geometry USE Wire_Un_%ld\n",
		    (unsigned long)base_data->unattributed);
	    fprintf(outfile, "}\n");
	}
    }
    else
    {
	fprintf(outfile, "Shape {\n");

	VRML_Export_Attributes(outfile, attribs);
	fprintf(outfile, "geometry ");
	VRML_Export_Wireframe_Geom(outfile, unattributed_wire, solid,
				   do_normals);

	fprintf(outfile, "}\n");
    }
}

static void
VRML_Export_Base_Object(FILE *outfile, ObjectInstancePtr obj)
{
    if ( ! obj->o_wireframe )
    {
	/* Must be a CSG preview. */
	obj->o_wireframe =
	    CSG_Generate_Wireframe((CSGNodePtr)obj->o_parent->b_struct,
				   0, FALSE);
	VRML_Export_Wireframe_Shape(outfile, obj->o_wireframe,
				    (AttributePtr)obj->o_attribs, TRUE, TRUE,
				    TRUE);
	Wireframe_Destroy(obj->o_wireframe);
	obj->o_wireframe = NULL;
    }
    else
	VRML_Export_Wireframe_Shape(outfile,
	    obj->o_parent->b_major_wires[Wireframe_Density_Level(obj)],
	    (AttributePtr)obj->o_attribs, TRUE, TRUE, TRUE);
}


static void
VRML_Export_LOD_Wireframes(FILE *outfile, WireframePtr *wireframes,
                           LODInfoPtr lods, int num_wires,
			   AttributePtr attribs, Boolean base)
{
    int i, j;

    if ( lods->num_lods >= num_wires )
    {
        if ( num_wires == 1 )
        {
            VRML_Export_Wireframe_Shape(outfile, wireframes[0], attribs, base,
					TRUE, TRUE);
            return;
        }
        else
            i = lods->num_lods - num_wires + 1;
    }
    else
        i = 0;

    fprintf(outfile, "LOD {\n\trange [ ");
    fprintf(outfile, "%g ", lods->lods[i++]);
    for ( ; i < lods->num_lods ; i++ )
        fprintf(outfile, ", %g ", lods->lods[i]);
    fprintf(outfile, "]\n");

    fprintf(outfile, "\tlevel [ ");

    for ( j = num_wires - 1, i = 0 ; i <= lods->num_lods && j >= 0 ; j--, i++ )
        VRML_Export_Wireframe_Shape(outfile, wireframes[j], attribs, base,
				    TRUE, TRUE);

    fprintf(outfile, "] }\n");
}


static void
VRML_Export_Geometry(FILE *outfile, ObjectInstancePtr obj)
{
    WireframePtr    wire;

    switch ( obj->o_parent->b_class )
    {
        case cube_obj:
            fprintf(outfile, "geometry Box {}\n");
            break;

        case cylinder_obj:
            /* Need to rotate it. */
            if ( Attribs_Defined((AttributePtr)obj->o_attribs) &&
                 ((AttributePtr)obj->o_attribs)->use_extension &&
                 ((AttributePtr)obj->o_attribs)->open )
                fprintf(outfile,
			"geometry Cylinder { bottom FALSE top FALSE }\n");
            else
                fprintf(outfile, "geometry Cylinder {}\n");
            break;

        case cone_obj:
            /* Need to rotate it. */
            if ( Attribs_Defined((AttributePtr)obj->o_attribs) &&
                 ((AttributePtr)obj->o_attribs)->use_extension &&
                 ((AttributePtr)obj->o_attribs)->open )
                fprintf(outfile, "geometry Cone { bottom FALSE }\n");
            else
                fprintf(outfile, "geometry Cone {}\n");
            break;

        case sphere_obj:
            fprintf(outfile, "geometry Sphere {}\n");
            break;

        case plane_obj:
            fprintf(outfile, "geometry IndexedFaceSet {\n");
            fprintf(outfile, "\tcoord Coordinate { point ");
	    fprintf(outfile, "[ 10 10 0 -10 10 0 -10 -10 0 10 -10 0 ] }\n");
            fprintf(outfile, "coordIndex [ 0 3 2 1 -1 ]\n");
	    fprintf(outfile, "solid FALSE\n");
            fprintf(outfile, "}\n");
            break;

        case triangle_obj:
            fprintf(outfile, "geometry IndexedFaceSet {\n");
            fprintf(outfile, "\tcoord Coordinate { point [ ");
            fprintf(outfile, "%1.5g %1.5g %1.5g ",
                    control_part(obj)->control_verts[0].x,
                    control_part(obj)->control_verts[0].y,
                    control_part(obj)->control_verts[0].z);
            fprintf(outfile, "%1.5g %1.5g %1.5g ",
                    control_part(obj)->control_verts[1].x,
                    control_part(obj)->control_verts[1].y,
                    control_part(obj)->control_verts[1].z);
            fprintf(outfile, "%1.5g %1.5g %1.5g ",
                    control_part(obj)->control_verts[2].x,
                    control_part(obj)->control_verts[2].y,
                    control_part(obj)->control_verts[2].z);
            fprintf(outfile, "] }\n");
            fprintf(outfile, "coordIndex [ 0, 1, 2, -1 ]\n");
	    fprintf(outfile, "solid FALSE\n");
            fprintf(outfile, "}\n");
            break;

        case torus_obj:
        case bezier_obj:
	    wire = Object_To_Wireframe(obj, FALSE);
	    fprintf(outfile, "geometry ");
	    VRML_Export_Wireframe_Geom(outfile, wire,
				    obj->o_parent->b_class == torus_obj, TRUE);
	    Wireframe_Destroy(wire);
            break;

        default:;
    }
}


static void
VRML_Export_LOD_Object(FILE *outfile, ObjectInstancePtr obj)
{
    WireframePtr    *wires;
    WireframePtr    orig_wire = obj->o_wireframe;
    int             num_wires;
    int             i;

    if ( obj->o_parent->b_class == aggregate_obj )
    {
	VRML_Export_Base_Object(outfile, obj);
	return;
    }
    if ( obj->o_parent->b_class != csg_obj &&
	 obj->o_parent->b_class != wireframe_obj &&
	 obj->o_parent->b_class != bezier_obj &&
	 obj->o_parent->b_class != torus_obj )
    {
	fprintf(outfile, "Shape {\n");

	VRML_Export_Attributes(outfile, (AttributePtr)obj->o_attribs);
	VRML_Export_Geometry(outfile, obj);

	fprintf(outfile, "}\n");
	return;
    }

    if ( obj->o_parent->b_class == csg_obj ||
	 obj->o_parent->b_class == wireframe_obj )
	VRML_Export_LOD_Wireframes(outfile, obj->o_parent->b_major_wires,
				   obj->o_lods, obj->o_parent->b_max_density,
				   (AttributePtr)obj->o_attribs, TRUE);
    else
    {
	num_wires = Wireframe_Density_Level(obj) + 1;

	wires = New(WireframePtr, num_wires);
	for ( i = 0 ; i < num_wires ; i++ )
	{
	    obj->o_wireframe = obj->o_parent->b_major_wires[i];
	    wires[i] = Object_To_Wireframe(obj, FALSE);
	}

	VRML_Export_LOD_Wireframes(outfile, wires, obj->o_lods, num_wires,
				   (AttributePtr)obj->o_attribs, FALSE);

	for ( i = 0 ; i < num_wires ; i++ )
	    Wireframe_Destroy(wires[i]);
	free(wires);
	obj->o_wireframe = orig_wire;
    }

}


static void
VRML_Export_Alias(FILE *outfile, ObjectInstancePtr obj)
{
    char    *alias = (char*)obj->o_aliases[VRML];
    int     index;

    fprintf(outfile, "Group { children [\n");

    index = 0;
    while ( alias[index] != '\0' )
    {
        if ( alias[index] == '(' && alias[index + 1] == '*' )
        {
            if ( ! strncmp(alias + index, "(*transform*)", 13) )
            {
                VRML_Export_Transform(outfile, &(obj->o_transform));
                index += 13;
            }
            else if ( ! strncmp(alias + index, "(*attributes*)", 14) )
            {
                VRML_Export_Attributes(outfile, (AttributePtr)obj->o_attribs);
                index += 14;
            }
            else if ( ! strncmp(alias + index, "(*geometry*)", 12) )
            {
                VRML_Export_Geometry(outfile, obj);
                index += 12;
            }
            else
                fputc((int)alias[index++], outfile);
        }
        else
            fputc((int)alias[index++], outfile);
    }

    fprintf(outfile, "] }\n\n");
}

static void
VRML_Export_Object(FILE *outfile, ObjectInstancePtr obj)
{
    int	num_trans;
    int	i;

    fprintf(outfile, "# %s\n", obj->o_label);

    if ( Obj_Is_Aliased(obj, VRML) )
    {
        VRML_Export_Alias(outfile, obj);
        return;
    }

    num_trans = VRML_Export_Transform(outfile, &(obj->o_transform));

    if ( obj->o_parent->b_class == cylinder_obj ||
	 obj->o_parent->b_class == cone_obj )
    {
	fprintf(outfile, "Transform { rotation 1 0 0 %g\n", M_PI_2);
	fprintf(outfile, "children [\n");
	num_trans++;
    }

    if ( obj->o_lods )
	VRML_Export_LOD_Object(outfile, obj);
    else if ( Obj_Is_Derived(obj) )
	VRML_Export_Base_Object(outfile, obj);
    else
    {
	fprintf(outfile, "Shape {\n");

	VRML_Export_Attributes(outfile, (AttributePtr)obj->o_attribs);
	VRML_Export_Geometry(outfile, obj);

	fprintf(outfile, "}\n");
    }

    for ( i = 0 ; i < num_trans ; i++ )
	fprintf(outfile, "] } ");
    fprintf(outfile, "\n\n");
}


static void
VRML_Export_Instances(FILE *outfile, InstanceList insts)
{
    for ( ; insts ; insts = insts->next )
        if ( ! Obj_Is_Light(insts->the_instance) &&
             ! Obj_Is_Construction(insts->the_instance) )
            VRML_Export_Object(outfile, insts->the_instance);
}


static void
VRML_Base_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    vrml_done = TRUE;
}


static void
VRML_Create_Shell()
{
    Widget  dialog_form;
    Widget  buttons[4];
    Arg     args[15];
    int     n, m, count;

    vrml_base_shell = XtCreatePopupShell("Options",
                        transientShellWidgetClass, main_window.shell, NULL, 0);

    n = 0;
    dialog_form = XtCreateManagedWidget("vrmlBaseForm", formWidgetClass,
					vrml_base_shell, args, n);

    count = 0;

    n = 0;
    XtSetArg(args[n], XtNlabel, "Options"); 	n++;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom,XtChainTop);    n++;
    XtSetArg(args[n], XtNborderWidth, 0);	n++;
    buttons[count++] =
     XtCreateManagedWidget("vrmlLabel", labelWidgetClass, dialog_form, args, n);

    m = 0;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom,XtChainTop);    m++;

    n = m;
    XtSetArg(args[n], XtNlabel, "Solid TRUE");		n++;
    XtSetArg(args[n], XtNfromVert, buttons[count - 1]); n++;
    solid_button =
    buttons[count++] = XtCreateManagedWidget("solidToggle", toggleWidgetClass,
					     dialog_form, args, n);
    
    n = m;
    XtSetArg(args[n], XtNlabel, "Set Normals");		n++;
    XtSetArg(args[n], XtNfromVert, buttons[count - 1]); n++;
    normal_button =
    buttons[count++] = XtCreateManagedWidget("normalToggle", toggleWidgetClass,
					     dialog_form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Do Export");		n++;
    XtSetArg(args[n], XtNfromVert, buttons[count - 1]); n++;
    buttons[count] = XtCreateManagedWidget("exportButton", commandWidgetClass,
					   dialog_form, args, n);
    XtAddCallback(buttons[count], XtNcallback, VRML_Base_Callback, NULL);
    count++;
    
    Match_Widths(buttons, count);
    XtRealizeWidget(vrml_base_shell);
}


static void
VRML_Get_Base_Info(Boolean *solid, Boolean *do_normals)
{
    XtAppContext    context;
    XEvent          event;

    if ( ! vrml_base_shell )
	VRML_Create_Shell();

    Position_Shell(vrml_base_shell, FALSE);
    XtPopup(vrml_base_shell, XtGrabExclusive);

    vrml_done = FALSE;
    context = XtWidgetToApplicationContext(main_window.shell);
    while ( ! vrml_done )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }

    XtPopdown(vrml_base_shell);

    XtVaGetValues(solid_button, XtNstate, solid, NULL);
    XtVaGetValues(normal_button, XtNstate, do_normals, NULL);
}


void
VRML_Export_Base(BaseObjectPtr base, char *name)
{
    char    path_name[MAXPATHLEN];
    char    *filename;
    FILE    *outfile;
    char    *time_s;
    time_t  current_time;
    Boolean solid;
    Boolean do_normals;

    if ( name )
	outfile = fopen(name, "w");
    else
    {
	/* Build the default filename. */
	strcat(path_name, "/");
	if ( sced_preferences.scene_path )
	    strcpy(path_name, sced_preferences.scene_path);
	else
#if HAVE_GETCWD
	    getcwd(path_name, MAXPATHLEN);
#else
	    getwd(path_name);
#endif
	strcat(path_name, "/");
	strcat(path_name, base->b_label);
	strcat(path_name, ".wrl");
	outfile = File_Select_Open(path_name, "Export to:", "w", &filename);
	free(filename);
    }
    if ( ! outfile )
	return;

    /* Get some other info. */
    VRML_Get_Base_Info(&solid, &do_normals);

    /* Required VRML header. */
    fprintf(outfile, "#VRML V2.0 utf8\n");

    /* Comments. */
    fprintf(outfile, "WorldInfo {\n");
    fprintf(outfile, "\ntitle \"%s\"\n", base->b_label);
    fprintf(outfile, "\tinfo [\n");
    fprintf(outfile, "\t\t\"File produced by Sced version "VERSION"\"\n");
    time(&current_time);
    time_s = ctime(&current_time);
    time_s[strlen(time_s) - 1] = '\0';
    fprintf(outfile, "\t\t\"%s\"\n", time_s);
    fprintf(outfile, "\t]\n}\n\n");

    fprintf(outfile, "PROTO %s [] {\n", base->b_label);

    wire_hash = Hash_New_Table();
    VRML_Export_Base_Wireframe(outfile, base->b_major_wires[0],
                               &(sced_preferences.default_attributes), FALSE,
			       solid, do_normals);
    Hash_Free(wire_hash);

    fprintf(outfile, "}\n");

    fclose(outfile);
}

