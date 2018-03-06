/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/attributes.c,v 1.1 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: attributes.c,v $
 * Revision 1.1  1997/06/11 21:12:52  schenney
 * Added type equality function.
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
**  attributes.c : attribute definition dialog box functions.
**
**  Created: 09/06/94
**
*/

#include <math.h>
#include <sced.h>
#include <aggregate.h>

static void Update_Attribute_Strings(AttributePtr);

static void Create_Attributes_Dialog();
static void Create_Declare_Dialog();

static void Target_Attributes_Callback();
static void Default_Attributes_Callback(Widget, XtPointer, XtPointer);
static void No_Attributes_Callback(Widget, XtPointer, XtPointer);
static void Attributes_Cancel_Callback(Widget, XtPointer, XtPointer);
static void Apply_Attributes_Callback(Widget, XtPointer, XtPointer);
static void Declares_Callback(Widget, XtPointer, XtPointer);
static void Create_Specific_Attributes_Dialog();

static void LOD_Callback(Widget, XtPointer, XtPointer);

static Widget   attributes_dialog_shell = NULL;
static Widget   attributes_form;

static Widget   lod_shell;
static Widget   *lod_text;
static int      lod_num;

static WindowInfoPtr    window;

#define MAX_TEXT_LENGTH 10
#define NUM_TEXT_STRINGS 7

static char red_text[MAX_TEXT_LENGTH] = {'\0'};
static char green_text[MAX_TEXT_LENGTH] = {'\0'};
static char blue_text[MAX_TEXT_LENGTH] = {'\0'};
static char diffuse_text[MAX_TEXT_LENGTH] = {'\0'};
static char specular_text[MAX_TEXT_LENGTH] = {'\0'};
static char spec_power_text[MAX_TEXT_LENGTH] = {'\0'};
static char reflect_text[MAX_TEXT_LENGTH] = {'\0'};
static char transparency_text[MAX_TEXT_LENGTH] = {'\0'};
static char refract_text[MAX_TEXT_LENGTH] = {'\0'};

static Widget   attribute_text[NUM_TEXT_STRINGS];
static Widget   rgb_text[3];
static Widget   none_button;
static Widget   target_button;

/* Declaration handling. */
char        *declarations[LastTarget] = {NULL};
static Widget   declare_text;
static Widget   declare_shell = NULL;

/* Specialised attribute handling. */
static Widget   specific_attributes_shell = NULL;
static Widget   specific_attributes_text;
static Widget   specific_transform_toggle;
static Widget   specific_open_toggle;
static Widget   specific_lod_button;

/* Functions for controlling whether light dialogs appear. */
static Boolean  attr_have_light;
static Boolean  attr_have_spotlight;
static Boolean  attr_have_arealight;

/* Flag for controlling what happens when done. */
static Boolean  attr_doing_defaults;

Boolean
Attributes_Equal(AttributePtr a, AttributePtr b)
{
    if ( a == b )
	return TRUE;

    if ( a->defined != b->defined ||
	 ! DEqual(a->color.red, b->color.red) ||
	 ! DEqual(a->color.green, b->color.green) ||
	 ! DEqual(a->color.blue, b->color.blue) ||
	 ! DEqual(a->diff_coef, b->diff_coef) ||
	 ! DEqual(a->spec_coef, b->spec_coef) ||
	 ! DEqual(a->spec_power, b->spec_power) ||
	 ! DEqual(a->reflect_coef, b->reflect_coef) ||
	 ! DEqual(a->transparency, b->transparency) ||
	 ! DEqual(a->refract_index, b->refract_index) ||
	 a->use_extension ||
	 b->use_extension ||
	 ! DEqual(a->intensity.red, b->intensity.red) ||
	 ! DEqual(a->intensity.green, b->intensity.green) ||
	 ! DEqual(a->intensity.blue, b->intensity.blue) ||
	 ! DEqual(a->radius, b->radius) ||
	 ! DEqual(a->tightness, b->tightness) ||
	 a->samples != b->samples ||
	 a->jitter != b->jitter ||
	 a->invert != b->invert )
	return FALSE;

    return TRUE;
}


void
Set_Attributes_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    Boolean         have_objs = FALSE;
    InstanceList    elmt;

    if ( cl_data )
        window = (WindowInfoPtr)cl_data;

    if ( ! attributes_dialog_shell )
        Create_Attributes_Dialog();

    attr_doing_defaults = FALSE;
    attr_have_light = attr_have_spotlight = attr_have_arealight = FALSE;
    for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
    {
        if ( ! elmt->the_instance->o_parent )
            continue;

        if ( elmt->the_instance->o_parent->b_class == light_obj ||
             elmt->the_instance->o_parent->b_class == dirlight_obj ||
             ( elmt->the_instance->o_parent->b_class == aggregate_obj &&
               ( Aggregate_Contains_Pointlight(elmt->the_instance->o_parent,
                                               FALSE) ||
                 Aggregate_Contains_Dirlight(elmt->the_instance->o_parent,
                                             FALSE) ) ) )
            attr_have_light = TRUE;
        else if ( elmt->the_instance->o_parent->b_class == spotlight_obj ||
                  ( elmt->the_instance->o_parent->b_class == aggregate_obj &&
                    Aggregate_Contains_Spotlight(elmt->the_instance->o_parent,
                                                 FALSE) ) )
            attr_have_spotlight = TRUE;
        else if ( elmt->the_instance->o_parent->b_class == arealight_obj  ||
                  ( elmt->the_instance->o_parent->b_class == aggregate_obj &&
                    Aggregate_Contains_Arealight(elmt->the_instance->o_parent,
                                                 FALSE) ) )
            attr_have_arealight = TRUE;
        else
            have_objs = TRUE;
    }

    if ( have_objs )
    {
        for ( elmt = window->selected_instances ;
              Obj_Is_Light(elmt->the_instance) ;
              elmt = elmt->next );

        Update_Attribute_Strings(elmt->the_instance->o_attribs);

        if ( elmt->the_instance->o_attribs->use_extension )
        {
            Target_Attributes_Callback();
            return;
        }

        Position_Shell(attributes_dialog_shell, FALSE);
        XtPopup(attributes_dialog_shell, XtGrabExclusive);
    }
    else if ( attr_have_light )
        Set_Light_Attributes(window->selected_instances,
                             attr_have_spotlight, attr_have_arealight);
    else if ( attr_have_spotlight )
        Set_Spotlight_Attributes(window->selected_instances,
                                 attr_have_arealight);
    else if ( attr_have_arealight )
        Set_Arealight_Attributes(window->selected_instances);
}


