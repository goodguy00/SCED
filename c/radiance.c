/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/radiance.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: radiance.c,v $
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
**  radiance.c: Radiance specific functions - mostly export.
*/

#include <math.h>
#include <sced.h>
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <csg.h>
#include <hash.h>
#include <time.h> 
#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

#define LOW 1
#define MEDIUM 2
#define HIGH 3

#define INTERIOR 1
#define EXTERIOR 2

#define RadVPrint(vect) fprintf(outfile, "%1.15g %1.15g %1.15g", \
                                (vect).x, (vect).y, (vect).z)

/* Translations to get exactly one toggle always active. */
#define TOGGLE_TRANSLATIONS "<EnterWindow>: highlight(Always)\n"\
                            "<LeaveWindow>: unhighlight()\n"\
                            "<Btn1Down>,<Btn1Up>: set() notify()\n"

static int  Radiance_Export_Main(FILE*, FILE*, char*, ScenePtr);
static int  Export_Rad_File(FILE*, char*, ScenePtr);
static int  Export_Scene_File(FILE*, ScenePtr);
static int  Export_Light(FILE*, ObjectInstancePtr, TransformPtr);
static int  Export_Instances(FILE*, InstanceList);
static void Radiance_Create_Shell();
static void Radiance_Set_Zone(InstanceList);

static Widget   radiance_export_shell = NULL;
static Widget   zone_radio;
static Widget   zone_text;
static Widget   exposure_text;
static Widget   var_radio;
static Widget   det_radio;
static Widget   qual_radio;
static Widget   indirect_text;

static char *rad_basename;

static Boolean  export, cancel;

static Boolean  did_csg_wire = FALSE;

static HashTable    base_hash;

static Transformation   identity = { { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } },
                                     { 0, 0, 0 } };

int
Export_Radiance(FILE *rad_file, char *base_name, ScenePtr scene,
                Boolean preview)
{
    FILE            *scene_file;
    char            *scene_name;
    XtAppContext    context;
    XEvent          event;
    char            *temp_ch;
    int             result;

    /* Work out what the base filename is. */
    if ( preview )
        rad_basename = Strdup(base_name);
    else
    {
        rad_basename = Strdup(base_name);
        temp_ch = strrchr(rad_basename, '.');
        if ( temp_ch && ! strcmp(temp_ch, ".rif") )
            *temp_ch = '\0';
    }

    /* Create supplementary information dialog. */
    if ( ! radiance_export_shell )
        Radiance_Create_Shell();

    export = cancel = FALSE;

    /* Check if it's a CSG Preview. If so, we need the wireframe. */
    if ( preview && ! scene->instances->the_instance->o_wireframe )
    {
        WireframePtr    main_wireframe;

        main_wireframe = CSG_Generate_Wireframe(
                (CSGNodePtr)scene->instances->the_instance->o_parent->b_struct,
                0, FALSE);
        scene->instances->the_instance->o_wireframe =
            Wireframe_Simplify(main_wireframe, FALSE);
        Wireframe_Destroy(main_wireframe);
        did_csg_wire = TRUE;
    }

    Radiance_Set_Zone(scene->instances);

    Position_Shell(radiance_export_shell, FALSE);
    XtPopup(radiance_export_shell, XtGrabExclusive);

    /* Need my own event loop, because the function must not return until
    ** the file has been exported.
    */
    context = XtWidgetToApplicationContext(main_window.shell);
    while ( ! ( export || cancel ) )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }

    if ( export )
    {
        /* Create the scene file. */
        scene_name = New(char, strlen(rad_basename) + 5);
        strcpy(scene_name, rad_basename);
        strcat(scene_name, ".rad");
        if ( ! ( scene_file = fopen(scene_name, "w") ) )
            return 0;
        if ( preview )
        {
            char    *temp_name;

            Save_Temp_Filename(scene_name);

            /* Also need to store the octree and ambient filenames for deletion.
            */
            temp_name = New(char, strlen(rad_basename) + 5);
            strcpy(temp_name, rad_basename);
            strcat(temp_name, ".oct");
            Save_Temp_Filename(temp_name);
            strcpy(temp_name, rad_basename);
            strcat(temp_name, ".amb");
            Save_Temp_Filename(temp_name);
            free(temp_name);
        }

        result = Radiance_Export_Main(rad_file, scene_file, scene_name, scene);
        free(scene_name);
        free(rad_basename);
        fclose(scene_file);
        return result;
    }

    free(rad_basename);
    return 1;
}


static int
Radiance_Export_Main(FILE *rad_file, FILE *scene_file, char *scene_name,
                     ScenePtr scene)
{
    if ( ! Export_Rad_File(rad_file, scene_name, scene) ||
         ! Export_Scene_File(scene_file, scene) )
    {
        Popup_Error("Write Failed!", main_window.shell, "Error");
        return 0;
    }
    
    return 1;
}


