/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/camera.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: camera.c,v $
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
**  camera.c : Camera definition, saving and loading functions.
**
**  Created: 01/05/94
**
*/

#include <math.h>
#include <sced.h>
#include <edge_table.h>
#include <edit.h>
#include <gen_wireframe.h>
#include <instance_list.h>
#include <quaternion.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Toggle.h>
#include <View.h>



/* Define the global camera here.  It starts out undefined. */
Camera  camera = { FALSE,
                   {0, 0, 1},
                   {0, 0, -1},
                   {1, 0, 0},
                   90.0, 90.0, 1.0,
                   0, 0 };

ObjectInstance  camera_object;      /* The camera object in the scene. */

static void Update_Camera_Strings(Camera* new_vals, Viewport *vp);
static void Viewport_To_Camera_Callback(Widget, XtPointer, XtPointer);

static void Create_Camera_Shell();

static void Cancel_Camera_Callback(Widget, XtPointer, XtPointer);
static void Apply_Camera_Callback(Widget, XtPointer, XtPointer);


static Widget   camera_dialog_shell = NULL;

#define MAX_TEXT_LENGTH 30
static char     location_text[MAX_TEXT_LENGTH];
static char     look_at_text[MAX_TEXT_LENGTH];
static char     look_up_text[MAX_TEXT_LENGTH];
static char     horiz_fov_text[MAX_TEXT_LENGTH];
static char     vert_fov_text[MAX_TEXT_LENGTH];
static char     horiz_dist_text[MAX_TEXT_LENGTH];
static char     vert_dist_text[MAX_TEXT_LENGTH];
static char     eye_dist_text[MAX_TEXT_LENGTH];

static Widget   location_widget;
static Widget   look_at_widget;
static Widget   look_up_widget;
static Widget   horiz_fov_widget;
static Widget   vert_fov_widget;
static Widget   horiz_dist_widget;
static Widget   vert_dist_widget;
static Widget   eye_dist_widget;
static Widget   use_fov_toggle;

static Widget   display_widget;

void
Define_Camera_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    if ( ! camera_dialog_shell )
        Create_Camera_Shell();

    Update_Camera_Strings(&camera, &(main_window.viewport));

    Position_Shell(camera_dialog_shell, FALSE);
    XtPopup(camera_dialog_shell, XtGrabExclusive);
}

static void
Camera_FOV_Toggle_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Boolean new_state = ( ca ? TRUE : FALSE );

    if ( new_state )
    {
        XtSetSensitive(horiz_fov_widget, TRUE);
        XtSetSensitive(vert_fov_widget, TRUE);
        XtSetSensitive(horiz_dist_widget, FALSE);
        XtSetSensitive(vert_dist_widget, FALSE);
    }
    else
    {
        XtSetSensitive(horiz_fov_widget, FALSE);
        XtSetSensitive(vert_fov_widget, FALSE);
        XtSetSensitive(horiz_dist_widget, TRUE);
        XtSetSensitive(vert_dist_widget, TRUE);
    }
}