void
Attributes_Set_Defaults_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    attr_doing_defaults = TRUE;

    if ( ! attributes_dialog_shell )
        Create_Attributes_Dialog();

    Update_Attribute_Strings(&sced_preferences.default_attributes);
    Position_Shell(attributes_dialog_shell, FALSE);
    XtPopup(attributes_dialog_shell, XtGrabExclusive);
}


void
Attributes_Set_Defaults(AttributePtr defs, int flags)
{
    if ( flags & ModSimple )
    {
        sced_preferences.default_attributes.color.red = defs->color.red;
        sced_preferences.default_attributes.color.green = defs->color.green;
        sced_preferences.default_attributes.color.blue = defs->color.blue;
        sced_preferences.default_attributes.diff_coef = defs->diff_coef;
        sced_preferences.default_attributes.spec_coef = defs->spec_coef;
        sced_preferences.default_attributes.spec_power = defs->spec_power;
        sced_preferences.default_attributes.reflect_coef = defs->reflect_coef;
        sced_preferences.default_attributes.refract_index = defs->refract_index;
    }

    if ( flags & ModLight )
    {
        sced_preferences.default_attributes.intensity.red = defs->intensity.red;
        sced_preferences.default_attributes.intensity.green =
                                                        defs->intensity.green;
        sced_preferences.default_attributes.intensity.blue =
                                                        defs->intensity.blue;
    }

    if ( flags & ModSpotlight )
    {
        sced_preferences.default_attributes.radius = defs->radius;
        sced_preferences.default_attributes.tightness = defs->tightness;
        sced_preferences.default_attributes.invert = defs->invert;
    }

    if ( flags & ModArealight )
    {
        sced_preferences.default_attributes.samples = defs->samples;
        sced_preferences.default_attributes.jitter = defs->jitter;
    }
}


static void
Target_Attributes_Callback()
{
    if ( attr_doing_defaults )
        return;

    switch ( target_renderer )
    {
        case POVray:
        case Rayshade:
        case Radiance:
        case Renderman:
        case VRML:
            Specific_Attributes_Callback();
            break;

        default:;
    }
}



