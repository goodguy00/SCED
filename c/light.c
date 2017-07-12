/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/light.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: light.c,v $
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
**  light.c : Functions for creating and managing lights.
**
**  void
**  Popup_New_Light_Shell();
**  Pops up the shell needed to create a new light.
*/

#include <sced.h>
#include <aggregate.h>
#include <attributes.h>
#include <instance_list.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

static void Create_Ambient_Dialog();
static void Set_Ambient_Callback(Widget, XtPointer, XtPointer);
static void Cancel_Light_Callback(Widget, XtPointer, XtPointer);

static void Create_Light_Dialog();
static void Light_Intensity_Callback(Widget, XtPointer, XtPointer);

static void Create_Spotlight_Dialog();

static void Create_Arealight_Dialog();

static Widget   ambient_light_shell = NULL;
static Widget   ambient_dialog;

/* The ambient light in the scene. */
ColorVector     ambient_light = { 0, 0, 0 };

static Boolean  spotlight_after;    /* Whether to popup the spotlight dialog. */
static Boolean  arealight_after;    /* Whether to popup the arealight dialog. */

static InstanceList instances;

#define MAX_STRING_LENGTH 24

static char     intensity_string[MAX_STRING_LENGTH];

static Widget   light_shell = NULL;
static Widget   light_dialog;

static Widget   spotlight_shell = NULL;
static Widget   spotlight_intensity_text;
static Widget   radius_text;
static char     radius_string[MAX_STRING_LENGTH];
static Widget   tightness_text;
static char     tightness_string[MAX_STRING_LENGTH];
static Widget   invert_toggle;

static Widget   arealight_shell = NULL;
static Widget   arealight_intensity_text;
static Widget   xnum_text;
static char     xnum_string[MAX_STRING_LENGTH];
static Widget   ynum_text;
static char     ynum_string[MAX_STRING_LENGTH];
static Widget   jitter_toggle;

static Boolean  light_doing_defaults;

static void
Light_Create_Simple_Attribs(ObjectInstancePtr obj)
{
    obj->o_attribs = Attribute_New(NULL, TRUE);
}


void
Light_Create_Point_Light(ObjectInstancePtr obj)
{
    Light_Create_Simple_Attribs(obj);

    obj->o_num_features = 1;
    obj->o_features = New(Feature, 1);

    Feature_Create_Origin_Constraints(obj->o_features);

    obj->o_dynamic_func = Maintain_Point_Dynamic;
    obj->o_static_func = Maintain_Point_Static;
}

void
Light_Create_Area_Light(ObjectInstancePtr obj)
{
    Light_Create_Simple_Attribs(obj);

    obj->o_num_features = 4;
    obj->o_features = New(Feature, 4);

    Feature_Create_Origin_Constraints(obj->o_features);
    Feature_Create_Major_Constraints(obj->o_features + major_feature, 0, 0, 1);
    Feature_Create_Minor_Constraints(obj->o_features + minor_feature, 1, 0, 0);
    Feature_Create_Scale_Constraints(obj->o_features + scale_feature,
                                obj->o_world_verts + obj->o_parent->b_ref_num);

    obj->o_dynamic_func = Maintain_Generic_Dynamic;
    obj->o_static_func = Maintain_Generic_Static;
}

void
Light_Create_Dir_Light(ObjectInstancePtr obj)
{
    Light_Create_Simple_Attribs(obj);

    obj->o_num_features = 3;
    obj->o_features = New(Feature, 3);

    Feature_Create_Origin_Constraints(obj->o_features);
    Feature_Create_Major_Constraints(obj->o_features + major_feature, 0, 0, -1);
    Feature_Create_Minor_Constraints(obj->o_features + minor_feature, -1, 0, 0);

    obj->o_dynamic_func = Maintain_Dir_Dynamic;
    obj->o_static_func = Maintain_Dir_Static;
}