static void
Create_Camera_Shell()
{
    Dimension   label_height;
    Widget      top_label;
    Widget      camera_form;
    Widget      viewport_button;
    Widget      done_button;
    Widget      cancel_button;
    Widget      labels[8];
    Arg         args[15];
    Dimension   max_width, width;
    int         count;
    int         m, n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Camera");  n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    camera_dialog_shell = XtCreatePopupShell("cameraShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    camera_form = XtCreateManagedWidget("cameraForm", formWidgetClass,
                    camera_dialog_shell, args, n);

    /* Add the label at the top. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Camera");      n++;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom,XtChainTop);    n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    top_label = XtCreateManagedWidget("cameraLabel", labelWidgetClass,
                                        camera_form, args, n);

    /* Common args for all the labels. */
    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;
    m = n;

    count = 0;
    /* The label for the location vector. */
    XtSetArg(args[n], XtNlabel, "location");        n++;
    XtSetArg(args[n], XtNfromVert, top_label);      n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    labels[count] = XtCreateManagedWidget("locationLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;

    /* The label for the look_at vector. */
    n = m;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNlabel, "look at");             n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("lookAtLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;

    /* The label for the up vector. */
    n = m;
    XtSetArg(args[n], XtNlabel, "up");                  n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("up", labelWidgetClass,
                    camera_form, args, n);
    count++;

    /* Labels for the fov values. */
    n = m;
    XtSetArg(args[n], XtNlabel, "horiz fov");           n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("horizFovLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "vert fov");            n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("vertFovLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;

    /* Labels for the dist values. */
    n = m;
    XtSetArg(args[n], XtNlabel, "horiz dist");          n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("horizDistLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;
    n = m;
    XtSetArg(args[n], XtNlabel, "vert dist");           n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("vertDistLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;

    /* The label for the focal vector. */
    n = m;
    XtSetArg(args[n], XtNlabel, "focal");               n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[count] = XtCreateManagedWidget("focalLabel", labelWidgetClass,
                    camera_form, args, n);
    count++;

    Match_Widths(labels, count);

    /* Need the height of the labels to get the string size right. */
    n = 0;
    XtSetArg(args[n], XtNheight, &label_height);    n++;
    XtGetValues(top_label, args, n);

    count = 0;
    /* All the strings now.  What joy. */
    n = 0;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainLeft);           n++;
    XtSetArg(args[n], XtNtop, XtChainTop);              n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);           n++;
    XtSetArg(args[n], XtNresizable, TRUE);              n++;
    XtSetArg(args[n], XtNheight, label_height);         n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, MAX_TEXT_LENGTH);      n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);   n++;
    m = n;

    XtSetArg(args[n], XtNstring, location_text);    n++;
    XtSetArg(args[n], XtNfromVert, top_label);      n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]); n++;
    location_widget = XtCreateManagedWidget("locationText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(location_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, look_at_text);         n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    look_at_widget = XtCreateManagedWidget("lookatText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(look_at_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, look_up_text);         n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    look_up_widget = XtCreateManagedWidget("lookupText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(look_up_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, horiz_fov_text);       n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    horiz_fov_widget = XtCreateManagedWidget("horizFovText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(horiz_fov_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, vert_fov_text);        n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    vert_fov_widget = XtCreateManagedWidget("vertFovText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(vert_fov_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, horiz_dist_text);      n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    horiz_dist_widget = XtCreateManagedWidget("horizDistText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(horiz_dist_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, vert_dist_text);       n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNsensitive, FALSE);             n++;
    vert_dist_widget = XtCreateManagedWidget("vertDistText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(vert_dist_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;

    n = m;
    XtSetArg(args[n], XtNstring, eye_dist_text);        n++;
    XtSetArg(args[n], XtNfromVert, labels[count - 1]);  n++;
    XtSetArg(args[n], XtNfromHoriz, labels[count]);     n++;
    XtSetArg(args[n], XtNsensitive, FALSE);             n++;
    eye_dist_widget = XtCreateManagedWidget("eyeDistText",
                    asciiTextWidgetClass, camera_form, args, n);
    XtOverrideTranslations(eye_dist_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));
    count++;


    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;
    m = n;

    /* The FOV / Dist toggle. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Use FOV");             n++;
    XtSetArg(args[n], XtNfromVert, eye_dist_widget);    n++;
    XtSetArg(args[n], XtNstate, TRUE);                  n++;
    use_fov_toggle = XtCreateManagedWidget("useFOVToggle",
                        toggleWidgetClass, camera_form, args, n);
    XtAddCallback(use_fov_toggle, XtNcallback, Camera_FOV_Toggle_Callback,NULL);

    /* The buttons at the bottom right. */
    /* Viewport->camera. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Viewport");            n++;
    XtSetArg(args[n], XtNfromVert, use_fov_toggle);     n++;
    viewport_button = XtCreateManagedWidget("viewportButton",
                        commandWidgetClass, camera_form, args, n);
    XtAddCallback(viewport_button, XtNcallback, Viewport_To_Camera_Callback,
                    NULL);

    /* Cancel button. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");              n++;
    XtSetArg(args[n], XtNfromVert, use_fov_toggle);     n++;
    XtSetArg(args[n], XtNfromHoriz, viewport_button);   n++;
    cancel_button = XtCreateManagedWidget("cancelButton",
                    commandWidgetClass, camera_form, args, n);
    XtAddCallback(cancel_button, XtNcallback, Cancel_Camera_Callback, NULL);

    /* Done button. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Done");            n++;
    XtSetArg(args[n], XtNfromVert, use_fov_toggle); n++;
    XtSetArg(args[n], XtNfromHoriz, cancel_button); n++;
    done_button = XtCreateManagedWidget("doneButton",
                    commandWidgetClass, camera_form, args, n);
    XtAddCallback(done_button, XtNcallback, Apply_Camera_Callback, NULL);

    /* Make them all the same width. */
    max_width = 0;
    XtSetArg(args[0], XtNwidth, &width);
    XtGetValues(viewport_button, args, 1);
    if ( width > max_width ) max_width = width;
    XtGetValues(done_button, args, 1);
    if ( width > max_width ) max_width = width;
    XtGetValues(cancel_button, args, 1);
    if ( width > max_width ) max_width = width;

    XtSetArg(args[0], XtNwidth, max_width);
    XtSetValues(viewport_button, args, 1);
    XtSetValues(done_button, args, 1);
    XtSetValues(cancel_button, args, 1);

    XtRealizeWidget(camera_dialog_shell);
}


static void
Cancel_Camera_Callback(Widget w, XtPointer a, XtPointer b)
{
    XtPopdown(camera_dialog_shell);
}


static void
Viewport_To_Camera_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    Camera  new_cam;

    new_cam.defined = TRUE;

    Viewport_To_Camera(&(main_window.viewport), main_window.view_widget,
                       &new_cam, FALSE);

    /* Update the strings if necessary. */
    Update_Camera_Strings(&new_cam, &(main_window.viewport));

    Camera_Set_Object_From_Camera(TRUE);
}