static int
Export_Rad_File(FILE *outfile, char *scene_name, ScenePtr scene)
{
    time_t      current_time;
    XtPointer   temp_p;
    char        *temp_s;
    float       temp_f;
    int         temp_i;
    Vector      temp_v;
    char        *file_base;

#define Print_Level(level)  if ( level == LOW ) \
                                fprintf(outfile, "LOW\n"); \
                            else if ( level == MEDIUM ) \
                                fprintf(outfile, "MEDIUM\n"); \
                            else \
                                fprintf(outfile, "HIGH\n")

    time(&current_time);

    fprintf(outfile, "# File generated by Sced\n# %s\n", ctime(&current_time));
    fprintf(outfile, "\n");

    /* Get assorted support information from the dialog. */

    temp_p = XawToggleGetCurrent(zone_radio);
    XtVaGetValues(zone_text, XtNstring, &temp_s, NULL);
    fprintf(outfile, "ZONE = %s %s\n", ( (int)temp_p == INTERIOR ? "I" : "E" ),
            temp_s);

    XtVaGetValues(exposure_text, XtNstring, &temp_s, NULL); 
    if ( sscanf(temp_s, "%g", &temp_f) == 1 )
        fprintf(outfile, "EXPOSURE = %1g\n", temp_f);

    temp_p = XawToggleGetCurrent(var_radio);
    fprintf(outfile, "VARIABILITY = ");
    Print_Level((int)temp_p);

    temp_p = XawToggleGetCurrent(det_radio);
    fprintf(outfile, "DETAIL = ");
    Print_Level((int)temp_p);

    temp_p = XawToggleGetCurrent(qual_radio);
    fprintf(outfile, "QUALITY = ");
    Print_Level((int)temp_p);

    XtVaGetValues(indirect_text, XtNstring, &temp_s, NULL); 
    if ( sscanf(temp_s, "%d", &temp_i) == 1 )
        fprintf(outfile, "INDIRECT = %0d\n", temp_i);

    fprintf(outfile, "\n");

    /* File information. */
    if ( ! ( file_base = strrchr(rad_basename, '/') ) )
        file_base = rad_basename;
    else
        file_base++;
    fprintf(outfile, "OCTREE = %s.oct\n", file_base);
    fprintf(outfile, "PICTURE = %s\n", file_base);
    fprintf(outfile, "AMBFILE = %s.amb\n", file_base);
    if ( ! ( file_base =  strrchr(scene_name, '/') ) )
        file_base = scene_name;
    else
        file_base++;
    fprintf(outfile, "\nscene = %s\n\n", file_base);

    /* Image size. */
    fprintf(outfile, "RESOLUTION %d %d\n\n", scene->camera.scr_width,
            scene->camera.scr_height);

    /* View information. */
    fprintf(outfile, "view sced -vtv -vp %1.3g %1.3g %1.3g",
            scene->camera.location.x, scene->camera.location.y,
            scene->camera.location.z);
    VSub(scene->camera.look_at, scene->camera.location, temp_v);
    fprintf(outfile, " -vd %1.3g %1.3g %1.3g", temp_v.x, temp_v.y, temp_v.z);
    fprintf(outfile, " -vu %1.3g %1.3g %1.3g", scene->camera.look_up.x,
            scene->camera.look_up.y, scene->camera.look_up.z);
    fprintf(outfile, " -vh %1g -vv %1g\n", scene->camera.horiz_fov,
            scene->camera.vert_fov);

    return fprintf(outfile, "\n");
}


static int
Export_Scene_File(FILE *outfile, ScenePtr scene)
{
    time_t      current_time;

    time(&current_time);

    base_hash = Hash_New_Table();

    fprintf(outfile, "# File generated by Sced\n# %s\n", ctime(&current_time));
    fprintf(outfile, "\n");

    /* Export the declarations as one chunk. */
    if ( declarations[Radiance] )
        fprintf(outfile, "#\n# Declarations\n#\n%s\n\n",declarations[Radiance]);

    if ( scene->light )
        Export_Light(outfile, scene->light, &identity);

    Export_Instances(outfile, scene->instances);

    Hash_Free(base_hash);

    return fprintf(outfile, "\n");
}


static int
Export_Light(FILE *outfile, ObjectInstancePtr light, TransformPtr add_trans)
{
    double  radius;
    double  cos_rad;
    Vector  vect1, vect2;
    Vector  v0, v8, v9;
    Vector  direction;
    int     i;

    switch ( light->o_parent->b_class )
    {
        case dirlight_obj:
            fprintf(stderr,
                    "Sced: Directional light not supported by Radiance\n");
            fprintf(stderr, "Sced: Exporting as point source instead.\n");
        case light_obj:
            fprintf(outfile, "void light %s_mat\n0\n0\n", light->o_label);
            fprintf(outfile, "3 %1.5g %1.5g %1.5g\n",
                    light->o_attribs->intensity.red,
                    light->o_attribs->intensity.green,
                    light->o_attribs->intensity.blue);

            fprintf(outfile, "%s_mat sphere %s\n0\n0\n4 ", light->o_label,
                    light->o_label);
	    Transform_Vector(*add_trans, light->o_transform.displacement,vect1);
            RadVPrint(vect1);
            fprintf(outfile, " 0.2\n");

            break;

        case spotlight_obj:
	    Transform_Vector(*add_trans, light->o_world_verts[0], v0);
	    Transform_Vector(*add_trans, light->o_world_verts[8], v8);
	    Transform_Vector(*add_trans, light->o_world_verts[9], v9);

            /* Calculate the radius. */
            VSub(v0, v9, vect1);
            VSub(v8, v9, vect2);
            VUnit(vect1, radius, vect1);
            VUnit(vect2, radius, vect2);
            cos_rad = VDot(vect1, vect2);
            radius = acos(cos_rad) * 180 / M_PI;

            if ( light->o_attribs->invert )
            {
                /* Invert it. */
                /* vect2 still points toward direction. */
                VScalarMul(vect2, -1.0, vect2);
                VAdd(vect2, v9, direction);

                radius += 90.0;
            }
            else
                direction = v8;

            fprintf(outfile, "void spotlight %s_mat\n0\n0\n", light->o_label);
            fprintf(outfile, "7 %1.5g %1.5g %1.5g %1.5g ",
                    light->o_attribs->intensity.red,
                    light->o_attribs->intensity.green,
                    light->o_attribs->intensity.blue, radius);
            RadVPrint(direction);
            fprintf(outfile, "\n");

            fprintf(outfile, "%s_mat sphere %s\n0\n0\n4 ", light->o_label,
                    light->o_label);
            RadVPrint(light->o_transform.displacement);
            fprintf(outfile, " 0.2\n");

            break;

        case arealight_obj:
            fprintf(outfile, "void light %s_mat\n0\n0\n", light->o_label);
            fprintf(outfile, "3 %1.5g %1.5g %1.5g\n",
                    light->o_attribs->intensity.red,
                    light->o_attribs->intensity.green,
                    light->o_attribs->intensity.blue);
            fprintf(outfile, "%s_mat polygon %s\n0\n0\n12\n", light->o_label,
                    light->o_label);
            for ( i = 0 ; i < 4 ; i++ )
            {
                fprintf(outfile, "\t");
		Transform_Vector(*add_trans, light->o_world_verts[i], vect1);
                RadVPrint(vect1);
                fprintf(outfile, "\n");
            }
            break;

        default:;
    }

    return fprintf(outfile, "\n");
}