/*  void
**  Create_Attributes_Dialog()
**  Creates the attributes dialog box.
*/
static void
Create_Attributes_Dialog()
{
    Dimension   label_height;
    Dimension   max_width;
    Widget  top_label;
    Widget  done_button;
    Widget  cancel_button;
    Widget  default_button;
    Widget  labels[11];
    Widget  rgb_labels[3];
    Arg     args[15];
    int     count, rgb_count;
    int     m, n;

    attributes_dialog_shell = XtCreatePopupShell("Attributes",
                        transientShellWidgetClass, main_window.shell, NULL, 0);

    n = 0;
    attributes_form = XtCreateManagedWidget("attributesForm", formWidgetClass,
                    attributes_dialog_shell, args, n);

    /* Add the label at the top. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Object Attributes");   n++;
    XtSetArg(args[n], XtNtop, XtChainTop);              n++;
    XtSetArg(args[n], XtNbottom,XtChainTop);            n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    top_label = XtCreateManagedWidget("attributesLabel", labelWidgetClass,
                attributes_form, args, n);

    /* Need the size of the labels to get the string size right. */
    n = 0;
    XtSetArg(args[n], XtNheight, &label_height);    n++;
    XtGetValues(top_label, args, n);

    /* Common args for all the labels. */
    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;
    m = n;

    count = 0;
    /* The label for the colour: */
    XtSetArg(args[n], XtNlabel, "Colour");      n++;
    XtSetArg(args[n], XtNfromVert, top_label);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);       n++;
    labels[count] = XtCreateManagedWidget("colourLabel", labelWidgetClass,
                    attributes_form, args, n);
    count++;

    /* The label for the red. */
    rgb_count = 0;
    n = 5;
    XtSetArg(args[n], XtNfromVert, top_label);          n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count - 1]); n++;
    XtSetArg(args[n], XtNlabel, "Red");                 n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    rgb_labels[rgb_count] = XtCreateManagedWidget("redLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, red_text);         n++;
    XtSetArg(args[n], XtNfromVert, top_label);      n++;
    XtSetArg(args[n], XtNfromHoriz, rgb_labels[rgb_count]); n++;
/*    XtSetArg(args[n], XtNvalue, "");                n++; */
    rgb_text[rgb_count] = XtCreateManagedWidget("redText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(rgb_text[rgb_count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    rgb_count++;

    /* The label for the green. */
    n = 5;
    XtSetArg(args[n], XtNfromVert, top_label);          n++;
    XtSetArg(args[n], XtNfromHoriz, rgb_text[rgb_count - 1]);   n++;
    XtSetArg(args[n], XtNlabel, "Green");               n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    rgb_labels[1] = XtCreateManagedWidget("greenLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, green_text);       n++;
    XtSetArg(args[n], XtNfromVert, top_label);      n++;
    XtSetArg(args[n], XtNfromHoriz, rgb_labels[rgb_count]); n++;
    rgb_text[rgb_count] = XtCreateManagedWidget("greenText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(rgb_text[rgb_count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    rgb_count++;

    /* The label for the blue. */
    n = 5;
    XtSetArg(args[n], XtNfromVert, top_label);          n++;
    XtSetArg(args[n], XtNfromHoriz, rgb_text[rgb_count - 1]);   n++;
    XtSetArg(args[n], XtNlabel, "Blue");                n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    rgb_labels[2] = XtCreateManagedWidget("blueLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, blue_text);        n++;
    XtSetArg(args[n], XtNfromVert, top_label);      n++;
    XtSetArg(args[n], XtNfromHoriz, rgb_labels[rgb_count]); n++;
    rgb_text[rgb_count] = XtCreateManagedWidget("blueText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(rgb_text[rgb_count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    rgb_count++;

    /* The label for the diffuse value. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Diffuse Coefficient");     n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);      n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    labels[count] = XtCreateManagedWidget("diffuseLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);             n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);            n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);          n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);           n++;
    XtSetArg(args[n], XtNstring, diffuse_text);             n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);      n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);         n++;
    attribute_text[count] = XtCreateManagedWidget("diffuseText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(attribute_text[count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    /* The label for the specular value. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Specular Coef");       n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("specularLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, specular_text);    n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]); n++;
    attribute_text[count] = XtCreateManagedWidget("specularText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(attribute_text[count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    /* The specular power label and text.*/
    n = 5;
    XtSetArg(args[n], XtNlabel, "Power");   n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 2]);      n++;
    XtSetArg(args[n], XtNfromHoriz, attribute_text[count-1]);   n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    labels[count] = XtCreateManagedWidget("specPowerLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, spec_power_text);  n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 2]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]); n++;
    attribute_text[count] = XtCreateManagedWidget("specPowerText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(attribute_text[count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;



    /* The label for the reflection value. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Reflection");          n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 2]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("reflectLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, reflect_text);     n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 2]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]); n++;
    attribute_text[count] = XtCreateManagedWidget("reflectText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(attribute_text[count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    /* The label for the transparency value. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Transparency");        n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("transpLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, transparency_text);n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]); n++;
    attribute_text[count] = XtCreateManagedWidget("transparencyText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(attribute_text[count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    /* The label for the refraction value. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Refraction");          n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("refractLabel", labelWidgetClass,
                    attributes_form, args, n);
    /* The text. */
    n = 5;
    XtSetArg(args[n], XtNheight, label_height);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, refract_text);     n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]); n++;
    attribute_text[count] = XtCreateManagedWidget("refractText",
                    asciiTextWidgetClass, attributes_form, args, n);
    XtOverrideTranslations(attribute_text[count],
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    /* The buttons at the bottom. */

    /* Target specific attributes. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Target Specific");     n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    target_button = XtCreateManagedWidget("targetButton",
                    commandWidgetClass, attributes_form, args, n);
    XtAddCallback(target_button, XtNcallback, Apply_Attributes_Callback,
                  (XtPointer)TRUE);

    /* Done button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Done");            n++;
    XtSetArg(args[n], XtNfromVert, target_button);  n++;
    done_button = XtCreateManagedWidget("doneButton",
                    commandWidgetClass, attributes_form, args, n);
    XtAddCallback(done_button, XtNcallback, Apply_Attributes_Callback, NULL);

    /* None. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "None");            n++;
    XtSetArg(args[n], XtNfromVert, target_button);  n++;
    XtSetArg(args[n], XtNfromHoriz, done_button);   n++;
    none_button = XtCreateManagedWidget("noneButton",
                        toggleWidgetClass, attributes_form, args, n);
    XtAddCallback(none_button, XtNcallback, No_Attributes_Callback, NULL);

    /* Default. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Default");         n++;
    XtSetArg(args[n], XtNfromVert, target_button);  n++;
    XtSetArg(args[n], XtNfromHoriz, none_button);   n++;
    default_button = XtCreateManagedWidget("defaultButton",
                        commandWidgetClass, attributes_form, args, n);
    XtAddCallback(default_button, XtNcallback, Default_Attributes_Callback,
                    NULL);

    /* Cancel button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Cancel");              n++;
    XtSetArg(args[n], XtNfromVert, target_button);      n++;
    XtSetArg(args[n], XtNfromHoriz, default_button);    n++;
    cancel_button = XtCreateManagedWidget("cancelButton",
                    commandWidgetClass, attributes_form, args, n);
    XtAddCallback(cancel_button, XtNcallback, Attributes_Cancel_Callback,
                  (XtPointer)attributes_dialog_shell);

    /* Set all the widths to the same value. */
    max_width = Match_Widths(labels, count);

    n = 0;
    XtSetArg(args[n], XtNwidth, max_width); n++;
    XtSetValues(done_button, args, n);
    XtSetValues(default_button, args, n);
    XtSetValues(cancel_button, args, n);
    XtSetValues(none_button, args, n);

    XtRealizeWidget(attributes_dialog_shell);
}


static void
Attributes_Cancel_Callback(Widget w, XtPointer a, XtPointer b)
{
    XtPopdown((Widget)a);
}


/*  void
**  Update_Attribute_Strings()
**  Resets the strings displayed by the dialog box.  Redraws them too.
*/
static void
Update_Attribute_Strings(AttributePtr new_vals)
{
    XawTextBlock    text_block;
    int             old_length;
    int             count;

    /* For this to be called the relevant text widgets must have been
    ** created.  You can consider this a statement of fact or an
    ** excuse for crashing otherwise.
    */

    text_block.firstPos = 0;
    text_block.format = FMT8BIT;

    /* Colour. */
    old_length = strlen(red_text);
    sprintf(red_text, "%1.2g"HPFIX, new_vals->color.red);
    text_block.length = strlen(red_text);
    text_block.ptr = red_text;
    XawTextReplace(rgb_text[0], 0, old_length + 1, &text_block);

    old_length = strlen(green_text);
    sprintf(green_text, "%1.2g"HPFIX, new_vals->color.green);
    text_block.length = strlen(green_text);
    text_block.ptr = green_text;
    XawTextReplace(rgb_text[1], 0, old_length + 1, &text_block);

    old_length = strlen(blue_text);
    sprintf(blue_text, "%1.2g"HPFIX, new_vals->color.blue);
    text_block.length = strlen(blue_text);
    text_block.ptr = blue_text;
    XawTextReplace(rgb_text[2], 0, old_length + 1, &text_block);


    count = 1;
    /* diffuse. */
    old_length = strlen(diffuse_text);
    sprintf(diffuse_text, "%1.2g"HPFIX, new_vals->diff_coef);
    text_block.length = strlen(diffuse_text);
    text_block.ptr = diffuse_text;
    XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

    /* Specular. */
    old_length = strlen(specular_text);
    sprintf(specular_text, "%1.2g"HPFIX, new_vals->spec_coef);
    text_block.length = strlen(specular_text);
    text_block.ptr = specular_text;
    XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

    old_length = strlen(spec_power_text);
    sprintf(spec_power_text, "%1.2g"HPFIX, new_vals->spec_power);
    text_block.length = strlen(spec_power_text);
    text_block.ptr = spec_power_text;
    XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

    /* Reflection. */
    old_length = strlen(reflect_text);
    sprintf(reflect_text, "%1.2g"HPFIX, new_vals->reflect_coef);
    text_block.length = strlen(reflect_text);
    text_block.ptr = reflect_text;
    XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

    /* Transparency. */
    old_length = strlen(transparency_text);
    sprintf(transparency_text, "%1.2g"HPFIX, new_vals->transparency);
    text_block.length = strlen(transparency_text);
    text_block.ptr = transparency_text;
    XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

    /* Refraction. */
    old_length = strlen(refract_text);
    sprintf(refract_text, "%1.2g"HPFIX, new_vals->refract_index);
    text_block.length = strlen(refract_text);
    text_block.ptr = refract_text;
    XawTextReplace(attribute_text[count++], 0, old_length + 1, &text_block);

    /* Set the None toggle. */
    XtVaSetValues(none_button, XtNstate, ! new_vals->defined, NULL);
    No_Attributes_Callback(NULL, NULL, (XtPointer)(! new_vals->defined));
}


/*  void
**  Default_Attributes_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**  The callback invoked to set the attributes to their default.
*/
static void
Default_Attributes_Callback(Widget w, XtPointer a, XtPointer b)
{
    Renderer    i;
    Attributes  new_vals;

    new_vals.defined = TRUE;
    new_vals.color.red = sced_preferences.default_attributes.color.red;
    new_vals.color.green = sced_preferences.default_attributes.color.green;
    new_vals.color.blue = sced_preferences.default_attributes.color.blue;
    new_vals.diff_coef = sced_preferences.default_attributes.diff_coef;
    new_vals.spec_coef = sced_preferences.default_attributes.spec_coef;
    new_vals.spec_power = sced_preferences.default_attributes.spec_power;
    new_vals.reflect_coef = sced_preferences.default_attributes.reflect_coef;
    new_vals.refract_index = sced_preferences.default_attributes.refract_index;
    new_vals.transparency = sced_preferences.default_attributes.transparency;
    new_vals.use_extension = FALSE;
    for ( i = NoTarget ; i < LastTarget ; i++ )
        new_vals.extension[i] = NULL;
    new_vals.use_obj_trans = FALSE;
    new_vals.open = FALSE;

    Update_Attribute_Strings(&new_vals);
}


static void
Apply_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;
    Attributes      new_vals;
    float           temp_d;

    if ( cl )
    {
        if ( attr_doing_defaults )
            return;

        switch ( target_renderer )
        {
            case None:
                Popup_Error("No Target renderer.", main_window.shell, "Error");
                return;

            default:;
        }
    }

    XtPopdown(attributes_dialog_shell);

    XtVaGetValues(none_button, XtNstate, &(new_vals.defined), NULL);
    new_vals.defined = ! new_vals.defined;

    /* Need to parse all the strings. */
    sscanf(red_text, "%f", &temp_d);
    if ( temp_d > 1.0 ) temp_d = 1.0;
    if ( temp_d < 0.0 ) temp_d = 0.0;
    new_vals.color.red = temp_d;
    sscanf(green_text, "%f", &temp_d);
    if ( temp_d > 1.0 ) temp_d = 1.0;
    if ( temp_d < 0.0 ) temp_d = 0.0;
    new_vals.color.green = temp_d;
    sscanf(blue_text, "%f", &temp_d);
    if ( temp_d > 1.0 ) temp_d = 1.0;
    if ( temp_d < 0.0 ) temp_d = 0.0;
    new_vals.color.blue = temp_d;

    sscanf(diffuse_text, "%f", &(new_vals.diff_coef));
    if ( new_vals.diff_coef > 1.0 ) new_vals.diff_coef = 1.0;
    if ( new_vals.diff_coef < 0.0 ) new_vals.diff_coef = 0.0;
    sscanf(specular_text, "%f", &(new_vals.spec_coef));
    if ( new_vals.spec_coef > 1.0 ) new_vals.spec_coef = 1.0;
    if ( new_vals.spec_coef < 0.0 ) new_vals.spec_coef = 0.0;
    sscanf(spec_power_text, "%f", &(new_vals.spec_power));
    sscanf(reflect_text, "%f", &(new_vals.reflect_coef));
    if ( new_vals.reflect_coef > 1.0 ) new_vals.reflect_coef = 1.0;
    if ( new_vals.reflect_coef < 0.0 ) new_vals.reflect_coef = 0.0;
    sscanf(transparency_text, "%f", &(new_vals.transparency));
    if ( new_vals.transparency > 1.0 ) new_vals.transparency = 1.0;
    if ( new_vals.transparency < 0.0 ) new_vals.transparency = 0.0;
    sscanf(refract_text, "%f", &(new_vals.refract_index));

    new_vals.use_extension = FALSE;

    if ( attr_doing_defaults )
    {
        Attributes_Set_Defaults(&new_vals, ModSimple);
        return;
    }

    for ( elmt = window->selected_instances ;
          elmt != NULL ;
          elmt = elmt->next )
    {
        if ( ! Obj_Is_Light(elmt->the_instance) )
            Modify_Instance_Attributes(elmt->the_instance, &new_vals,ModSimple);
    }

    if ( cl )
    {
        Target_Attributes_Callback();
        return;
    }

    if ( attr_have_light )
        Set_Light_Attributes(window->selected_instances,
                             attr_have_spotlight, attr_have_arealight);
    else if ( attr_have_spotlight )
        Set_Spotlight_Attributes(window->selected_instances,
                                 attr_have_arealight);
    else if ( attr_have_arealight )
        Set_Arealight_Attributes(window->selected_instances);
}


void
Attributes_Change_String(InstanceList insts, char *new_str, Boolean transform,
                         Boolean open)
{
    Attributes      new_attr;
    InstanceList    elmt;

    for ( elmt = insts ; elmt != NULL ; elmt = elmt->next )
    {
        if ( ! Obj_Is_Light(elmt->the_instance) )
        {
            new_attr.defined = TRUE;
            new_attr.use_extension = TRUE;
            new_attr.extension[target_renderer] = Strdup(new_str);
            new_attr.use_obj_trans = transform;
            new_attr.open = open;
            Modify_Instance_Attributes(elmt->the_instance, &new_attr,ModExtend);
        }
    }
}

static void
No_Attributes_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;
    Boolean         result = ( ca ? TRUE : FALSE );
    int             i;

    if ( attr_doing_defaults )
        return;

    for ( elmt = window->selected_instances ;
          elmt != NULL ;
          elmt = elmt->next )
        if ( ! Obj_Is_Light(elmt->the_instance) )
            elmt->the_instance->o_attribs->defined = ! result;

    for ( i = 1 ; i < NUM_TEXT_STRINGS ; i++ )
        XtSetSensitive(attribute_text[i], ! result );
    for ( i = 0 ; i < 3 ; i++ )
        XtSetSensitive(rgb_text[i], ! result );
    XtSetSensitive(target_button, ! result );
}



static void
Declares_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ! declare_shell )
        Create_Declare_Dialog();

    /* Set the string. */
    if ( declarations[target_renderer] )
        XtVaSetValues(declare_text, XtNstring, declarations[target_renderer],
                      NULL);
    else
        XtVaSetValues(declare_text, XtNstring, "", NULL);

    Position_Shell(declare_shell, TRUE);

    XtPopup(declare_shell, XtGrabNonexclusive);
}