/*
**  Converts a known Viewport vp into a camera specification.
**  Finds all aspects of the camera, regardless of which raytracer is defined.
*/
void
Viewport_To_Camera(ViewportPtr vp, Widget widget, Camera *cam,
                   Boolean use_true_width)
{
    Dimension   width, height;
    Vector  max_pt;
    Vector  temp_v;
    Arg     args[5];
    int     n;

    /* Work out the location. */
    VScalarMul(vp->view_from, vp->view_distance + vp->eye_distance, temp_v);
    VAdd(temp_v, vp->view_at, cam->location);

    /* look_at has an equivalent. */
    cam->look_at = vp->view_at;

    /* look_up is also equiv. */
    cam->look_up = vp->view_up;

    /* horiz fov is a bit of a pain. */
    /* It is derived from the maximum point that will appear on the screen.
    */
    if ( ! use_true_width )
    {
        if ( ! ( width = vp->scr_width ) || ! (height = vp->scr_height ) )
        {
            n = 0;
            XtSetArg(args[n], XtNdesiredWidth, &width);     n++;
            XtSetArg(args[n], XtNdesiredHeight, &height);   n++;
            XtGetValues(widget, args, n);
        }
    }
    n = 0;
    if ( use_true_width || width == 0 || height == 0 )
    {
        XtSetArg(args[n], XtNwidth, &width);    n++;
        XtSetArg(args[n], XtNheight, &height);  n++;
    }
    XtGetValues(widget, args, n);

    /* Work out what the max point is (in view coords). */
    max_pt.x = ((double)width / 2 ) / vp->magnify;
    max_pt.y = ((double)height / 2 ) / vp->magnify;

    /* We now have the following triangle. */
    /*
    **      -max_pt.x             max_pt.x  _
    **          \                       /   |
    **           \                     /    |
    **            \                   /     |
    **             \                 /      |
    **              \               /       |
    **               \             /    eye_dist
    **                \           /         |
    **                 \ theta   /          |
    **                  \       /           |
    **                   \     /            |
    **                  location            -
    **
    */
    cam->horiz_fov = 360 * atan( max_pt.x / vp->eye_distance ) / M_PI;

    /* Vert fov comes from horiz fov and the aspect-ratio of the screen. */
    /* Actually I choose to compute it separately.  */
    cam->vert_fov = 360 * atan( max_pt.y / vp->eye_distance ) / M_PI;

    /* eye_dist is a copy. */
    cam->eye_dist = vp->eye_distance;

    /* Screen size is the current desired size. */
    cam->scr_width = width;
    cam->scr_height = height;

}