static void
Determine_Face_Attributes(AttributePtr attribs, char** mod_name,
                         char *def_name, Boolean *invert)
{
    if ( attribs && attribs->defined )
    {
        if ( attribs->use_extension && attribs->extension[Radiance])
        {
            free(*mod_name);
            *mod_name = Strdup(attribs->extension[Radiance]);
            *invert = attribs->use_obj_trans;
        }
        else
            sprintf(*mod_name, "material_%ld", (unsigned long)attribs);
    }
    else
    {
        free(*mod_name);
        *mod_name = Strdup(def_name);
    }
}


static int
Export_Polyhedral_Object(FILE *outfile, WireframePtr wire, char *name,
                         char *default_mod, Boolean invert)
{
    char    *material = New(char, 48);
    int     i, j;

#define Export_Vert { \
                        fprintf(outfile, "\t"); \
                        RadVPrint(wire->vertices[wire->faces[i].vertices[j]]); \
                        fprintf(outfile, "\n"); \
                    }

    /* Export the polygons. */
    for ( i = 0 ; i < wire->num_faces ; i++ )
    {
        if ( wire->faces[i].num_vertices < 3 )
            continue;

        Determine_Face_Attributes(wire->faces[i].face_attribs,
                                  &material, default_mod, &invert);

        fprintf(outfile, "%s polygon %s.%d\n0\n0\n%d\n", material, name, i,
                3 * wire->faces[i].num_vertices);

        if ( invert )
            for ( j = 0 ; j < wire->faces[i].num_vertices ; j++ )
                Export_Vert
        else
            for ( j = wire->faces[i].num_vertices - 1 ; j >= 0 ; j-- )
                Export_Vert
    }

    free(material);

    return 1;

#undef Export_Vert
}


static int
Export_Simple_Material(FILE *outfile, AttributePtr attribs, char *name)
{
    fprintf(outfile, "void plastic %s\n0\n0\n", name);
    return fprintf(outfile, "5 %1.5g %1.5g %1.5g %1.5g 0\n",
                attribs->color.red * attribs->diff_coef,
                attribs->color.green * attribs->diff_coef,
                attribs->color.blue * attribs->diff_coef,
                attribs->spec_coef);
}


static int
Export_Attributes(FILE *outfile, AttributePtr attribs,
                  Boolean *invert, Boolean *open, char **name)
{
    if ( attribs && attribs->defined )
    {
        if ( attribs->use_extension && attribs->extension[Radiance] )
        {
            *invert = attribs->use_obj_trans;
            *open = attribs->open;
            free(*name);
            *name = Strdup(attribs->extension[Radiance]);
        }
        else
        {
            /* Export the material. */
            *invert = FALSE;
            *open = FALSE;
            sprintf(*name, "material_%ld", (unsigned long)attribs);
            Export_Simple_Material(outfile, attribs, *name);
        }
    }
    else
    {
        strcpy(*name, "void");
        *invert = FALSE;
        *open = FALSE;
    }

    return 1;
}


static int
Export_Aliased_Object(FILE *outfile, ObjectInstancePtr obj)
{
    char    *mod_name = New(char, 48);
    Boolean dummy;
    char    *alias = (char*)obj->o_aliases[Radiance];
    int     index;

    fprintf(outfile, "#\n# %s\n#\n", obj->o_label);

    Export_Attributes(outfile, (AttributePtr)obj->o_attribs,
                      &dummy, &dummy, &mod_name);

    index = 0;
    while ( alias[index] != '\0' )
    {
        if ( alias[index] == '(' && alias[index + 1] == '*' )
        {
            if ( ! strncmp(alias + index, "(*attributes*)", 14) )
            {
                fprintf(outfile, "%s", mod_name);
                index += 14;
            }
            else
                fputc((int)alias[index++], outfile);
        }
        else
            fputc((int)alias[index++], outfile);
    }

    free(mod_name);
    return 1;
}


static int
Export_Object(FILE *outfile, ObjectInstancePtr obj)
{
    char            *mod_name = New(char, 48);
    Boolean         invert;
    Boolean         open;
    WireframePtr    wireframe = Object_To_Wireframe(obj, TRUE);

    fprintf(outfile, "#\n# %s\n#\n", obj->o_label);

    Export_Attributes(outfile, (AttributePtr)obj->o_attribs,
                      &invert, &open, &mod_name);

    if ( open )
    {
        if ( obj->o_parent->b_class == cylinder_obj )
        {
            /* Take off the endcaps. */
            wireframe->num_faces -= 2;
        }
        else if ( obj->o_parent->b_class == cone_obj )
        {
            /* Take off the base. */
            wireframe->num_faces -= 1;
        }
    }

    Export_Polyhedral_Object(outfile, wireframe, obj->o_label, mod_name,
                             invert);

    Wireframe_Destroy(wireframe);

    free(mod_name);

    return fprintf(outfile, "\n");
}