/*  Changes the declarations text for the specified target.
*/
void
Change_Declarations(Renderer target, char *new_stuff, Boolean append)
{
    if ( ! new_stuff )
    {
        if ( ! append )
            Clear_Declarations(target);
        return;
    }

    if ( append && declarations[target] )
    {
        declarations[target] = More(declarations[target], char,
                                    strlen(new_stuff) +
                                    strlen(declarations[target]) + 4);
        strcat(declarations[target], new_stuff);
    }
    else
    {
        declarations[target] = More(declarations[target], char,
                                    strlen(new_stuff) + 4);
        strcpy(declarations[target], new_stuff);
    }

}


/*  Clears all the include files.
*/
void
Clear_Declarations(Renderer target)
{
    if ( declarations[target] )
        free(declarations[target]);
    declarations[target] = NULL;
}


/*  Saves includes.
*/
int
Save_Declarations(FILE *outfile)
{
    Renderer    i;

    fprintf(outfile, "Declare\n");

    for ( i = NoTarget + 1 ; i < LastTarget ; i++ )
    {
        if ( ! declarations[i] )
            Save_String(outfile, "");
        else
            Save_String(outfile, declarations[i]);
    }

    return fprintf(outfile, "\n");
}


static void
Add_Declare_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    char    *temp_string;

    XtPopdown(declare_shell);

    XtVaGetValues(declare_text, XtNstring, &temp_string, NULL);
    Change_Declarations(target_renderer, temp_string, FALSE);
}