/*  void
**  Update_Camera_Strings()
**  Resets the strings displayed by the dialog box.  Redraws them too.
*/
static void
Update_Camera_Strings(Camera *new_vals, Viewport *vp)
{
    XawTextBlock    text_block;
    int             old_length;


    /* Set all the strings which are common to each raytracer. */
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;

    /* Location. */
    old_length = strlen(location_text);
    sprintf(location_text, "%1.3g %1.3g %1.3g",
            new_vals->location.x, new_vals->location.y, new_vals->location.z);
    text_block.length = strlen(location_text);
    text_block.ptr = location_text;
    XawTextReplace(location_widget, 0, old_length + 1, &text_block);

    /* Look at. */
    old_length = strlen(look_at_text);
    sprintf(look_at_text, "%1.3g %1.3g %1.3g",
            new_vals->look_at.x, new_vals->look_at.y, new_vals->look_at.z);
    text_block.length = strlen(look_at_text);
    text_block.ptr = look_at_text;
    XawTextReplace(look_at_widget, 0, old_length + 1, &text_block);

    /* Look up. */
    old_length = strlen(look_up_text);
    sprintf(look_up_text, "%1.3g %1.3g %1.3g",
            new_vals->look_up.x, new_vals->look_up.y, new_vals->look_up.z);
    text_block.length = strlen(look_up_text);
    text_block.ptr = look_up_text;
    XawTextReplace(look_up_widget, 0, old_length + 1, &text_block);

    /* Horiz FOV. */
    old_length = strlen(horiz_fov_text);
    sprintf(horiz_fov_text, "%1.3g"HPFIX, new_vals->horiz_fov);
    text_block.length = strlen(horiz_fov_text);
    text_block.ptr = horiz_fov_text;
    XawTextReplace(horiz_fov_widget, 0, old_length + 1, &text_block);

    /* Vert FOV. */
    old_length = strlen(vert_fov_text);
    sprintf(vert_fov_text, "%1.3g"HPFIX, new_vals->vert_fov);
    text_block.length = strlen(vert_fov_text);
    text_block.ptr = vert_fov_text;
    XawTextReplace(vert_fov_widget, 0, old_length + 1, &text_block);

    /* Horiz Dist. */
    old_length = strlen(horiz_dist_text);
    sprintf(horiz_dist_text, "%1.3g"HPFIX,
            new_vals->eye_dist * 2 * tan(new_vals->horiz_fov * M_PI / 360));
    text_block.length = strlen(horiz_dist_text);
    text_block.ptr = horiz_dist_text;
    XawTextReplace(horiz_dist_widget, 0, old_length + 1, &text_block);

    /* Vert Dist. */
    old_length = strlen(vert_dist_text);
    sprintf(vert_dist_text, "%1.3g"HPFIX,
            new_vals->eye_dist * 2 * tan(new_vals->vert_fov * M_PI / 360));
    text_block.length = strlen(vert_dist_text);
    text_block.ptr = vert_dist_text;
    XawTextReplace(vert_dist_widget, 0, old_length + 1, &text_block);

    /* Eye dist. */
    old_length = strlen(eye_dist_text);
    sprintf(eye_dist_text, "%1.3g"HPFIX, new_vals->eye_dist);
    text_block.length = strlen(eye_dist_text);
    text_block.ptr = eye_dist_text;
    XawTextReplace(eye_dist_widget, 0, old_length + 1, &text_block);
}