static int
Export_Plane(FILE *outfile, ObjectInstancePtr plane)
{
    char            *mod_name = New(char, 48);
    Boolean         invert;
    Boolean         open;
    WireframePtr    wireframe = New(Wireframe, 1);
    int             i;
    Vector          temp_v;

    fprintf(outfile, "#\n# %s\n#\n", plane->o_label);

    Export_Attributes(outfile, (AttributePtr)plane->o_attribs,
                      &invert, &open, &mod_name);

    /* Build a dummy wireframe. */
    wireframe->num_vertices = 4;
    wireframe->num_real_verts = 4;
    wireframe->vertices = New(Vector, 4);
    VNew(10, 10, 0, wireframe->vertices[0]);
    VNew(-10, 10, 0, wireframe->vertices[1]);
    VNew(-10, -10, 0, wireframe->vertices[2]);
    VNew(10, -10, 0, wireframe->vertices[3]);
    wireframe->num_normals = 1;
    wireframe->normals = New(Vector, 1);
    VNew(0, 0, 1, wireframe->normals[0]);
    wireframe->num_faces = 1;
    wireframe->faces = New(Face, 1);
    wireframe->faces[0].num_vertices = 4;
    wireframe->faces[0].vertices = New(int, 4);
    wireframe->faces[0].normals = New(int, 4);
    for ( i = 0 ; i < 4 ; i++ )
    {
        wireframe->faces[0].vertices[i] = i;
        wireframe->faces[0].normals[i] = 0;
    }
    wireframe->faces[0].face_attribs = NULL;
    wireframe->num_attribs = 0;
    wireframe->attribs = NULL;

    for ( i = 0 ; i < 4 ; i++ )
    {
        MVMul(plane->o_transform.matrix, wireframe->vertices[i], temp_v);
        VAdd(temp_v, plane->o_transform.displacement, wireframe->vertices[i]);
    }

    Export_Polyhedral_Object(outfile, wireframe, plane->o_label, mod_name,
                             invert);

    free(mod_name);

    Wireframe_Destroy(wireframe);

    return fprintf(outfile, "\n");
}


static int
Export_Sphere(FILE *outfile, ObjectInstancePtr sphere)
{
    Boolean uneven = FALSE;
    double  rad_sq, first_rad_sq;
    Vector  center;
    Vector  temp_v;
    int     i;

    /* Radiance only allows for untransformed spheres.
    ** To get around this, we sample to radius at several key points.
    ** If all these radii are equal, it's a sphere and is exported as
    ** such. Otherwise, it's exported as a set of polygons.
    */

    /* Sample radius of vertices.
    */
    center = sphere->o_world_verts[sphere->o_num_vertices - 1];
    VSub(sphere->o_world_verts[0], center, temp_v);
    first_rad_sq = VDot(temp_v, temp_v);
    for ( i = 1 ; i < sphere->o_num_real && ! uneven ; i++ )
    {
        VSub(sphere->o_world_verts[i], center, temp_v);
        rad_sq = VDot(temp_v, temp_v);
        if ( ! DEqual(rad_sq, first_rad_sq) )
            uneven = TRUE;
    }

    /* uneven is FALSE if it is a sphere (ie constant radius surface). */
    if ( uneven )
        Export_Object(outfile, sphere);
    else
    {
        char            *mod_name = New(char, 48);
        Boolean         invert;
        Boolean         dummy;

        fprintf(outfile, "#\n# %s\n#\n", sphere->o_label);

        Export_Attributes(outfile, (AttributePtr)sphere->o_attribs,
                          &invert, &dummy, &mod_name);

        if ( invert )
            fprintf(outfile, "%s bubble %s\n0\n0\n4 ", mod_name,
                    sphere->o_label);
        else
            fprintf(outfile, "%s sphere %s\n0\n0\n4 ", mod_name,
                    sphere->o_label);
        RadVPrint(sphere->o_transform.displacement);
        fprintf(outfile, " %1.15g\n", sqrt(rad_sq));

        free(mod_name);
    }

    return fprintf(outfile,"\n");
}