static void
Create_Declare_Dialog()
{
    Arg     args[15];
    int     n, m;
    Widget  form;
    Widget  label;
    Widget  done, cancel;
    Dimension   height;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Declarations");    n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    declare_shell = XtCreatePopupShell("povDeclareShell",
                        topLevelShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("povDeclareForm", formWidgetClass,
                        declare_shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;
    XtSetArg(args[n], XtNlabel, "Declarations:");   n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    label = XtCreateManagedWidget("povDeclareExistingLabel",
                        labelWidgetClass, form, args, n);

    XtVaGetValues(label, XtNheight, &height, NULL);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);          n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNright, XtChainRight);      n++;
    XtSetArg(args[n], XtNresizable, TRUE);          n++;
    XtSetArg(args[n], XtNheight, (int)height * 10); n++;
    XtSetArg(args[n], XtNstring, "");               n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);               n++;
    XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);  n++;
    XtSetArg(args[n], XtNfromVert, label);          n++;
    declare_text = XtCreateManagedWidget("povDeclareExistingText",
                        asciiTextWidgetClass, form, args, n);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainBottom);       m++;
    XtSetArg(args[m], XtNbottom, XtChainBottom);    m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);        m++;
    XtSetArg(args[m], XtNright, XtChainLeft);       m++;
    XtSetArg(args[m], XtNresizable, TRUE);          m++;

    n = m;
    XtSetArg(args[n], XtNlabel, "Done");                n++;
    XtSetArg(args[n], XtNfromVert, declare_text);   n++;
    done = XtCreateManagedWidget("povDeclareDoneButton", commandWidgetClass,
                                form, args, n);
    XtAddCallback(done, XtNcallback, Add_Declare_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");              n++;
    XtSetArg(args[n], XtNfromVert, declare_text);   n++;
    XtSetArg(args[n], XtNfromHoriz, done);              n++;
    cancel = XtCreateManagedWidget("povDeclareCancelButton", commandWidgetClass,
                                form, args, n);
    XtAddCallback(cancel, XtNcallback, Attributes_Cancel_Callback,
                    (XtPointer)declare_shell);

    XtRealizeWidget(declare_shell);
}


