/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/renderman.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: renderman.c,v $
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
**  The RenderMan(r) Interface Procedures and RIB Protocol are:
**  Copyright 1988, 1989, Pixar.
**  All rights reserved.
**  RenderMan(r) is a registered trademark of Pixar.
*/

/*
**  renderman.c: File containing functions for exporting to renderman.
*/


#include <math.h>
#include <ctype.h>
#include <sced.h>
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <bezier.h>
#include <csg.h>
#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif
#include <time.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>

static void     Renderman_Export_Camera(FILE*, Camera*);
static void     Renderman_Export_Lights(FILE*, ScenePtr);
static void     Renderman_Export_Object(FILE*, ObjectInstancePtr);
static void     Renderman_Export_Instances(FILE*, InstanceList);
static void     Renderman_Export_Declarations(FILE*);
static char*    Renderman_Get_Filename(char*);

static Boolean  dialog_done;
static Boolean  dialog_cancel;
static int      light_number;

#define Renderman_Export_Transform(t) \
        fprintf(outfile, "ConcatTransform [ %1.15g %1.15g %1.15g 0.0 "\
                                           "%1.15g %1.15g %1.15g 0.0 "\
                                           "%1.15g %1.15g %1.15g 0.0 "\
                                           "%1.15g %1.15g %1.15g 1.0 ]\n",\
                (t).matrix.x.x, (t).matrix.y.x, (t).matrix.z.x,\
                (t).matrix.x.y, (t).matrix.y.y, (t).matrix.z.y,\
                (t).matrix.x.z, (t).matrix.y.z, (t).matrix.z.z,\
                (t).displacement.x, (t).displacement.y, (t).displacement.z);

int
Export_Renderman(FILE *outfile, char *filename, ScenePtr scene, Boolean preview)
{
    char    *image_filename;
    time_t  current_time;

    time(&current_time);

    light_number = 1;

    if ( ! preview )
        image_filename = Renderman_Get_Filename(filename);
    else
        image_filename = "preview.tif";

    if ( ! image_filename )
        return 1;

    fprintf(outfile, "# File produced by Sced version "VERSION"\n");
    fprintf(outfile, "# %s\n", ctime(&current_time));
    if ( preview )
        fprintf(outfile, "Display \"preview.tif\" \"file\" \"rgba\"\n");
    else
        fprintf(outfile, "Display \"%s\" \"file\" \"rgba\"\n", image_filename);

    Renderman_Export_Camera(outfile, &(scene->camera));

    fprintf(outfile, "WorldBegin\n");

    Renderman_Export_Declarations(outfile);

    Renderman_Export_Lights(outfile, scene);

    Renderman_Export_Instances(outfile, scene->instances);

    return fprintf(outfile, "WorldEnd\n");
}


static void
Renderman_File_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    dialog_cancel = cl ? FALSE : TRUE;
    dialog_done = TRUE;
}


void
Renderman_Action_Func(Widget w, XEvent *e, String *s, Cardinal *num)
{
    Renderman_File_Done_Callback(w, (XtPointer)TRUE, NULL);
}

static Widget
Renderman_Create_Filename_Shell(Widget *dialog)
{
    Widget  shell;
    Arg     args[3];
    int     n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Image Filename");  n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    shell = XtCreatePopupShell("rendermanFileShell", transientShellWidgetClass,
                                main_window.shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNlabel, "Output Image Filename");   n++;
    XtSetArg(args[n], XtNvalue, "");                        n++;
    *dialog = XtCreateManagedWidget("rendermanFileDialog", dialogWidgetClass,
                                    shell, args, n);

    XawDialogAddButton(*dialog, "Done", Renderman_File_Done_Callback,
                       (XtPointer)TRUE);
    XawDialogAddButton(*dialog, "Cancel", Renderman_File_Done_Callback,
                       (XtPointer)FALSE);

    XtOverrideTranslations(XtNameToWidget(*dialog, "value"),
        XtParseTranslationTable(":<Key>Return: Renderman_File_Action()"));

    XtVaSetValues(XtNameToWidget(*dialog, "label"), XtNborderWidth, 0, NULL);

    XtRealizeWidget(shell);

    return shell;
}