void
Camera_To_Viewport(Camera *src, ViewportPtr viewport)
{
    /* Set the view_from vector. */
    VSub(src->location, src->look_at, viewport->view_from);

    /* Set view_at and view_up. */
    viewport->view_at = src->look_at;
    viewport->view_up = src->look_up;

    /* view_distance is the length of the view_from vector - eye_dist. */
    viewport->view_distance = VMod(viewport->view_from) - src->eye_dist;
    viewport->eye_distance = src->eye_dist;

    if ( src->scr_width )
        viewport->magnify = (double)src->scr_width /
                        ( src->eye_dist * 2 * tan(src->horiz_fov * M_PI / 360));
    else
        viewport->magnify = (double)main_window.width /
                        ( src->eye_dist * 2 * tan(src->horiz_fov * M_PI / 360));

    Build_Viewport_Transformation(viewport);
}


/*  void
**  Camera_To_Window(WindowInfoPtr window)
**  Converts the window so that it is aligned with the camera.  It may also
**  adjust the image size to match that which was in force when the camera
**  was defined.
**
**  This is as good a place as any for a speil about image size.
**  Image size is stored with the camera.
**  Image size is set/modified as follows:
**  - Initially it is set to the window size as it is realized.
**  - When a file is loaded it is set to what the file says.
**  - When a camera is defined, it is reset accordingly.
**  - When it is explicitly set it is changed on the screen BUT NOT in
**      the camera structure.
**  - When a viewport is switched to the camera, the image size is reset
**      from what the camera says.
**
**  NOTE: It would be nice if the user had access to the image size when they
**      edit a camera.  But I'm not feeling nice.
*/
void
Camera_To_Window(WindowInfoPtr window)
{
    if ( ! camera.defined )
    {
        Popup_Error("No camera defined!", window->shell, "Error");
        return;
    }

    Camera_To_Viewport(&camera, &(window->viewport));

    window->magnify = window->viewport.magnify;

    /* Set the screen size to what the camera thinks. */
    XtVaSetValues(window->view_widget,
                /*
                XtNwidth, camera.scr_width,
                XtNheight, camera.scr_height,
                */
                XtNdesiredWidth, camera.scr_width,
                XtNdesiredHeight, camera.scr_height, NULL);

    /* Redraw the screen. */
    View_Update(window, window->all_instances, CalcView);
    Update_Projection_Extents(window->all_instances);

}


static void
Apply_Camera_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Vector  dir, cross;
    double  temp_d;
    Boolean use_fov;

    XtPopdown(camera_dialog_shell);

    camera.defined = TRUE;

    XtVaGetValues(main_window.view_widget,
                XtNdesiredWidth, &(camera.scr_width),
                XtNdesiredHeight, &(camera.scr_height), NULL);
    if ( camera.scr_width == 0 || camera.scr_height == 0 )
        XtVaGetValues(main_window.view_widget,
                    XtNwidth, &(camera.scr_width),
                    XtNheight, &(camera.scr_height), NULL);

    /* Need to parse all the strings. */
    /* The common ones first. */
    sscanf(location_text, "%lg %lg %lg", &(camera.location.x),
            &(camera.location.y), &(camera.location.z));
    sscanf(look_at_text, "%lg %lg %lg", &(camera.look_at.x),
            &(camera.look_at.y), &(camera.look_at.z));
    VSub(camera.location, camera.look_at, dir);
    if ( VZero(dir) )
    {
        camera.look_at.z -= 0.01;
        dir.z += 0.01;
    }

    sscanf(look_up_text, "%lg %lg %lg", &(camera.look_up.x),
            &(camera.look_up.y), &(camera.look_up.z));
    VCross(camera.look_up, dir, cross);
    if ( VZero(cross) )
    {
        VNew(0, 0, 1, camera.look_up);
        VCross(camera.look_up, dir, cross);
        if ( VZero(cross) )
            VNew(1,  0, 0, camera.look_up);
    }

    sscanf(eye_dist_text, "%f", &(camera.eye_dist));
    if ( IsZero(camera.eye_dist) )
        camera.eye_dist = 0.01;

    XtVaGetValues(use_fov_toggle, XtNstate, &use_fov, NULL);
    if ( use_fov )
    {
        sscanf(horiz_fov_text, "%f", &(camera.horiz_fov));
        sscanf(vert_fov_text, "%f", &(camera.vert_fov));
    }
    else
    {
        sscanf(horiz_dist_text, "%lf", &temp_d);
        camera.horiz_fov = 360 / M_PI * atan(temp_d * 0.5 / camera.eye_dist);

        sscanf(vert_dist_text, "%lf", &temp_d);
        camera.vert_fov = 360 / M_PI * atan(temp_d * 0.5 / camera.eye_dist);
    }
    if ( IsZero(camera.horiz_fov) )
        camera.horiz_fov = 0.01;
    if ( IsZero(camera.vert_fov) )
        camera.vert_fov = 0.01;


    Camera_Set_Object_From_Camera(TRUE);

    changed_scene = TRUE;

}