void
Set_Specific_Attributes_Initial_Text(char *text, Boolean transform,Boolean open)
{
    if ( ! specific_attributes_shell )
        Create_Specific_Attributes_Dialog();
    
    if ( ! text )
        text = "";

    XtVaSetValues(specific_attributes_text, XtNstring, text, NULL);
    XtVaSetValues(specific_transform_toggle, XtNstate, transform, NULL);
    XtVaSetValues(specific_open_toggle, XtNstate, open, NULL);
}


void
Specific_Attributes_Callback()
{
    InstanceList    elmt;

    if ( ! specific_attributes_shell )
        Create_Specific_Attributes_Dialog();

    for ( elmt = window->selected_instances ;
          Obj_Is_Light(elmt->the_instance) ;
          elmt = elmt->next );

    if ( target_renderer == VRML && window == &main_window )
        XtSetSensitive(specific_lod_button, TRUE);
    else
        XtSetSensitive(specific_lod_button, FALSE);

    Set_Specific_Attributes_Initial_Text(
      elmt->the_instance->o_attribs->extension[target_renderer],
      elmt->the_instance->o_attribs->use_obj_trans,
      elmt->the_instance->o_attribs->open);

    Position_Shell(specific_attributes_shell, FALSE);
    XtPopup(specific_attributes_shell, XtGrabExclusive);
}


static void
Specific_Attributes_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    char    *new_attribs;
    Boolean transform, open;

    XtPopdown(specific_attributes_shell);

    XtVaGetValues(specific_attributes_text, XtNstring, &new_attribs, NULL);
    XtVaGetValues(specific_transform_toggle, XtNstate, &transform, NULL);
    XtVaGetValues(specific_open_toggle, XtNstate, &open, NULL);
    Attributes_Change_String(window->selected_instances, new_attribs,
                             transform, open);

    if ( attr_have_light )
        Set_Light_Attributes(window->selected_instances, attr_have_spotlight,
                             attr_have_arealight);
    else if ( attr_have_spotlight )
        Set_Spotlight_Attributes(window->selected_instances,
                                 attr_have_arealight);
    else if ( attr_have_arealight )
        Set_Arealight_Attributes(window->selected_instances);
}


static void
Attributes_Simple_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;
    char    *new_attribs;
    Boolean transform, open;

    XtPopdown((Widget)cl);

    XtVaGetValues(specific_attributes_text, XtNstring, &new_attribs, NULL);
    XtVaGetValues(specific_transform_toggle, XtNstate, &transform, NULL);
    XtVaGetValues(specific_open_toggle, XtNstate, &open, NULL);
    Attributes_Change_String(window->selected_instances, new_attribs,
                             transform, open);

    for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
    {
        if ( ! Obj_Is_Light(elmt->the_instance ) )
          elmt->the_instance->o_attribs->use_extension = FALSE;
    }

    Set_Attributes_Callback(NULL, NULL, NULL);
}