static char*
Renderman_Get_Filename(char *default_base)
{
    static Widget   filename_shell = NULL;
    static Widget   filename_dialog = NULL;
    char            *default_name;
    char            *temp;
    XtAppContext    context;
    XEvent          event;

    if ( ! filename_shell )
        filename_shell = Renderman_Create_Filename_Shell(&filename_dialog);

    if ( ! default_base )
        default_name = Strdup("image.tif");
    else
    {
        temp = strrchr(default_base, '/');
        if ( temp )
            default_name = Strdup(temp + 1);
        else
            default_name = Strdup(default_base);
        temp = strrchr(default_name, '.');
        if ( temp )
            *temp = '\0';
        default_name = More(default_name, char, strlen(default_name) + 5);
        strcat(default_name, ".tif");
    }

    XtVaSetValues(filename_dialog, XtNvalue, default_name, NULL);

    free(default_name);

    Position_Shell(filename_shell, FALSE);
    XtPopup(filename_shell, XtGrabExclusive);

    dialog_done = FALSE;
    dialog_cancel = FALSE;
    context = XtWidgetToApplicationContext(main_window.shell);
    while ( ! dialog_done )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }

    XtPopdown(filename_shell);

    if ( dialog_cancel )
        return NULL;

    return XawDialogGetValueString(filename_dialog);
}


static void
Renderman_Export_Camera(FILE *outfile, Camera *cam)
{
    Viewport    cam_view;
    double      min_fov;

    fprintf(outfile, "Format %d %d 1\n", cam->scr_width, cam->scr_height);

    min_fov = min(cam->horiz_fov, cam->vert_fov);
    fprintf(outfile, "Projection \"perspective\" \"fov\" %g\n", min_fov);

    fprintf(outfile, "FrameAspectRatio %g\n",
            (double)cam->scr_width / (double)cam->scr_height);

    Camera_To_Viewport(cam, &cam_view);
    /* Export the camera transformation matrix. */
    fprintf(outfile,
            "Transform [ %1.15g %1.15g %1.15g 0.0 %1.15g %1.15g %1.15g 0.0 "
            "%1.15g %1.15g %1.15g 0.0 %1.15g %1.15g %1.15g 1.0 ]\n",
            cam_view.world_to_view.matrix.x.x,
            cam_view.world_to_view.matrix.y.x,
            cam_view.world_to_view.matrix.z.x,
            cam_view.world_to_view.matrix.x.y,
            cam_view.world_to_view.matrix.y.y,
            cam_view.world_to_view.matrix.z.y,
            cam_view.world_to_view.matrix.x.z,
            cam_view.world_to_view.matrix.y.z,
            cam_view.world_to_view.matrix.z.z,
            cam_view.world_to_view.displacement.x,
            cam_view.world_to_view.displacement.y,
            cam_view.world_to_view.displacement.z + cam_view.eye_distance);

    fprintf(outfile, "\n");
}