/*  Create the camera object. */
void
Camera_Create_Object()
{
    int i;

    camera_object.o_label = Strdup("Camera");
    camera_object.o_parent = NULL;

    camera_object.o_wireframe = Generic_Camera_Wireframe();
    Edge_Table_Build(camera_object.o_wireframe);

    NewIdentityMatrix(camera_object.o_transform.matrix);
    NewIdentityMatrix(camera_object.o_inverse);
    VNew(0.0, 0.0, 0.0, camera_object.o_transform.displacement);

    camera_object.o_attribs = NULL;

    camera_object.o_layer = 0;

    camera_object.o_num_vertices = camera_object.o_wireframe->num_vertices;
    camera_object.o_num_real = camera_object.o_wireframe->num_real_verts;
    camera_object.o_num_faces = camera_object.o_wireframe->num_faces;
    camera_object.o_world_verts = New(Vector, camera_object.o_num_vertices);
    camera_object.o_main_verts = New(Vertex, camera_object.o_num_vertices);
    camera_object.o_normals = New(Vector, camera_object.o_num_faces);
    for ( i = 0 ; i < camera_object.o_num_vertices ; i++ )
        camera_object.o_world_verts[i] =
            camera_object.o_wireframe->vertices[i];
    for ( i = 0 ; i < camera_object.o_num_faces ; i++ )
        camera_object.o_normals[i] =
            camera_object.o_wireframe->faces[i].normal;

    /* Set constraints. */
    /* 4 features, an Origin point, a major and minor axis and a scaling pt. */
    camera_object.o_num_features = 4;
    camera_object.o_features = New(Feature, 4);

    Feature_Create_Origin_Constraints(camera_object.o_features +origin_feature);
    Feature_Create_Major_Constraints(camera_object.o_features + major_feature,
                                     0, 0, 1);
    Feature_Create_Minor_Constraints(camera_object.o_features + minor_feature,
                                     1, 0, 0);
    Feature_Create_Camera_Scale_Constraints(
                                camera_object.o_features + scale_feature,
                                &camera_object,
                                camera_object.o_world_verts + 1);

    camera_object.o_dynamic_func = Maintain_Generic_Dynamic;
    camera_object.o_static_func = Maintain_Generic_Static;

    camera_object.o_dependents = NULL;
    camera_object.o_num_depend = 0;
    camera_object.o_hook = NULL;
    camera_object.o_flags = 0;
    camera_object.o_dfs_mark = 0;
}