void
Light_Create_Spot_Light(ObjectInstancePtr obj)
{
    Light_Create_Simple_Attribs(obj);

    obj->o_num_features = 4;
    obj->o_features = New(Feature, 4);

    Feature_Create_Origin_Constraints(obj->o_features);
    Feature_Create_Major_Constraints(obj->o_features + major_feature, 0, 0, -1);
    Feature_Create_Minor_Constraints(obj->o_features + minor_feature, -1, 0, 0);
    Feature_Create_Cone_Scale_Constraints(obj->o_features + scale_feature, obj,
                                obj->o_world_verts + obj->o_parent->b_ref_num);

    obj->o_dynamic_func = Maintain_Generic_Dynamic;
    obj->o_static_func = Maintain_Generic_Static;
}

void
Light_Destroy_Object(ObjectInstancePtr obj)
{
    free(obj->o_attribs);
}

void
Create_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Create_New_Object_From_Base((WindowInfoPtr)cl,
                                Get_Base_Object_From_Label("light"), FALSE);
}


void
Create_Spotlight_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Create_New_Object_From_Base((WindowInfoPtr)cl,
                                Get_Base_Object_From_Label("spotlight"),
                                FALSE);
}

void
Create_Arealight_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Create_New_Object_From_Base((WindowInfoPtr)cl,
                                Get_Base_Object_From_Label("arealight"),
                                FALSE);
}

void
Create_Dirlight_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Create_New_Object_From_Base((WindowInfoPtr)cl,
                                Get_Base_Object_From_Label("dirlight"),
                                FALSE);
}


void
Pointlight_Set_Defaults_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    char    light_string[20];

    light_doing_defaults = TRUE;
    arealight_after = FALSE;
    spotlight_after = FALSE;

    if ( ! light_shell )
        Create_Light_Dialog();

    sprintf(light_string, "%1.2g %1.2g %1.2g",
            sced_preferences.default_attributes.intensity.red,
            sced_preferences.default_attributes.intensity.green,
            sced_preferences.default_attributes.intensity.blue);

    XtVaSetValues(light_dialog, XtNvalue, light_string, NULL);

    Position_Shell(light_shell, FALSE);
    XtPopup(light_shell, XtGrabExclusive);
}

void
Spotlight_Set_Defaults_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XawTextBlock    text_block;
    int             old_length;

    light_doing_defaults = TRUE;
    arealight_after = FALSE;

    if ( ! spotlight_shell )
        Create_Spotlight_Dialog();

    /* Set all the text strings. */
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    old_length = strlen(intensity_string);
    sprintf(intensity_string, "%0.2g %0.2g %0.2g", 
            sced_preferences.default_attributes.intensity.red,
            sced_preferences.default_attributes.intensity.green,
            sced_preferences.default_attributes.intensity.blue);
    text_block.length = strlen(intensity_string);
    text_block.ptr = intensity_string;
    XawTextReplace(spotlight_intensity_text, 0, old_length + 1, &text_block);

    old_length = strlen(radius_string);
    sprintf(radius_string, "%0.2g"HPFIX,
            sced_preferences.default_attributes.radius);
    text_block.length = strlen(radius_string);
    text_block.ptr = radius_string;
    XawTextReplace(radius_text, 0, old_length + 1, &text_block);

    old_length = strlen(tightness_string);
    sprintf(tightness_string, "%0.2g"HPFIX,
            sced_preferences.default_attributes.tightness);
    text_block.length = strlen(tightness_string);
    text_block.ptr = tightness_string;
    XawTextReplace(tightness_text, 0, old_length + 1, &text_block);

    /* Set the inverse toggle. */
    XtVaSetValues(invert_toggle, XtNstate,
                  sced_preferences.default_attributes.invert, NULL);

    Position_Shell(spotlight_shell, FALSE);
    XtPopup(spotlight_shell, XtGrabExclusive);
}