static void
Renderman_Export_Light(FILE *outfile, ObjectInstancePtr obj,
                       AttributePtr def_attribs)
{
    Vector  color;
    double  intensity, temp_d;

    if ( ! def_attribs || obj->o_attribs->defined )
        def_attribs = obj->o_attribs;

    intensity = max(def_attribs->intensity.red,
                    def_attribs->intensity.green);
    intensity = max(intensity, def_attribs->intensity.blue);
    color.x = def_attribs->intensity.red;
    color.y = def_attribs->intensity.green;
    color.z = def_attribs->intensity.blue;
    if ( ! IsZero(intensity) )
    {
        temp_d = 1 / intensity;
        VScalarMul(color, temp_d, color);
    }

    switch ( obj->o_parent->b_class )
    {
        case light_obj:
            fprintf(outfile,
                    "LightSource \"pointlight\" %d \"lightcolor\" [ %g %g %g ]"
                    " \"intensity\" %f \"from\" [ %1.15g %1.15g %1.15g ]\n",
                    light_number, color.x, color.y, color.z, intensity,
                    obj->o_transform.displacement.x,
                    obj->o_transform.displacement.y,
                    obj->o_transform.displacement.z);
            break;

        case arealight_obj:
            /* Begin the area light source definition. */
            fprintf(outfile, "AttributeBegin\n");
            fprintf(outfile,
                "AreaLightSource \"arealight\" %d"
                " \"lightcolor\" [ %g %g %g ] \"intensity\" %f\n",
                light_number, color.x, color.y, color.z, intensity);
            /* Export the actual arealight as a polygon. */
            Renderman_Export_Transform(obj->o_transform);
            fprintf(outfile, "Polygon \"P\" [ 1 1 0 -1 1 0 -1 -1 0 1 -1 0 ]\n");
            fprintf(outfile, "AttributeEnd\n");
            break;

        case spotlight_obj:
            {
                double  radius;
                double  cos_rad;
                Vector  vect1, vect2;
                Vector  direction;

                /* Calculate the radius. */
                VSub(obj->o_world_verts[0], obj->o_world_verts[9], vect1);
                VSub(obj->o_world_verts[8], obj->o_world_verts[9], vect2);
                VUnit(vect1, radius, vect1);
                VUnit(vect2, radius, vect2);
                cos_rad = VDot(vect1, vect2);
                radius = acos(cos_rad);

                if ( def_attribs->invert )
                {
                    /* Invert it. */
                    /* vect2 still points toward direction. */
                    VScalarMul(vect2, -1.0, vect2);
                    VAdd(vect2, obj->o_world_verts[9], direction);

                    radius += M_PI_2;
                }
                else
                    direction = obj->o_world_verts[8];

                fprintf(outfile,
                        "LightSource \"spotlight\" %d"
                        " \"lightcolor\" [ %g %g %g ] \"intensity\" %g"
                        " \"from\" [ %1.15g %1.15g %1.15g ]"
                        " \"to\" [ %1.15g %1.15g %1.15g ]"
                        " \"coneangle\" %g"
                        " \"conedeltaangle\" %g"
                        " \"beamdistribution\" %g\n",
                        light_number,
                        color.x, color.y, color.z, intensity,
                        obj->o_transform.displacement.x,
                        obj->o_transform.displacement.y,
                        obj->o_transform.displacement.z,
                        direction.x, direction.y, direction.z,
                        radius,
                        radius * (def_attribs->radius - 1.0),
                        def_attribs->tightness);
            }
            break;

        case dirlight_obj:
            fprintf(outfile,
                    "LightSource \"distantlight\" %d"
                    " \"lightcolor\" [ %g %g %g ] \"intensity\" %g"
                    " \"from\" [ %1.15g %1.15g %1.15g ]"
                    " \"to\" [ %1.15g %1.15g %1.15g ]",
                    light_number,
                    color.x, color.y, color.z, intensity,
                    obj->o_world_verts[5].x,
                    obj->o_world_verts[5].y,
                    obj->o_world_verts[5].z,
                    obj->o_world_verts[0].x,
                    obj->o_world_verts[0].y,
                    obj->o_world_verts[0].z);
            break;

        default:;
    }

    light_number++;

    fprintf(outfile, "\n");
}


static void
Radiance_Export_Light_Insts(FILE *outfile, InstanceList inst,
                            AttributePtr def_attribs)
{
    /* Go through all the instances looking for lights. */
    for ( ; inst ; inst = inst->next )
    {
        if ( Obj_Is_Construction(inst->the_instance) )
            continue;
        else if ( Obj_Is_Light(inst->the_instance) )
            Renderman_Export_Light(outfile, inst->the_instance, NULL);
        else if ( Obj_Is_Aggregate(inst->the_instance) )
        {
            fprintf(outfile, "AttributeBegin\n");
            Renderman_Export_Transform(inst->the_instance->o_transform);
            if ( inst->the_instance->o_attribs->defined || ! def_attribs )
	      Radiance_Export_Light_Insts(outfile,
	       ((AggregatePtr)inst->the_instance->o_parent->b_struct)->children,
               inst->the_instance->o_attribs);
            else
              Radiance_Export_Light_Insts(outfile,
	       ((AggregatePtr)inst->the_instance->o_parent->b_struct)->children,
	       def_attribs);
            fprintf(outfile, "AttributeEnd\n");
        }
    }
}

static void
Renderman_Export_Lights(FILE *outfile, ScenePtr scene)
{
    int	i;

    /* The ambient light source. */
    fprintf(outfile,
            "LightSource \"ambientlight\" %d \"lightcolor\" [ %f %f %f ]\n",
            light_number++,
            scene->ambient.red, scene->ambient.green, scene->ambient.blue);

    if ( scene->light )
        Renderman_Export_Light(outfile, scene->light, NULL);

    Radiance_Export_Light_Insts(outfile, scene->instances, NULL);

    for ( i = 2 ; i < light_number ; i++ )
	fprintf(outfile, "Illuminate %d 1\n", i);
}