static int
Export_Cylinder(FILE *outfile, ObjectInstancePtr cyl)
{
    Boolean uneven = FALSE;
    double  rad_sq, first_rad_sq;
    Vector  temp_v, axis;
    Vector  center;
    double  temp_d;
    int     half_num, i;

    /* Radiance only allows for cylinders with endcaps parallel to the axis
    ** and constant endcap radius.
    */

    /* Sample radius of vertices.
    */
    half_num = cyl->o_num_real / 2;
    center = cyl->o_world_verts[cyl->o_num_real];
    VSub(cyl->o_world_verts[0], center, temp_v);
    first_rad_sq = VDot(temp_v, temp_v);
    for ( i = 1 ; i < half_num && ! uneven ; i++ )
    {
        VSub(cyl->o_world_verts[i], center, temp_v);
        rad_sq = VDot(temp_v, temp_v);
        if ( ! DEqual(rad_sq, first_rad_sq) )
            uneven = TRUE;
    }
    center = cyl->o_world_verts[cyl->o_num_real + 1];
    for ( ; i < cyl->o_num_real && ! uneven ; i++ )
    {
        VSub(cyl->o_world_verts[i], center, temp_v);
        rad_sq = VDot(temp_v, temp_v);
        if ( ! DEqual(rad_sq, first_rad_sq) )
            uneven = TRUE;
    }

    /* Also test that the endcap is perpendicular to the axis.  */
    /* Must calculate axis. */
    VSub(cyl->o_world_verts[cyl->o_num_real],
         cyl->o_world_verts[cyl->o_num_real + 1], axis);
    VSub(cyl->o_world_verts[0],
         cyl->o_world_verts[cyl->o_num_real], temp_v);
    temp_d = VDot(axis, temp_v);
    if ( ! IsZero(temp_d) )
        uneven = TRUE;

    /* uneven is FALSE if it is a cone (ie constant radius surface). */
    if ( uneven )
        Export_Object(outfile, cyl);
    else
    {
        char            *mod_name = New(char, 48);
        Boolean         invert;
        Boolean         open;

        fprintf(outfile, "#\n# %s\n#\n", cyl->o_label);

        Export_Attributes(outfile, (AttributePtr)cyl->o_attribs,
                          &invert, &open, &mod_name);

        if ( invert )
            fprintf(outfile, "%s tube %s\n0\n0\n7\n\t ", mod_name,
                    cyl->o_label);
        else
            fprintf(outfile, "%s cylinder %s\n0\n0\n7\n\t", mod_name,
                    cyl->o_label);
        RadVPrint(cyl->o_world_verts[cyl->o_num_real]);
        fprintf(outfile, "\n\t");
        RadVPrint(cyl->o_world_verts[cyl->o_num_real + 1]);
        fprintf(outfile, "\n");
        fprintf(outfile, "\t%1.15g\n", sqrt(first_rad_sq));

        if ( ! open )
        {
            if ( invert )
                VScalarMul(axis, -1, axis);
            fprintf(outfile, "%s ring %s_ring\n0\n0\n\t8\n\t",
                    mod_name, cyl->o_label);
            RadVPrint(cyl->o_world_verts[cyl->o_num_real]);
            fprintf(outfile, "\n\t");
            RadVPrint(axis);
            fprintf(outfile, "\n\t0.0 %1.15g\n", sqrt(rad_sq));

            VScalarMul(axis, -1, axis);
            fprintf(outfile, "%s ring %s_ring\n0\n0\n\t8\n\t",
                    mod_name, cyl->o_label);
            RadVPrint(cyl->o_world_verts[cyl->o_num_real + 1]);
            fprintf(outfile, "\n\t");
            RadVPrint(axis);
            fprintf(outfile, "\n\t0.0 %1.15g\n", sqrt(rad_sq));
        }

        free(mod_name);
    }

    return fprintf(outfile,"\n");
}


static int
Export_Cone(FILE *outfile, ObjectInstancePtr cone)
{
    Boolean uneven = FALSE;
    double  rad_sq, first_rad_sq;
    Vector  temp_v, axis;
    Vector  center;
    double  temp_d;
    int     i;

    /* Radiance only allows for cones with endcaps parallel to the axis
    ** and constant endcap radius.
    ** To get around this, we sample to key parameters.
    */

    /* Sample radius of vertices 1 through num - 2 with respect to the center
    ** of the endcap. Also test the the endcap is perpendicular to the axis.
    */
    center = cone->o_world_verts[cone->o_num_real];
    VSub(cone->o_world_verts[1], center, temp_v);
    first_rad_sq = VDot(temp_v, temp_v);
    for ( i = 2 ; i < cone->o_num_real && ! uneven ; i++ )
    {
        VSub(cone->o_world_verts[i], center, temp_v);
        rad_sq = VDot(temp_v, temp_v);
        if ( ! DEqual(rad_sq, first_rad_sq) )
            uneven = TRUE;
    }
    /* Also test that the endcap is perpendicular to the axis.  */
    /* Must calculate axis. */
    VSub(cone->o_world_verts[0],
         cone->o_world_verts[cone->o_num_real], axis);
    VSub(cone->o_world_verts[1],
         cone->o_world_verts[cone->o_num_real], temp_v);
    temp_d = VDot(axis, temp_v);
    if ( ! IsZero(temp_d) )
        uneven = TRUE;

    /* uneven is FALSE if it is a cone (ie constant radius surface). */
    if ( uneven )
        Export_Object(outfile, cone);
    else
    {
        char            *mod_name = New(char, 48);
        Boolean         invert;
        Boolean         open;

        fprintf(outfile, "#\n# %s\n#\n", cone->o_label);

        Export_Attributes(outfile, (AttributePtr)cone->o_attribs,
                          &invert, &open, &mod_name);

        if ( invert )
            fprintf(outfile, "%s cup %s\n0\n0\n8\n\t", mod_name, cone->o_label);
        else
            fprintf(outfile, "%s cone %s\n0\n0\n8\n\t", mod_name,
                    cone->o_label);
        RadVPrint(cone->o_world_verts[0]);  fprintf(outfile, "\n\t");
        RadVPrint(cone->o_world_verts[9]);  fprintf(outfile, "\n");
        fprintf(outfile, "\t0.0 %1.15g\n", sqrt(rad_sq));

        if ( ! open )
        {
            if ( ! invert )
                VScalarMul(axis, -1, axis);
            fprintf(outfile, "%s ring %s_ring\n0\n0\n\t8\n\t",
                    mod_name, cone->o_label);
            RadVPrint(cone->o_world_verts[9]);  fprintf(outfile, "\n\t");
            RadVPrint(axis);
            fprintf(outfile, "\n\t0.0 %1.15g\n", sqrt(rad_sq));
        }

        free(mod_name);
    }

    return fprintf(outfile,"\n");
}