void
Arealight_Set_Defaults_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XawTextBlock    text_block;
    int             old_length;

    light_doing_defaults = TRUE;

    if ( ! arealight_shell )
        Create_Arealight_Dialog();

    /* Set all the text strings. */
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    old_length = strlen(intensity_string);
    sprintf(intensity_string, "%0.2g %0.2g %0.2g", 
            sced_preferences.default_attributes.intensity.red,
            sced_preferences.default_attributes.intensity.green,
            sced_preferences.default_attributes.intensity.blue);
    text_block.length = strlen(intensity_string);
    text_block.ptr = intensity_string;
    XawTextReplace(arealight_intensity_text, 0, old_length + 1, &text_block);

    old_length = strlen(xnum_string);
    sprintf(xnum_string, "%d"HPFIX,
            sced_preferences.default_attributes.samples & 0xFF);
    text_block.length = strlen(xnum_string);
    text_block.ptr = xnum_string;
    XawTextReplace(xnum_text, 0, old_length + 1, &text_block);

    old_length = strlen(ynum_string);
    sprintf(ynum_string, "%d"HPFIX,
            ( sced_preferences.default_attributes.samples & 0xFF00 ) >> 8);
    text_block.length = strlen(ynum_string);
    text_block.ptr = ynum_string;
    XawTextReplace(ynum_text, 0, old_length + 1, &text_block);

    XtVaSetValues(jitter_toggle, XtNstate,
                  sced_preferences.default_attributes.jitter, NULL);

    Position_Shell(arealight_shell, FALSE);
    XtPopup(arealight_shell, XtGrabExclusive);
}

/*  void
**  Ambient_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
**  Pops up the ambient light dialog.
*/
void
Ambient_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    char    ambient_string[20];

    if ( ! ambient_light_shell )
        Create_Ambient_Dialog();

    sprintf(ambient_string, "%1.2g %1.2g %1.2g", (double)ambient_light.red,
            (double)ambient_light.green, (double)ambient_light.blue);

    XtVaSetValues(ambient_dialog, XtNvalue, ambient_string, NULL);

    Position_Shell(ambient_light_shell, FALSE);

    XtPopup(ambient_light_shell, XtGrabExclusive);
}


Boolean
Light_In_Instances(InstanceList insts)
{
    for ( ; insts ; insts = insts->next )
        if ( Obj_Is_Light(insts->the_instance) ||
	     ( Obj_Is_Aggregate(insts->the_instance) && 
               Aggregate_Contains_Light(insts->the_instance->o_parent, FALSE) ))
            return TRUE;

    return FALSE;
}


/*  void
**  Create_Ambient_Dialog() 
**  Creates the shell used to set the ambient light.
*/
static void
Create_Ambient_Dialog()
{
    Arg args[3];
    int n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Ambient Light");   n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    ambient_light_shell =  XtCreatePopupShell("ambientShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNlabel, "Ambient color:");  n++;
    XtSetArg(args[n], XtNvalue, "");                n++;
    ambient_dialog = XtCreateManagedWidget("ambientDialog", dialogWidgetClass,
                            ambient_light_shell, args, n);

    XawDialogAddButton(ambient_dialog, "Done", Set_Ambient_Callback, NULL);
    XawDialogAddButton(ambient_dialog, "Cancel", Cancel_Light_Callback,
                        (XtPointer)ambient_light_shell);

    XtOverrideTranslations(XtNameToWidget(ambient_dialog, "value"),
        XtParseTranslationTable(":<Key>Return: Ambient_Action()"));

    XtVaSetValues(XtNameToWidget(ambient_dialog, "label"),
                  XtNborderWidth, 0, NULL);

    XtRealizeWidget(ambient_light_shell);
}



/*  void
**  Set_Ambient_Callback(Widget w, XtPointer cl, XtPointer ca)
**  Sets the ambient light from the dialog shell.
*/
static void
Set_Ambient_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    String  ambient_text;
    float   r, g, b;
    int     num_args;

    XtPopdown(ambient_light_shell);

    ambient_text = XawDialogGetValueString(ambient_dialog);

    num_args = sscanf(ambient_text, "%f %f %f", &r, &g, &b);
    ambient_light.red = r;
    if ( num_args < 2 )
        ambient_light.green = ambient_light.red;
    else
        ambient_light.green = g;
    if ( num_args < 3 )
        ambient_light.blue = ambient_light.red;
    else
        ambient_light.blue = b;

    changed_scene = TRUE;

}