static void
Create_Specific_Attributes_Dialog()
{
    Arg         args[15];
    int         n, m;
    Widget      form;
    Widget      label;
    Widget      done, cancel, simplified;
    Widget      declare_widget;
    Dimension   height;
    String      shell_geometry;
    unsigned    shell_width, shell_height;
    int         gap;
    int         junk;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Specific Attributes");     n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);           n++;
    specific_attributes_shell = XtCreatePopupShell("specAttributesShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    XtVaGetValues(specific_attributes_shell, XtNgeometry, &shell_geometry,NULL);
    XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

    n = 0;
    form = XtCreateManagedWidget("specAttributesForm", formWidgetClass,
                                    specific_attributes_shell, args, n);

    XtVaGetValues(form, XtNdefaultDistance, &gap, NULL);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);              n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);           n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainLeft);           n++;
    XtSetArg(args[n], XtNresizable, TRUE);              n++;
    XtSetArg(args[n], XtNlabel, "Texture String:");     n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    label = XtCreateManagedWidget("specAttributesLabel",
                                    labelWidgetClass, form, args, n);

    XtVaGetValues(label, XtNheight, &height, NULL);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);          n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNright, XtChainRight);      n++;
    XtSetArg(args[n], XtNresizable, TRUE);          n++;
    XtSetArg(args[n], XtNwidth, (int)shell_height - 2 - 2  * (int)gap); n++;
    XtSetArg(args[n], XtNheight,
             (int)shell_height - (int)height * 4 - 8 - 6 * (int)gap);   n++;
    XtSetArg(args[n], XtNstring, "");               n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNresize, TRUE);             n++;
    XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);  n++;
    XtSetArg(args[n], XtNfromVert, label);          n++;
    specific_attributes_text = XtCreateManagedWidget("specAttributesText",
                        asciiTextWidgetClass, form, args, n);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainBottom);       m++;
    XtSetArg(args[m], XtNbottom, XtChainBottom);    m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);        m++;
    XtSetArg(args[m], XtNright, XtChainLeft);       m++;
    XtSetArg(args[m], XtNresizable, TRUE);          m++;

    n = m;
    XtSetArg(args[n], XtNlabel, "Transform Texture");           n++;
    XtSetArg(args[n], XtNfromVert, specific_attributes_text);   n++;
    specific_transform_toggle = XtCreateManagedWidget(
        "specAttributesTransformToggle", toggleWidgetClass, form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Open");                    n++;
    XtSetArg(args[n], XtNfromVert, specific_attributes_text);   n++;
    XtSetArg(args[n], XtNfromHoriz, specific_transform_toggle); n++;
    specific_open_toggle = XtCreateManagedWidget("specAttributesOpenToggle",
                        toggleWidgetClass, form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Declarations");                n++;
    XtSetArg(args[n], XtNfromVert, specific_transform_toggle);  n++;
    declare_widget = XtCreateManagedWidget("specAttributesDeclareButton",
                        commandWidgetClass, form, args, n);
    XtAddCallback(declare_widget, XtNcallback, Declares_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "LOD");                         n++;
    XtSetArg(args[n], XtNfromVert, specific_transform_toggle);  n++;
    XtSetArg(args[n], XtNfromHoriz, declare_widget);            n++;
    specific_lod_button = XtCreateManagedWidget("specAttributesLODButton",
                        commandWidgetClass, form, args, n);
    XtAddCallback(specific_lod_button, XtNcallback, LOD_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Done");            n++;
    XtSetArg(args[n], XtNfromVert, declare_widget); n++;
    done = XtCreateManagedWidget("specAttributesDoneButton", commandWidgetClass,
                                    form, args, n);
    XtAddCallback(done, XtNcallback, Specific_Attributes_Done_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Simplified");      n++;
    XtSetArg(args[n], XtNfromVert, declare_widget); n++;
    XtSetArg(args[n], XtNfromHoriz, done);          n++;
    simplified = XtCreateManagedWidget("specAttributesSimpleButton",
                    commandWidgetClass, form, args, n);
    XtAddCallback(simplified, XtNcallback, Attributes_Simple_Callback,
                  (XtPointer)specific_attributes_shell);
    
    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNfromVert, declare_widget); n++;
    XtSetArg(args[n], XtNfromHoriz, simplified);    n++;
    cancel = XtCreateManagedWidget("specAttributesCancelButton",
                                commandWidgetClass, form, args, n);
    XtAddCallback(cancel, XtNcallback, Attributes_Cancel_Callback,
                    (XtPointer)specific_attributes_shell);
    
    XtRealizeWidget(specific_attributes_shell);
}


AttributePtr
Attribute_New(AttributePtr dest, Boolean allocate)
{
    Renderer    rend;

    if ( allocate )
        dest = New(Attributes, 1);

    *dest = sced_preferences.default_attributes;
    for ( rend = NoTarget ; rend < LastTarget ; rend++ )
        dest->extension[rend] = NULL;
    dest->defined = FALSE;

    return dest;
}

void
Attribute_Copy(AttributePtr dest, AttributePtr src)
{
    Renderer    rend;

    *dest = *src;
    for ( rend = NoTarget ; rend < LastTarget ; rend++ )
        if ( src->extension[rend] )
            dest->extension[rend] = Strdup(src->extension[rend]);
}


void
Attribute_Destroy(AttributePtr victim)
{
    Renderer    rend;

    for ( rend = NoTarget ; rend < LastTarget ; rend++ )
        if ( victim->extension[rend] )
            free(victim->extension[rend]);
}


static void
LOD_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(lod_shell);
    XtDestroyWidget(lod_shell);
    free(lod_text);
}


static void
LOD_Clear_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;

    XtPopdown(lod_shell);
    XtDestroyWidget(lod_shell);
    free(lod_text);

    for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
        if ( elmt->the_instance->o_lods )
        {
            free(elmt->the_instance->o_lods->lods);
            free(elmt->the_instance->o_lods);
            elmt->the_instance->o_lods = NULL;
        }
}