/* Set the camera object to match the camera. Redraw if requested. */
void
Camera_Set_Object_From_Camera(Boolean draw_it)
{
    Camera  old_cam;
    Matrix  scale_matrix;
    Matrix  rotate_matrix, up_matrix;
    Vector  rotate_axis;
    Vector  camera_dir, orig_dir, orig_vertex;
    Vector  new_vertex;
    Vector  orig_right, new_right;
    Vector  start_origin, start_scale;
    Vector  temp_v;
    double  temp_d;

    /* Get the old values. */
    Camera_Set_Camera_From_Object(&old_cam);

    /* Get the origin and scale pts in generic space. */
    MVMul(camera_object.o_inverse,
          camera_object.o_features[origin_feature].location, start_origin);
    MVMul(camera_object.o_inverse,
          camera_object.o_features[scale_feature].location, start_scale);

    /* Set the scaling. */
    NewIdentityMatrix(scale_matrix);
    scale_matrix.z.z = camera.eye_dist / old_cam.eye_dist;

    /* FOV. */
    scale_matrix.x.x = (camera.eye_dist * tan(camera.vert_fov * M_PI/360.0)) /
                       (old_cam.eye_dist * tan(old_cam.vert_fov * M_PI/360.0));
    scale_matrix.y.y = (camera.eye_dist * tan(camera.horiz_fov * M_PI/360.0)) /
                       (old_cam.eye_dist * tan(old_cam.horiz_fov * M_PI/360.0));

    /* Apply the scaling. */
    camera_object.o_transform.matrix =
        MMMul(&(camera_object.o_transform.matrix), &scale_matrix);
    Transform_Instance(&camera_object, &(camera_object.o_transform), TRUE);

    /* Transform the axes etc. */
    MVMul(camera_object.o_transform.matrix, start_origin,
          camera_object.o_features[origin_feature].location);
    MVMul(camera_object.o_transform.matrix, start_scale,
          camera_object.o_features[scale_feature].location);

    /* Now line it up. */
    VSub(camera.look_at, camera.location, camera_dir);
    VUnit(camera_dir, temp_d, camera_dir);
    VSub(old_cam.look_at, old_cam.location, orig_dir);
    VUnit(orig_dir, temp_d, orig_dir);
    VCross(orig_dir, camera_dir, rotate_axis);
    if ( VZero(rotate_axis) )
        VNew(1, 0, 0, rotate_axis);
    else
        VUnit(rotate_axis, temp_d, rotate_axis);
    rotate_matrix = Quaternion_To_Matrix(
                    Build_Rotate_Quaternion(rotate_axis,
                                            VDot(orig_dir, camera_dir)));

    /* Rotate matrix holds the rotation to align the axes. */
    /* Also need to align look_up properly. */
    /* Find old right vector. */
    VCross(orig_dir, old_cam.look_up, orig_right);
    MVMul(rotate_matrix, orig_right, temp_v);
    VUnit(temp_v, temp_d, orig_right);

    /* Find new right vector. */
    VCross(camera_dir, camera.look_up, new_right);
    VUnit(new_right, temp_d, new_right);

    /* Find the rotation to put the orig_right onto the new right. */
    VCross(orig_right, new_right, rotate_axis);
    if ( VZero(rotate_axis) )
        VNew(1, 0, 0, rotate_axis);
    else
        VUnit(rotate_axis, temp_d, rotate_axis);
    up_matrix = Quaternion_To_Matrix(
                    Build_Rotate_Quaternion(rotate_axis,
                                            VDot(orig_right, new_right)));
    rotate_matrix = MMMul(&up_matrix, &rotate_matrix);

    camera_object.o_transform.matrix =
        MMMul(&rotate_matrix, &(camera_object.o_transform.matrix));
    VNew(0, 0, 1, orig_vertex);
    MVMul(camera_object.o_transform.matrix, orig_vertex, new_vertex);
    VSub(camera.location, new_vertex, camera_object.o_transform.displacement);

    Transform_Instance(&camera_object, &(camera_object.o_transform), TRUE);

    /* Transform the axes and scaling and origin again. */
    MVMul(rotate_matrix, camera_object.o_features[origin_feature].location,
          temp_v);
    camera_object.o_features[origin_feature].location = temp_v;
    MVMul(rotate_matrix, camera_object.o_features[scale_feature].location,
          temp_v);
    camera_object.o_features[scale_feature].location = temp_v;
    MVMul(rotate_matrix, camera_object.o_features[major_feature].location,
          temp_v);
    camera_object.o_features[major_feature].location = temp_v;
    MVMul(rotate_matrix, camera_object.o_features[minor_feature].location,
          temp_v);
    camera_object.o_features[minor_feature].location = temp_v;

    if ( Edit_Maintain_All_Constraints(&camera_object, &main_window, TRUE) )
        Update_Projection_Extents(topological_list);
    else if ( draw_it && ( camera_object.o_flags & ObjVisible ) )
    {
        View_Update(&main_window, NULL, CalcView);
        camera_object.o_proj_extent =
            Calculate_Projection_Extents(camera_object.o_main_verts,
                                         camera_object.o_num_real);
    }
    Edit_Maintain_Free_List();
}