static void
Export_Wireframe_Attributes(FILE *outfile, WireframePtr wire)
{
    char    *name = New(char, 48);
    Boolean dummy;
    int     i;

    for ( i = 0 ; i < wire->num_attribs ; i++ )
        Export_Attributes(outfile, wire->attribs[i], &dummy, &dummy, &name);

    free(name);
}


static int
Export_Wireframe(FILE *outfile, ObjectInstancePtr obj)
{
    char            *modifier = New(char, 48);
    Boolean         invert;
    Boolean         open;
    WireframePtr    wireframe;

    fprintf(outfile, "#\n# %s\n#\n", obj->o_label);

    if ( did_csg_wire )
	wireframe = obj->o_wireframe;
    else
	wireframe = Object_To_Wireframe(obj, TRUE);

    Export_Attributes(outfile, (AttributePtr)obj->o_attribs,
                      &invert, &open, &modifier);

    Export_Wireframe_Attributes(outfile, wireframe);

    Export_Polyhedral_Object(outfile, wireframe, obj->o_label, modifier,invert);

    Wireframe_Destroy(wireframe);
    if ( did_csg_wire )
	obj->o_wireframe = NULL;

    free(modifier);

    return fprintf(outfile,"\n");
}


static void
Export_Aggregate_Lights(FILE *outfile, InstanceList insts,
			TransformPtr add_trans)
{
    InstanceList    elmt;

    for ( elmt = insts ; elmt ; elmt = elmt->next )
    {
	if ( Obj_Is_Light(elmt->the_instance) )
	    Export_Light(outfile, elmt->the_instance, add_trans);
	else if ( Obj_Is_Aggregate(elmt->the_instance) )
	{
	    Transformation  new_trans;

	    Concat_Transform(elmt->the_instance->o_transform,
			     *add_trans, new_trans);
	    Export_Aggregate_Lights(outfile,
	       ((AggregatePtr)elmt->the_instance->o_parent->b_struct)->children,
	       &new_trans);
	}
    }
}


static int
Export_Aggregate(FILE *outfile, ObjectInstancePtr obj)
{
    Export_Wireframe(outfile, obj);
    Export_Aggregate_Lights(outfile,
			    ((AggregatePtr)obj->o_parent->b_struct)->children,
			    &(obj->o_transform));
    return fprintf(outfile,"\n");
}


static int
Export_Instances(FILE *outfile, InstanceList insts)
{
    InstanceList    elmt;

    for ( elmt = insts ; elmt ; elmt = elmt->next )
    {
        if ( Obj_Is_Construction(elmt->the_instance) )
            continue;

        if ( Obj_Is_Aliased(elmt->the_instance, Radiance) )
        {
            Export_Aliased_Object(outfile, elmt->the_instance);
            continue;
        }

        switch ( elmt->the_instance->o_parent->b_class )
        {
            case cube_obj:
            case triangle_obj:
            case bezier_obj:
            case torus_obj:
                Export_Object(outfile, elmt->the_instance);
                break;

            case plane_obj:
                Export_Plane(outfile, elmt->the_instance);
                break;
            
            case sphere_obj:
                Export_Sphere(outfile, elmt->the_instance);
                break;

            case cylinder_obj:
                Export_Cylinder(outfile, elmt->the_instance);
                break;

            case cone_obj:
                Export_Cone(outfile, elmt->the_instance);
                break;

            case csg_obj:
            case wireframe_obj:
                Export_Wireframe(outfile, elmt->the_instance);
                break;

	    case aggregate_obj:
                Export_Aggregate(outfile, elmt->the_instance);
                break;

            case light_obj:
            case spotlight_obj:
            case arealight_obj:
            case dirlight_obj:
                Export_Light(outfile, elmt->the_instance, &identity);
                break;

            case last_obj:;
        }
    }

    return 1;
}


static void
Radiance_Export_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(radiance_export_shell);
    export = TRUE;
}


static void
Radiance_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(radiance_export_shell);
    cancel = TRUE;
}