static void
Renderman_Export_Attributes(FILE *outfile, AttributePtr attribs, char *name)
{
    fprintf(outfile, "Attribute \"identifier\" \"name\" [ \"%s\" ]\n", name);

    if ( ! attribs->defined )
        return;

    fprintf(outfile, "Color %g %g %g\n",
            attribs->color.red, attribs->color.green, attribs->color.blue);

    if ( attribs->use_extension && attribs->extension[Renderman] )
    {
        fprintf(outfile, "%s\n", attribs->extension[Renderman]);
        return;
    }

    /* Must be simple attributes. We'll export as plastic. */
    fprintf(outfile,
            "Surface \"plastic\" \"Ka\" 1.0 \"Kd\" %g \"Ks\" %g"
            " \"roughness\" %g\n",
            attribs->diff_coef, attribs->spec_coef,
            ( attribs->spec_power > 0.0 ? 1 / attribs->spec_power : 0.0 ));
}

static void
Renderman_Export_CSG_Object(FILE *outfile, CSGNodePtr obj)
{
    if ( ! obj )
        return;

    switch ( obj->csg_op )
    {
        case csg_union_op:
            fprintf(outfile, "SolidBegin \"union\"\n");
            Renderman_Export_CSG_Object(outfile, obj->csg_left_child);
            Renderman_Export_CSG_Object(outfile, obj->csg_right_child);
            fprintf(outfile, "SolidEnd\n");
            break;

        case csg_intersection_op:
            fprintf(outfile, "SolidBegin \"intersection\"\n");
            Renderman_Export_CSG_Object(outfile, obj->csg_left_child);
            Renderman_Export_CSG_Object(outfile, obj->csg_right_child);
            fprintf(outfile, "SolidEnd\n");
            break;

        case csg_difference_op:
            fprintf(outfile, "SolidBegin \"difference\"\n");
            Renderman_Export_CSG_Object(outfile, obj->csg_left_child);
            Renderman_Export_CSG_Object(outfile, obj->csg_right_child);
            fprintf(outfile, "SolidEnd\n");
            break;

        case csg_leaf_op:
            fprintf(outfile, "SolidBegin \"primitive\"\n");
            Renderman_Export_Object(outfile, obj->csg_instance);
            fprintf(outfile, "SolidEnd\n");
            break;
    }
}


static void
Renderman_Export_Wireframe_Object(FILE *outfile, WireframePtr wireframe,
                                  char *obj_name)
{
    char    *name;
    int     i, j;

    name = New(char, strlen(obj_name) + 8);

    for ( i = 0 ; i < wireframe->num_faces ; i++ )
    {
        if ( wireframe->faces[i].face_attribs &&
             wireframe->faces[i].face_attribs->defined )
        {
            fprintf(outfile, "AttributeBegin\n");
            sprintf(name, "%s.%0d", obj_name, i);
            Renderman_Export_Attributes(outfile,
                                        wireframe->faces[i].face_attribs, name);
        }

        fprintf(outfile, "Polygon \"P\" [ ");
        for ( j = 0 ; j < wireframe->faces[i].num_vertices ; j++ )
            fprintf(outfile, "%1.15g %1.15g %1.15g ",
                    wireframe->vertices[wireframe->faces[i].vertices[j]].x,
                    wireframe->vertices[wireframe->faces[i].vertices[j]].y,
                    wireframe->vertices[wireframe->faces[i].vertices[j]].z);
	fprintf(outfile, "] \"N\" [ ");
	for ( j = 0 ; j < wireframe->faces[i].num_vertices ; j++ )
	    fprintf(outfile, "%1.15g %1.15g %1.15g ",
                    wireframe->normals[wireframe->faces[i].normals[j]].x,
                    wireframe->normals[wireframe->faces[i].normals[j]].y,
                    wireframe->normals[wireframe->faces[i].normals[j]].z);
	fprintf(outfile, "]\n");

        if ( wireframe->faces[i].face_attribs &&
             wireframe->faces[i].face_attribs->defined )
            fprintf(outfile, "AttributeEnd\n");
    }

    free(name);
}