static void
LOD_Set_Distances(InstanceList insts, float *dists, int num)
{
    InstanceList    elmt;
    int             i;

    for ( elmt = insts ; elmt ; elmt = elmt->next )
    {
        if ( elmt->the_instance->o_lods )
            elmt->the_instance->o_lods->lods =
                More(elmt->the_instance->o_lods->lods, float, num);
        else
        {
            elmt->the_instance->o_lods = New(LODInfo, 1);
            elmt->the_instance->o_lods->lods = New(float, num);
        }
        elmt->the_instance->o_lods->num_lods = num;
        for ( i = 0 ; i < num ; i++ )
            elmt->the_instance->o_lods->lods[i] = dists[i];
    }
}

static void
LOD_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    char    *temp_s;
    float   *distances;
    int     i;

    XtPopdown(lod_shell);

    distances = New(float, lod_num);

    for ( i = 0 ; i < lod_num ; i++ )
    {
        XtVaGetValues(lod_text[i], XtNstring, &temp_s, NULL);
        sscanf(temp_s, "%g", distances + i);
    }

    LOD_Set_Distances(window->selected_instances, distances, lod_num);

    free(distances);
    XtDestroyWidget(lod_shell);
    free(lod_text);
}


static void
LOD_Create_Shell(int num_levels)
{
    Widget  form;
    Widget  label;
    char    label_text[32];
    Arg     args[10];
    int     n, m, i;
    Widget  done, clear, cancel;

    lod_num = num_levels;
    lod_text = New(Widget, lod_num);

    n = 0;
    XtSetArg(args[n], XtNtitle, "LOD");             n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    lod_shell = XtCreatePopupShell("lodShell", transientShellWidgetClass,
                                    main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("lodForm", formWidgetClass, lod_shell, args,n);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;

    n = m;
    XtSetArg(args[n], XtNlabel, "Density 0");   n++;
    XtSetArg(args[n], XtNborderWidth, 0);       n++;
    label = XtCreateManagedWidget("lodLabel", labelWidgetClass, form, args, n);

    for ( i = 0 ; i < lod_num ; i++ )
    {
        n = m;
        XtSetArg(args[n], XtNfromVert, label);              n++;
        XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
        XtSetArg(args[n], XtNresize, XawtextResizeWidth);   n++;
        lod_text[i] = XtCreateManagedWidget("lodText", asciiTextWidgetClass,
                                            form, args, n);
        XtOverrideTranslations(lod_text[i],
            XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

        n = m;
        sprintf(label_text, "Density %d", i + 1);
        XtSetArg(args[n], XtNlabel, label_text);        n++;
        XtSetArg(args[n], XtNborderWidth, 0);           n++;
        XtSetArg(args[n], XtNfromVert, lod_text[i]);    n++;
        label = XtCreateManagedWidget("lodLabel", labelWidgetClass, form,
                                      args, n);
    }

    n = m;
    XtSetArg(args[n], XtNlabel, "Done");    n++;
    XtSetArg(args[n], XtNfromVert, label);  n++;
    done = XtCreateManagedWidget("lodDone", commandWidgetClass, form, args, n);
    XtAddCallback(done, XtNcallback, LOD_Done_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Clear");   n++;
    XtSetArg(args[n], XtNfromVert, label);  n++;
    XtSetArg(args[n], XtNfromHoriz, done);  n++;
    clear = XtCreateManagedWidget("lodClear", commandWidgetClass, form, args,n);
    XtAddCallback(clear, XtNcallback, LOD_Clear_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");  n++;
    XtSetArg(args[n], XtNfromVert, label);  n++;
    XtSetArg(args[n], XtNfromHoriz, clear); n++;
    cancel = XtCreateManagedWidget("lodCancel", commandWidgetClass, form,
                                   args, n);
    XtAddCallback(cancel, XtNcallback, LOD_Cancel_Callback, NULL);

    XtRealizeWidget(lod_shell);
}


static void
LOD_Set_Densities(LODInfoPtr lod)
{
    char    str[24];
    int     i;

    if ( ! lod )
    {
        for ( i = 0 ; i < lod_num ; i++ )
            XtVaSetValues(lod_text[i], XtNstring, "1000", NULL);
        return;
    }

    for ( i = 0 ; i < lod->num_lods ; i++ )
    {
        sprintf(str, "%0.3g", lod->lods[i]);
        XtVaSetValues(lod_text[i], XtNstring, str, NULL);
    }

    for ( ; i < lod_num ; i++ )
    {
        sprintf(str, "%0.3g", lod->lods[lod->num_lods - 1]);
        XtVaSetValues(lod_text[i], XtNstring, str, NULL);
    }
}


static void
LOD_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt;
    int         max_level = 0;
    int         level;
    LODInfoPtr  max_lod = NULL;

    for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
    {
        level = Wireframe_Density_Level(elmt->the_instance);
        if ( level > max_level )    max_level = level;
        if ( elmt->the_instance->o_lods )
        {
            if ( max_lod )
            {
                if ( elmt->the_instance->o_lods->num_lods >
                max_lod->num_lods )
                    max_lod = elmt->the_instance->o_lods;
            }
            else
                max_lod = elmt->the_instance->o_lods;
        }
    }

    if ( ! max_level )
    {
        Popup_Error("No Dense Wireframes for LOD", main_window.shell, "Error");
        return;
    }

    LOD_Create_Shell(max_level);

    LOD_Set_Densities(max_lod);

    Position_Shell(lod_shell, TRUE);
    XtPopup(lod_shell, XtGrabExclusive);
}