void
Ambient_Action_Func(Widget w, XEvent *e, String *s, Cardinal *n)
{
    Set_Ambient_Callback(w, NULL, NULL);
}


/*  void
**  Cancel_Ambient_Callback(Widget w, XtPointer cl, XtPointer ca)
**  Cancels an ambience session.
*/
static void
Cancel_Light_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown((Widget)cl);

    if ( spotlight_after )
        Set_Spotlight_Attributes(instances, arealight_after);
    else if ( arealight_after )
        Set_Arealight_Attributes(instances);
}


static void
Light_Intensity_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    String  int_string;
    float   r = 0.0;
    float   g = 0.0;
    float   b = 0.0;
    int     num_args;
    InstanceList    elmt;

    XtPopdown(light_shell);

    int_string = XawDialogGetValueString(light_dialog);

    num_args = sscanf(int_string, "%f %f %f", &r, &g, &b);
    if ( num_args < 3 )
        g = b = r;

    if ( light_doing_defaults )
    {
        Attributes  new_vals = sced_preferences.default_attributes;
        new_vals.intensity.red = r;
        new_vals.intensity.green = g;
        new_vals.intensity.blue = b;
        Attributes_Set_Defaults(&new_vals, ModLight);
        return;
    }

    for ( elmt = instances ; elmt ; elmt = elmt->next )
        if ( elmt->the_instance->o_parent &&
             ( elmt->the_instance->o_parent->b_class == light_obj ||
               elmt->the_instance->o_parent->b_class == dirlight_obj ||
               elmt->the_instance->o_parent->b_class == aggregate_obj ) )
        {
            elmt->the_instance->o_attribs->intensity.red = r;
            elmt->the_instance->o_attribs->intensity.green = g;
            elmt->the_instance->o_attribs->intensity.blue = b;
        }

    changed_scene = TRUE;

    if ( spotlight_after )
        Set_Spotlight_Attributes(instances, arealight_after);
    else if ( arealight_after )
        Set_Arealight_Attributes(instances);
}


void
Light_Action_Func(Widget w, XEvent *e, String *s, Cardinal *n)
{
    Light_Intensity_Callback(w, NULL, NULL);
}




static void
Spotlight_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;
    double  radius;
    double  tightness;
    double  red, green, blue;
    Boolean invert;
    XtPopdown(spotlight_shell);

    if ( sscanf(intensity_string, "%lf %lf %lf", &red, &green, &blue) < 3 )
        blue = green = red;
    sscanf(radius_string, "%lf", &radius);
    sscanf(tightness_string, "%lf", &tightness);
    XtVaGetValues(invert_toggle, XtNstate, &invert, NULL);

    if ( light_doing_defaults )
    {
        Attributes  new_vals = sced_preferences.default_attributes;
        new_vals.intensity.red = red;
        new_vals.intensity.green = green;
        new_vals.intensity.blue = blue;
        new_vals.radius = radius;
        new_vals.tightness = tightness;
        new_vals.invert = invert;
        Attributes_Set_Defaults(&new_vals, ModSpotlight);
        return;
    }

    for ( elmt = instances ; elmt ; elmt = elmt->next )
        if ( elmt->the_instance->o_parent &&
             ( elmt->the_instance->o_parent->b_class == spotlight_obj ||
               elmt->the_instance->o_parent->b_class == aggregate_obj ) )
        {
            elmt->the_instance->o_attribs->intensity.red = red;
            elmt->the_instance->o_attribs->intensity.green = green;
            elmt->the_instance->o_attribs->intensity.blue = blue;
            elmt->the_instance->o_attribs->radius = radius;
            elmt->the_instance->o_attribs->tightness = tightness;
            elmt->the_instance->o_attribs->invert = invert;
        }

    changed_scene = TRUE;

    if ( arealight_after )
        Set_Arealight_Attributes(instances);
}