static void
Radiance_Create_Shell()
{
    Widget  form;
    Widget  labels[6];
    Widget  zone_toggles[2];
    Widget  var_toggles[3];
    Widget  det_toggles[3];
    Widget  qual_toggles[3];
    Widget  export_button, cancel_button;
    Arg     args[15];
    int     n, m;
    int     count;
    XtTranslations  toggle_translations =
                    XtParseTranslationTable(TOGGLE_TRANSLATIONS);

    n = 0;
    XtSetArg(args[n], XtNtitle, "Radiance");        n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    radiance_export_shell = XtCreatePopupShell("radianceExportShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("radianceExportForm", formWidgetClass,
                                 radiance_export_shell, args, n);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;

    count = 0;

    n = m;
    XtSetArg(args[n], XtNlabel, "ZONE");    n++;
    XtSetArg(args[n], XtNborderWidth, 0);   n++;
    labels[count] = XtCreateManagedWidget("radianceZoneLabel", labelWidgetClass,
                                        form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Interior");                n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);         n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)INTERIOR);   n++;
    zone_radio =
    zone_toggles[0] = XtCreateManagedWidget("radianceZoneIntToggle",
                        toggleWidgetClass, form, args, n);
    XtVaSetValues(zone_toggles[0], XtNradioGroup, zone_toggles[0], NULL);
    XtOverrideTranslations(zone_toggles[0], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "Exterior");                n++;
    XtSetArg(args[n], XtNfromHoriz, zone_toggles[0]);       n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)EXTERIOR);   n++;
    XtSetArg(args[n], XtNradioGroup, zone_radio);           n++;
    zone_toggles[1] = XtCreateManagedWidget("radianceZoneExtToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(zone_toggles[1], toggle_translations);
    XawToggleSetCurrent(zone_radio, (XtPointer)INTERIOR);

    n = m;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNfromVert, zone_toggles[0]);    n++;
    XtSetArg(args[n], XtNstring, "");                   n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNresize, XawtextResizeBoth);    n++;
    zone_text = XtCreateManagedWidget("radianceZoneText", asciiTextWidgetClass,
                                    form, args, n);

    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "EXPOSURE");    n++;
    XtSetArg(args[n], XtNborderWidth, 0);       n++;
    XtSetArg(args[n], XtNfromVert, zone_text);  n++;
    labels[count] = XtCreateManagedWidget("radianceExpLabel", labelWidgetClass,
                                            form, args, n);

    n = m;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNfromVert, zone_text);          n++;
    XtSetArg(args[n], XtNstring, "");                   n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNresize, XawtextResizeBoth);    n++;
    exposure_text = XtCreateManagedWidget("radianceExpText",
                                        asciiTextWidgetClass, form, args, n);

    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "VARIABILITY");     n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    XtSetArg(args[n], XtNfromVert, exposure_text);  n++;
    labels[count] = XtCreateManagedWidget("radianceVarLabel", labelWidgetClass,
                                            form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "LOW");                 n++;
    XtSetArg(args[n], XtNfromVert, exposure_text);      n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)LOW);    n++;
    var_radio =
    var_toggles[0] = XtCreateManagedWidget("radianceVarLowToggle",
                        toggleWidgetClass, form, args, n);
    XtVaSetValues(var_toggles[0], XtNradioGroup, var_toggles[0], NULL);
    XtOverrideTranslations(var_toggles[0], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "MEDIUM");              n++;
    XtSetArg(args[n], XtNfromVert, exposure_text);      n++;
    XtSetArg(args[n], XtNfromHoriz, var_toggles[0]);    n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)MEDIUM); n++;
    XtSetArg(args[n], XtNradioGroup, var_radio);        n++;
    var_toggles[1] = XtCreateManagedWidget("radianceVarMedToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(var_toggles[1], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "HIGH");                n++;
    XtSetArg(args[n], XtNfromVert, exposure_text);          n++;
    XtSetArg(args[n], XtNfromHoriz, var_toggles[1]);    n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)HIGH);   n++;
    XtSetArg(args[n], XtNradioGroup, var_radio);        n++;
    var_toggles[2] = XtCreateManagedWidget("radianceVarHighToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(var_toggles[2], toggle_translations);
    XawToggleSetCurrent(var_radio, (XtPointer)LOW);

    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "DETAIL");          n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    XtSetArg(args[n], XtNfromVert, var_toggles[0]); n++;
    labels[count] = XtCreateManagedWidget("radianceDetLabel", labelWidgetClass,
                                            form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "LOW");                 n++;
    XtSetArg(args[n], XtNfromVert, var_toggles[0]);     n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)LOW);    n++;
    det_radio =
    det_toggles[0] = XtCreateManagedWidget("radianceDetLowToggle",
                        toggleWidgetClass, form, args, n);
    XtVaSetValues(det_toggles[0], XtNradioGroup, det_toggles[0], NULL);
    XtOverrideTranslations(det_toggles[0], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "MEDIUM");              n++;
    XtSetArg(args[n], XtNfromVert, var_toggles[0]);     n++;
    XtSetArg(args[n], XtNfromHoriz, det_toggles[0]);    n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)MEDIUM); n++;
    XtSetArg(args[n], XtNradioGroup, det_radio);        n++;
    det_toggles[1] = XtCreateManagedWidget("radianceDetMedToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(det_toggles[1], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "HIGH");                n++;
    XtSetArg(args[n], XtNfromVert, var_toggles[0]);     n++;
    XtSetArg(args[n], XtNfromHoriz, det_toggles[1]);    n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)HIGH);   n++;
    XtSetArg(args[n], XtNradioGroup, det_radio);        n++;
    det_toggles[2] = XtCreateManagedWidget("radianceDetHighToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(det_toggles[2], toggle_translations);
    XawToggleSetCurrent(det_radio, (XtPointer)MEDIUM);

    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "QUALITY");         n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    XtSetArg(args[n], XtNfromVert, det_toggles[0]); n++;
    labels[count] = XtCreateManagedWidget("radianceQualLabel", labelWidgetClass,
                                            form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "LOW");                 n++;
    XtSetArg(args[n], XtNfromVert, det_toggles[0]);     n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)LOW);    n++;
    qual_radio =
    qual_toggles[0] = XtCreateManagedWidget("radianceQualLowToggle",
                        toggleWidgetClass, form, args, n);
    XtVaSetValues(qual_toggles[0], XtNradioGroup, qual_toggles[0], NULL);
    XtOverrideTranslations(qual_toggles[0], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "MEDIUM");              n++;
    XtSetArg(args[n], XtNfromVert, det_toggles[0]);     n++;
    XtSetArg(args[n], XtNfromHoriz, qual_toggles[0]);   n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)MEDIUM); n++;
    XtSetArg(args[n], XtNradioGroup, qual_radio);       n++;
    qual_toggles[1] = XtCreateManagedWidget("radianceQualMedToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(qual_toggles[1], toggle_translations);
    n = m;
    XtSetArg(args[n], XtNlabel, "HIGH");                n++;
    XtSetArg(args[n], XtNfromVert, det_toggles[0]);     n++;
    XtSetArg(args[n], XtNfromHoriz, qual_toggles[1]);   n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)HIGH);   n++;
    XtSetArg(args[n], XtNradioGroup, qual_radio);       n++;
    qual_toggles[2] = XtCreateManagedWidget("radianceQualHighToggle",
                        toggleWidgetClass, form, args, n);
    XtOverrideTranslations(qual_toggles[2], toggle_translations);
    XawToggleSetCurrent(qual_radio, (XtPointer)LOW);

    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "INDIRECT");        n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    XtSetArg(args[n], XtNfromVert, qual_toggles[0]);    n++;
    labels[count] = XtCreateManagedWidget("radianceIndLabel", labelWidgetClass,
                                            form, args, n);

    n = m;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNfromVert, qual_toggles[0]);    n++;
    XtSetArg(args[n], XtNstring, "0");                  n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNresize, XawtextResizeBoth);    n++;
    indirect_text = XtCreateManagedWidget("radianceIndText",
                                        asciiTextWidgetClass, form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Export");          n++;
    XtSetArg(args[n], XtNfromVert, indirect_text);  n++;
    export_button = XtCreateManagedWidget("radianceExportButton",
                        commandWidgetClass, form, args, n);
    XtAddCallback(export_button, XtNcallback, Radiance_Export_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNfromVert, indirect_text);  n++;
    XtSetArg(args[n], XtNfromHoriz, export_button); n++;
    cancel_button = XtCreateManagedWidget("radianceCancelButton",
                        commandWidgetClass, form, args, n);
    XtAddCallback(cancel_button, XtNcallback, Radiance_Cancel_Callback, NULL);

    Match_Widths(labels, count + 1);

    XtRealizeWidget(radiance_export_shell);
}