static void
Renderman_Export_Object(FILE *outfile, ObjectInstancePtr obj)
{
    int i;

    fprintf(outfile, "# %s\n", obj->o_label);

    /* Save attributes. */
    fprintf(outfile, "AttributeBegin\n");

    if ( ! obj->o_attribs->use_obj_trans )
        Renderman_Export_Attributes(outfile, obj->o_attribs, obj->o_label);

    Renderman_Export_Transform(obj->o_transform);

    if ( obj->o_attribs->use_obj_trans )
        Renderman_Export_Attributes(outfile, obj->o_attribs, obj->o_label);

    if ( Obj_Is_Aliased(obj, Renderman) )
    {
        fprintf(outfile, "%s", (char*)obj->o_aliases[Renderman]);
        fprintf(outfile, "AttributeEnd\n\n");
        return;
    }

    switch ( obj->o_parent->b_class )
    {
        case sphere_obj:
            fprintf(outfile, "Sphere 1 -1 1 360\n");
            break;

        case cube_obj:
            fprintf(outfile,
                    "PointsPolygons [ 4 4 4 4 4 4 ] "
                    "[ 0 3 2 1 4 5 6 7 0 4 7 3 1 2 6 5 0 1 5 4 2 3 7 6 ] "
                    "\"P\" [ 1 1 1 -1 1 1 -1 -1 1 1 -1 1 1 1 -1 -1 1 -1 "
                    "-1 -1 -1 1 -1 -1 ]\n");
            break;

        case cylinder_obj:
            fprintf(outfile, "Cylinder 1 -1 1 360\n");
            if ( ! ( obj->o_attribs->defined &&
                     obj->o_attribs->use_extension &&
                     obj->o_attribs->open ) )
            {
                fprintf(outfile, "Disk 1 1 360\n");
                fprintf(outfile, "Disk -1 1 360\n");
            }
            break;

        case cone_obj:
            fprintf(outfile, "Translate 0 0 -1\n");
            fprintf(outfile, "Cone 2 1 360\n");
            if ( ! ( obj->o_attribs->defined &&
                     obj->o_attribs->use_extension &&
                     obj->o_attribs->open ) )
                fprintf(outfile, "Disk 0 1 360\n");
            break;

        case torus_obj:
            fprintf(outfile, "Torus %1.15g 1.0 0.0 360.0 360.0\n",
                    ((TorusPtr)obj->o_hook)->major_radius);
            break;

        case plane_obj:
            fprintf(outfile,
                    "Polygon \"P\" [ 10 10 0 -10 10 0 -10 -10 0 10 -10 0 ]\n");
            break;

        case triangle_obj:
            fprintf(outfile, "Polygon \"P\" [ ");
            fprintf(outfile, "%1.15g %1.15g %1.15g ",
                    control_part(obj)->control_verts[0].x,
                    control_part(obj)->control_verts[0].y,
                    control_part(obj)->control_verts[0].z);
            fprintf(outfile, "%1.15g %1.15g %1.15g ",
                    control_part(obj)->control_verts[1].x,
                    control_part(obj)->control_verts[1].y,
                    control_part(obj)->control_verts[1].z);
            fprintf(outfile, "%1.15g %1.15g %1.15g ",
                    control_part(obj)->control_verts[2].x,
                    control_part(obj)->control_verts[2].y,
                    control_part(obj)->control_verts[2].z);
            fprintf(outfile, "]\n");
            break;

        case bezier_obj:
            fprintf(outfile, "Patch \"bicubic\" \"P\" [ ");
            for ( i = 0 ; i < 16 ; i++ )
                fprintf(outfile, "%1.15g %1.15g %1.15g ",
                        control_part(obj)->control_verts[bezier_map[i]].x,
                        control_part(obj)->control_verts[bezier_map[i]].y,
                        control_part(obj)->control_verts[bezier_map[i]].z);
            fprintf(outfile, "]\n");
            break;

        case csg_obj: {
            CSGNodePtr  new_tree =
                        CSG_Contract_Tree((CSGNodePtr)obj->o_parent->b_struct);
            Renderman_Export_CSG_Object(outfile, new_tree);
            CSG_Delete_Tree(new_tree, FALSE);
            } break;

        case wireframe_obj:
            Renderman_Export_Wireframe_Object(outfile,
                    obj->o_parent->b_major_wires[0], obj->o_parent->b_label);
            break;

        case aggregate_obj:
            Renderman_Export_Instances(outfile,
			   ((AggregatePtr)obj->o_parent->b_struct)->children);
            break;

        case light_obj:
        case spotlight_obj:
        case arealight_obj:
        case dirlight_obj:
        case last_obj:;
    }

    /* Restore attributes. */
    fprintf(outfile, "AttributeEnd\n\n");
}


static void
Renderman_Export_Instances(FILE *outfile, InstanceList instances)
{
    InstanceList    inst;

    for ( inst = instances ; inst ; inst = inst->next )
        if ( ! Obj_Is_Light(inst->the_instance) &&
             ! Obj_Is_Construction(inst->the_instance) )
            Renderman_Export_Object(outfile, inst->the_instance);
}


static void
Renderman_Export_Declarations(FILE *outfile)
{
    if ( declarations[Renderman] )
        fprintf(outfile, "%s\n", declarations[Renderman]);
    return;
}