static void
Arealight_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;
    int     xnum;
    int     ynum;
    double  red, green, blue;
    Boolean jitter;

    XtPopdown(arealight_shell);

    if ( sscanf(intensity_string, "%lf %lf %lf", &red, &green, &blue) < 3 )
        blue = green = red;
    sscanf(xnum_string, "%d", &xnum);
    sscanf(ynum_string, "%d", &ynum);
    XtVaGetValues(jitter_toggle, XtNstate, &jitter, NULL);

    if ( light_doing_defaults )
    {
        Attributes  new_vals = sced_preferences.default_attributes;
        new_vals.intensity.red = red;
        new_vals.intensity.green = green;
        new_vals.intensity.blue = blue;
        new_vals.samples = ( xnum & 0xFF ) | ( ynum << 8 );
        new_vals.jitter = jitter;
        Attributes_Set_Defaults(&new_vals, ModArealight);
        return;
    }

    for ( elmt = instances ; elmt ; elmt = elmt->next )
        if ( elmt->the_instance->o_parent &&
             ( elmt->the_instance->o_parent->b_class == arealight_obj ||
               elmt->the_instance->o_parent->b_class == aggregate_obj ) )
        {
            elmt->the_instance->o_attribs->intensity.red = red;
            elmt->the_instance->o_attribs->intensity.green = green;
            elmt->the_instance->o_attribs->intensity.blue = blue;
            elmt->the_instance->o_attribs->samples =
                                                ( xnum & 0xFF ) | ( ynum << 8 );
            elmt->the_instance->o_attribs->jitter = jitter;
        }

    changed_scene = TRUE;
}

void
Set_Light_Attributes(InstanceList objects, Boolean do_spot, Boolean do_area)
{
    InstanceList    elmt;
    InstanceList    agg_list = NULL;
    char    light_string[20];

    instances = objects;
    spotlight_after = do_spot;
    arealight_after = do_area;

    if ( ! light_shell )
        Create_Light_Dialog();

    for ( elmt = objects ;
          elmt &&
          ( ( ! elmt->the_instance->o_parent ) ||
            ( elmt->the_instance->o_parent->b_class != light_obj &&
              elmt->the_instance->o_parent->b_class != dirlight_obj ) ) ;
          elmt = elmt->next );
    if ( ! elmt )
    {
        for ( elmt = objects ;
              elmt &&
              ( ( ! elmt->the_instance->o_parent ) ||
                elmt->the_instance->o_parent->b_class != aggregate_obj ||
                ! ( agg_list = Aggregate_Contains_Pointlight(
                                        elmt->the_instance->o_parent, TRUE) ) ||
                ! ( agg_list = Aggregate_Contains_Dirlight(
                                        elmt->the_instance->o_parent, TRUE) ) );
              elmt = elmt->next );
        if ( agg_list )
            elmt = agg_list;
    }

    if ( ! elmt )
        return;

    sprintf(light_string, "%1.2g %1.2g %1.2g",
            elmt->the_instance->o_attribs->intensity.red,
            elmt->the_instance->o_attribs->intensity.green,
            elmt->the_instance->o_attribs->intensity.blue);

    if ( agg_list )
        Free_Selection_List(agg_list);

    XtVaSetValues(light_dialog, XtNvalue, light_string, NULL);

    light_doing_defaults = FALSE;

    Position_Shell(light_shell, FALSE);

    XtPopup(light_shell, XtGrabExclusive);
}