static void
Radiance_Set_Zone(InstanceList insts)
{
    InstanceList    elmt;
    Cuboid  zone;
    Cuboid  obj_extent;
    char    zone_string[128];

    /* Need to calculate the maximum extents of the zone. */
    VNew(0, 0, 0, zone.min);
    VNew(0, 0, 0, zone.max);
    if ( did_csg_wire )
    {
        obj_extent = Calculate_Bounds_f(
			insts->the_instance->o_wireframe->vertices,
			insts->the_instance->o_wireframe->num_vertices);
        zone.min.x = obj_extent.min.x;
        zone.max.x = obj_extent.max.x;
        zone.min.y = obj_extent.min.y;
        zone.max.y = obj_extent.max.y;
        zone.min.z = obj_extent.min.z;
        zone.max.z = obj_extent.max.z;
    }
    else
	for ( elmt = insts ; elmt ; elmt = elmt->next )
	{
	    obj_extent = Calculate_Bounds_f(elmt->the_instance->o_world_verts,
					    elmt->the_instance->o_num_real);
	    if ( zone.min.x > obj_extent.min.x ) zone.min.x = obj_extent.min.x;
	    if ( zone.max.x < obj_extent.max.x ) zone.max.x = obj_extent.max.x;
	    if ( zone.min.y > obj_extent.min.y ) zone.min.y = obj_extent.min.y;
	    if ( zone.max.y < obj_extent.max.y ) zone.max.y = obj_extent.max.y;
	    if ( zone.min.z > obj_extent.min.z ) zone.min.z = obj_extent.min.z;
	    if ( zone.max.z < obj_extent.max.z ) zone.max.z = obj_extent.max.z;
	}

    sprintf(zone_string, "%1.5g %1.5g %1.5g %1.5g %1.5g %1.5g",
            zone.min.x, zone.max.x, zone.min.y, zone.max.y, zone.min.z,
            zone.max.z);
    XtVaSetValues(zone_text, XtNstring, zone_string, NULL);
}


void
Radiance_Save_Extras(FILE *outfile)
{
    XtPointer   temp_p;
    char        *temp_s;

    if ( ! radiance_export_shell )
        Radiance_Create_Shell();

    /* Get and save assorted support information from the dialog. */
    temp_p = XawToggleGetCurrent(zone_radio);
    XtVaGetValues(zone_text, XtNstring, &temp_s, NULL);
    fprintf(outfile, "%d \"%s\" ", (int)temp_p, temp_s);
    XtVaGetValues(exposure_text, XtNstring, &temp_s, NULL); 
    fprintf(outfile, "\"%s\" ", temp_s);
    temp_p = XawToggleGetCurrent(var_radio);
    fprintf(outfile, "%d ", (int)temp_p);
    temp_p = XawToggleGetCurrent(det_radio);
    fprintf(outfile, "%d ", (int)temp_p);
    temp_p = XawToggleGetCurrent(qual_radio);
    fprintf(outfile, "%d ", (int)temp_p);
    XtVaGetValues(indirect_text, XtNstring, &temp_s, NULL); 
    fprintf(outfile, "\"%s\"\n", temp_s);
}


void
Radiance_Set_Extras(int zone_type, char *zone_string, char *exposure_string,
                    int var, int det, int qual, char  *indirect_string)
{
    if ( ! radiance_export_shell )
        Radiance_Create_Shell();

    XawToggleSetCurrent(zone_radio, (XtPointer)zone_type);
    XtVaSetValues(zone_text, XtNstring, zone_string, NULL);
    XtVaSetValues(exposure_text, XtNstring, exposure_string, NULL);
    XawToggleSetCurrent(var_radio, (XtPointer)var);
    XawToggleSetCurrent(det_radio, (XtPointer)det);
    XawToggleSetCurrent(qual_radio, (XtPointer)qual);
    XtVaSetValues(indirect_text, XtNstring, indirect_string, NULL);
}