/* Set the camera from the Object. */
void
Camera_Set_Camera_From_Object(Camera *cam)
{
    Vector  dir;

    /* Location. */
    cam->location = camera_object.o_world_verts[0];

    VSub(camera_object.o_world_verts[6], camera_object.o_world_verts[0], dir);

    /* Eye dist. */
    cam->eye_dist = VMod(dir);

    /* Look at. */
    VScalarMul(dir, 2, dir);
    VAdd(camera_object.o_world_verts[0], dir, cam->look_at);

    /* Look up. */
    VSub(camera_object.o_world_verts[5], camera_object.o_world_verts[6],
         cam->look_up);

    /* FOV. */
    VSub(camera_object.o_world_verts[1], camera_object.o_world_verts[4], dir);
    cam->horiz_fov = 360 * atan( VMod(dir) * 0.5 / cam->eye_dist ) / M_PI;
    VSub(camera_object.o_world_verts[1], camera_object.o_world_verts[2], dir);
    cam->vert_fov = 360 * atan( VMod(dir) * 0.5 / cam->eye_dist ) / M_PI;

    cam->defined = TRUE;
}


static void
Camera_Toggle_Display(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    victim;

    if ( camera_object.o_flags & ObjVisible )
    {
        camera_object.o_flags &= ( ObjAll ^ ( ObjVisible | ObjSelected ) );

        /* Look for it in the edit and selected lists. */
        if ( ( victim = Find_Object_In_Instances(&camera_object,
                                        main_window.edit_instances) ) )
            Delete_Edit_Instance(&main_window, victim);
        if ( ( victim = Find_Object_In_Instances(&camera_object,
                                    main_window.selected_instances) ) )
        {
            if ( victim == main_window.selected_instances )
                main_window.selected_instances =
                    main_window.selected_instances->next;
            Delete_Element(victim);
        }

        View_Update(&main_window, NULL, ViewNone);

        XtVaSetValues(display_widget, XtNlabel, "Show", NULL);
    }
    else
    {
        camera_object.o_flags |= ObjVisible;
        View_Update(&main_window, NULL, CalcView);
        camera_object.o_proj_extent =
            Calculate_Projection_Extents(camera_object.o_main_verts,
                                         camera_object.o_num_real);

        XtVaSetValues(display_widget, XtNlabel, "Hide", NULL);
    }
}


static void
Camera_Edit_Object(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList    elmt = NULL;

    if ( ! ( camera_object.o_flags & ObjVisible ) )
        return;

    Insert_Element(&elmt, &camera_object);
    Add_Instance_To_Edit(&main_window, elmt, TRUE);
    Free_Selection_List(elmt);
}


void
Camera_Create_Menu()
{
    Widget  menu_widget;
    Widget  menu_children[3];

    int     count = 0;

    menu_widget = XtCreatePopupShell("cameraMenu", simpleMenuWidgetClass,
                                        main_window.shell, NULL, 0);

    menu_children[count] = XtCreateManagedWidget("Edit Values",
                                smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(menu_children[count], XtNcallback,
                    Define_Camera_Callback, NULL);
    count++;

    display_widget =
    menu_children[count] = XtCreateManagedWidget("Show", smeBSBObjectClass,
                                                    menu_widget, NULL, 0);
    XtAddCallback(menu_children[count], XtNcallback,
                    Camera_Toggle_Display, NULL);
    count++;

    menu_children[count] = XtCreateManagedWidget("Edit Object",
                                smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(menu_children[count], XtNcallback,
                    Camera_Edit_Object, NULL);
    count++;

}