void
Set_Spotlight_Attributes(InstanceList objects, Boolean do_area)
{
    InstanceList    elmt;
    InstanceList    agg_list = NULL;
    XawTextBlock    text_block;
    int             old_length;

    instances = objects;
    arealight_after = do_area;

    spotlight_after = FALSE;

    if ( ! spotlight_shell )
        Create_Spotlight_Dialog();

    for ( elmt = objects ;
          elmt &&
          ( ( ! elmt->the_instance->o_parent ) ||
            elmt->the_instance->o_parent->b_class != spotlight_obj ) ;
          elmt = elmt->next );
    if ( ! elmt )
    {
        for ( elmt = objects ;
              elmt &&
              ( ( ! elmt->the_instance->o_parent ) ||
                elmt->the_instance->o_parent->b_class != aggregate_obj ||
                ! ( agg_list = Aggregate_Contains_Pointlight(
                                        elmt->the_instance->o_parent, TRUE) ) ||
                ! ( agg_list = Aggregate_Contains_Dirlight(
                                        elmt->the_instance->o_parent, TRUE) ) );
              elmt = elmt->next );
        if ( agg_list )
            elmt = agg_list;
    }

    if ( ! elmt )
        return;

    /* Set all the text strings. */
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    old_length = strlen(intensity_string);
    sprintf(intensity_string, "%0.2g %0.2g %0.2g", 
            elmt->the_instance->o_attribs->intensity.red,
            elmt->the_instance->o_attribs->intensity.green,
            elmt->the_instance->o_attribs->intensity.blue);
    text_block.length = strlen(intensity_string);
    text_block.ptr = intensity_string;
    XawTextReplace(spotlight_intensity_text, 0, old_length + 1, &text_block);

    old_length = strlen(radius_string);
    sprintf(radius_string, "%0.2g"HPFIX,
            elmt->the_instance->o_attribs->radius);
    text_block.length = strlen(radius_string);
    text_block.ptr = radius_string;
    XawTextReplace(radius_text, 0, old_length + 1, &text_block);

    old_length = strlen(tightness_string);
    sprintf(tightness_string, "%0.2g"HPFIX,
            elmt->the_instance->o_attribs->tightness);
    text_block.length = strlen(tightness_string);
    text_block.ptr = tightness_string;
    XawTextReplace(tightness_text, 0, old_length + 1, &text_block);

    /* Set the inverse toggle. */
    XtVaSetValues(invert_toggle, XtNstate,
                  elmt->the_instance->o_attribs->invert, NULL);

    if ( agg_list )
        Free_Selection_List(agg_list);

    light_doing_defaults = FALSE;

    Position_Shell(spotlight_shell, FALSE);

    XtPopup(spotlight_shell, XtGrabExclusive);
}


void
Set_Arealight_Attributes(InstanceList objects)
{
    InstanceList    elmt;
    InstanceList    agg_list = NULL;
    XawTextBlock    text_block;
    int             old_length;

    instances = objects;

    arealight_after = FALSE;

    if ( ! arealight_shell )
        Create_Arealight_Dialog();

    for ( elmt = objects ;
          elmt &&
          ( ( ! elmt->the_instance->o_parent ) ||
            elmt->the_instance->o_parent->b_class != arealight_obj ) ;
          elmt = elmt->next );
    if ( ! elmt )
    {
        for ( elmt = objects ;
              elmt &&
              ( ( ! elmt->the_instance->o_parent ) ||
                elmt->the_instance->o_parent->b_class != aggregate_obj ||
                ! ( agg_list = Aggregate_Contains_Pointlight(
                                        elmt->the_instance->o_parent, TRUE) ) ||
                ! ( agg_list = Aggregate_Contains_Dirlight(
                                        elmt->the_instance->o_parent, TRUE) ) );
              elmt = elmt->next );
        if ( agg_list )
            elmt = agg_list;
    }

    if ( ! elmt )
        return;

    /* Set all the text strings. */
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    old_length = strlen(intensity_string);
    sprintf(intensity_string, "%0.2g %0.2g %0.2g", 
            elmt->the_instance->o_attribs->intensity.red,
            elmt->the_instance->o_attribs->intensity.green,
            elmt->the_instance->o_attribs->intensity.blue);
    text_block.length = strlen(intensity_string);
    text_block.ptr = intensity_string;
    XawTextReplace(arealight_intensity_text, 0, old_length + 1, &text_block);

    old_length = strlen(xnum_string);
    sprintf(xnum_string, "%d"HPFIX,
            elmt->the_instance->o_attribs->samples & 0xFF);
    text_block.length = strlen(xnum_string);
    text_block.ptr = xnum_string;
    XawTextReplace(xnum_text, 0, old_length + 1, &text_block);

    old_length = strlen(ynum_string);
    sprintf(ynum_string, "%d"HPFIX,
            ( elmt->the_instance->o_attribs->samples & 0xFF00 ) >> 8);
    text_block.length = strlen(ynum_string);
    text_block.ptr = ynum_string;
    XawTextReplace(ynum_text, 0, old_length + 1, &text_block);

    XtVaSetValues(jitter_toggle, XtNstate,
                  elmt->the_instance->o_attribs->jitter, NULL);

    if ( agg_list )
        Free_Selection_List(agg_list);

    light_doing_defaults = FALSE;

    Position_Shell(arealight_shell, FALSE);
    XtPopup(arealight_shell, XtGrabExclusive);
}


static void
Create_Light_Dialog()
{
    Arg args[3];
    int n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Intensity");   n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    light_shell =  XtCreatePopupShell("lightShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNlabel, "Light Intensity"); n++;
    XtSetArg(args[n], XtNvalue, "");                    n++;
    light_dialog = XtCreateManagedWidget("lightDialog", dialogWidgetClass,
                            light_shell, args, n);

    XawDialogAddButton(light_dialog, "Done", Light_Intensity_Callback, NULL);
    XawDialogAddButton(light_dialog, "Cancel", Cancel_Light_Callback,
                        (XtPointer)light_shell);

    XtOverrideTranslations(XtNameToWidget(light_dialog, "value"),
        XtParseTranslationTable(":<Key>Return: Light_Action()"));

    XtVaSetValues(XtNameToWidget(light_dialog, "label"),
                  XtNborderWidth, 0, NULL);

    XtRealizeWidget(light_shell);
}


static void
Create_Spotlight_Dialog()
{
    Widget  form;
    Widget  labels[3];
    Widget  done, cancel;
    Arg     args[15];
    int     m, n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Spotlight");       n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    spotlight_shell = XtCreatePopupShell("spotlightShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("spotlightForm", formWidgetClass,
                                 spotlight_shell, args, n);

    /* Common args. */
    m = 0;
    XtSetArg(args[m], XtNleft, XtChainLeft);        m++;
    XtSetArg(args[m], XtNright, XtChainRight);      m++;
    XtSetArg(args[m], XtNtop, XtChainTop);          m++;
    XtSetArg(args[m], XtNbottom, XtChainBottom);    m++;

    /* Intensity label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Intensity");       n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    labels[0] = XtCreateManagedWidget("spotlightIntensityLabel",
                                      labelWidgetClass, form, args, n);

    /* Intensity text. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);    n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, intensity_string);     n++;
    XtSetArg(args[n], XtNfromHoriz, labels[0]);         n++;
    spotlight_intensity_text = XtCreateManagedWidget("spotlightIntensityText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(spotlight_intensity_text,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    /* Outer Radius label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Outer Radius");                n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    XtSetArg(args[n], XtNfromVert, spotlight_intensity_text);   n++;
    labels[1] = XtCreateManagedWidget("spotlightRadiusLabel",
                                      labelWidgetClass, form, args, n);

    /* Radius text. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);    n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, radius_string);        n++;
    XtSetArg(args[n], XtNfromHoriz, labels[1]);         n++;
    XtSetArg(args[n], XtNfromVert, spotlight_intensity_text);   n++;
    radius_text = XtCreateManagedWidget("spotlightRadiusText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(radius_text,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    /* Tightness label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Tightness");       n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    XtSetArg(args[n], XtNfromVert, radius_text);    n++;
    labels[2] = XtCreateManagedWidget("spotlightTightnessLabel",
                                      labelWidgetClass, form, args, n);

    /* Radius text. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);    n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, tightness_string);     n++;
    XtSetArg(args[n], XtNfromHoriz, labels[2]);         n++;
    XtSetArg(args[n], XtNfromVert, radius_text);        n++;
    tightness_text = XtCreateManagedWidget("spotlightTightnessText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(tightness_text,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    Match_Widths(labels, 3);

    /* The invert toggle. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Invert");          n++;
    XtSetArg(args[n], XtNfromVert, tightness_text); n++;
    invert_toggle = XtCreateManagedWidget("spotlightInvertToggle",
                        toggleWidgetClass, form, args, n);

    /* Done and cancel. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Done");            n++;
    XtSetArg(args[n], XtNfromVert, invert_toggle);  n++;
    done = XtCreateManagedWidget("spotlightDoneCommand", commandWidgetClass,
                                 form, args, n);
    XtAddCallback(done, XtNcallback, Spotlight_Attributes_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNfromVert, invert_toggle);  n++;
    XtSetArg(args[n], XtNfromHoriz, done);          n++;
    cancel = XtCreateManagedWidget("spotlightCancelCommand", commandWidgetClass,
                                   form, args, n);
    XtAddCallback(cancel, XtNcallback, Cancel_Light_Callback,
                  (XtPointer)spotlight_shell);

    XtRealizeWidget(spotlight_shell);
}


static void
Create_Arealight_Dialog()
{
    Widget  form;
    Widget  labels[3];
    Widget  done, cancel;
    Arg     args[15];
    int     m, n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Arealight");       n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    arealight_shell = XtCreatePopupShell("arealightShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("arealightForm", formWidgetClass,
                                 arealight_shell, args, n);

    /* Common args. */
    m = 0;
    XtSetArg(args[m], XtNleft, XtChainLeft);        m++;
    XtSetArg(args[m], XtNright, XtChainRight);      m++;
    XtSetArg(args[m], XtNtop, XtChainTop);          m++;
    XtSetArg(args[m], XtNbottom, XtChainBottom);    m++;

    /* Intensity label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Intensity");       n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    labels[0] = XtCreateManagedWidget("arealightIntensityLabel",
                                      labelWidgetClass, form, args, n);

    /* Intensity text. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);    n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, intensity_string);     n++;
    XtSetArg(args[n], XtNfromHoriz, labels[0]);         n++;
    arealight_intensity_text = XtCreateManagedWidget("arealightIntensityText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(arealight_intensity_text,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    /* Xnum label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Xnum");                        n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    XtSetArg(args[n], XtNfromVert, arealight_intensity_text);   n++;
    labels[1] = XtCreateManagedWidget("arealightXnumLabel",
                                      labelWidgetClass, form, args, n);

    /* Xnum text. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);    n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, xnum_string);          n++;
    XtSetArg(args[n], XtNfromHoriz, labels[1]);         n++;
    XtSetArg(args[n], XtNfromVert, arealight_intensity_text);   n++;
    xnum_text = XtCreateManagedWidget("arealightXnumText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(xnum_text,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    /* Ynum label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Ynum");        n++;
    XtSetArg(args[n], XtNborderWidth, 0);       n++;
    XtSetArg(args[n], XtNfromVert, xnum_text);  n++;
    labels[2] = XtCreateManagedWidget("arealightYnumLabel",
                                      labelWidgetClass, form, args, n);

    /* Ynum text. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_STRING_LENGTH);    n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, ynum_string);          n++;
    XtSetArg(args[n], XtNfromHoriz, labels[2]);         n++;
    XtSetArg(args[n], XtNfromVert, xnum_text);          n++;
    ynum_text = XtCreateManagedWidget("arealightYnumText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(ynum_text,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    Match_Widths(labels, 3);

    /* The jitter toggle. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Jitter");      n++;
    XtSetArg(args[n], XtNfromVert, ynum_text);  n++;
    jitter_toggle = XtCreateManagedWidget("arealightJitterToggle",
                        toggleWidgetClass, form, args, n);

    /* Done and cancel. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Done");            n++;
    XtSetArg(args[n], XtNfromVert, jitter_toggle);  n++;
    done = XtCreateManagedWidget("arealightDoneCommand", commandWidgetClass,
                                 form, args, n);
    XtAddCallback(done, XtNcallback, Arealight_Attributes_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNfromVert, jitter_toggle);  n++;
    XtSetArg(args[n], XtNfromHoriz, done);          n++;
    cancel = XtCreateManagedWidget("arealightCancelCommand", commandWidgetClass,
                                   form, args, n);
    XtAddCallback(cancel, XtNcallback, Cancel_Light_Callback,
                  (XtPointer)arealight_shell);

    XtRealizeWidget(arealight_shell);
}

