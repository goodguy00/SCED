/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/preview.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: preview.c,v $
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
**  preview.c : Functions to preview the picture.
*/

#include <sced.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#include <ctype.h>
#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <instance_list.h>
#include <View.h>

extern char   start_directory_name[];


static void Create_Preview_Dialog();
static void Cancel_Preview(Widget, XtPointer, XtPointer);
static void Perform_Preview_Callback(Widget, XtPointer, XtPointer);

static WindowInfoPtr    current_window;

static Widget   preview_dialog = NULL;
static Widget   target_widget = NULL;
static Widget   all_widget;
static Widget   camera_widget;
static Widget   width_widget, height_widget;

static Renderer preview_target = NoTarget;

#define TEXT_LENGTH 10
static char width_string[TEXT_LENGTH];
static char height_string[TEXT_LENGTH];

char    *rayshade_path = RAYSHADE_PATH;
char    *rayshade_options = RAYSHADE_OPTIONS;
char    *povray_path = POVRAY_PATH;
char    *povray_options = POVRAY_OPTIONS;
char    *radiance_path = RADIANCE_PATH;
char    *radiance_options = RADIANCE_OPTIONS;
char    *renderman_path = RENDERMAN_PATH;
char    *renderman_options = RENDERMAN_OPTIONS;
char    *vrml_path = VRML_PATH;
char    *vrml_options = VRML_OPTIONS;

/*  void
**  Preview_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**  Sets a raytracer to previewing the scene.  cl_data is assumed to be
**  a WindowInfoPtr.
*/
void
Preview_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    current_window = (WindowInfoPtr)cl_data;

    if ( ! preview_dialog )
        Create_Preview_Dialog();
    Preview_Sensitize(TRUE);

    Position_Shell(preview_dialog, FALSE);
    XtPopup(preview_dialog, XtGrabExclusive);
}


void
Preview_Sensitize(Boolean state)
{
    if ( ! preview_dialog )
        Create_Preview_Dialog();

    XtSetSensitive(all_widget, state);
    XtSetSensitive(camera_widget, state);
}


static void
Create_Preview_Dialog()
{
    Widget  form;
    Widget  buttons[6];
    Widget  width_label, height_label;
    Widget  go, cancel;
    Arg     args[15];
    int     n, m, count;
    Dimension   width, height;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Preview");         n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    preview_dialog = XtCreatePopupShell("previewDialog",
                    transientShellWidgetClass, current_window->shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("previewForm", formWidgetClass,
                                    preview_dialog, args, n);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;
    m = n;

    count = 0;

#if POVRAY_SUPPORT
    n = m;
    XtSetArg(args[n], XtNlabel, "POVray");              n++;
    XtSetArg(args[n], XtNradioData, (XtPointer)POVray); n++;
    target_widget =
    buttons[count] = XtCreateManagedWidget("previewPOVray", toggleWidgetClass,
                                        form, args, n);
    XtVaSetValues(buttons[count], XtNradioGroup, target_widget, NULL);
    count++;
#endif

#if RADIANCE_SUPPORT
    n = m;
    XtSetArg(args[n], XtNlabel, "Radiance");                    n++;
    if ( count )
    {
        XtSetArg(args[n], XtNfromHoriz, buttons[count - 1]);    n++;
    }
    XtSetArg(args[n], XtNradioData, (XtPointer)Radiance);       n++;
    buttons[count] = XtCreateManagedWidget("previewRadiance", toggleWidgetClass,
                                        form, args, n);
    if ( ! count )
        target_widget = buttons[count];
    XtVaSetValues(buttons[count], XtNradioGroup, target_widget, NULL);
    count++;
#endif

#if RAYSHADE_SUPPORT
    n = m;
    XtSetArg(args[n], XtNlabel, "Rayshade");                n++;
    if ( count > 1 )
    {
        XtSetArg(args[n], XtNfromVert, buttons[count-2]);   n++;
    }
    if ( count % 2 )
    {
        XtSetArg(args[n], XtNfromHoriz, buttons[count-1]);  n++;
    }
    XtSetArg(args[n], XtNradioData, (XtPointer)Rayshade);   n++;
    buttons[count] = XtCreateManagedWidget("previewRayshade", toggleWidgetClass,
                                        form, args, n);
    if ( ! count )
        target_widget = buttons[count];
    XtVaSetValues(buttons[count], XtNradioGroup, target_widget, NULL);
    count++;
#endif

#if RENDERMAN_SUPPORT
    n = m;
    XtSetArg(args[n], XtNlabel, "RenderMan");               n++;
    if ( count > 1 )
    {
        XtSetArg(args[n], XtNfromVert, buttons[count-2]);   n++;
    }
    if ( count % 2 )
    {
        XtSetArg(args[n], XtNfromHoriz, buttons[count-1]);  n++;
    }
    XtSetArg(args[n], XtNradioData, (XtPointer)Renderman);  n++;
    buttons[count] = XtCreateManagedWidget("previewRenderman",toggleWidgetClass,
                                        form, args, n);
    if ( ! count )
        target_widget = buttons[count];
    XtVaSetValues(buttons[count], XtNradioGroup, target_widget, NULL);
    count++;
#endif

#if VRML_SUPPORT
    n = m;
    XtSetArg(args[n], XtNlabel, "VRML");                    n++;
    if ( count > 1 )
    {
        XtSetArg(args[n], XtNfromVert, buttons[count-2]);   n++;
    }
    if ( count % 2 )
    {
        XtSetArg(args[n], XtNfromHoriz, buttons[count-1]);  n++;
    }
    XtSetArg(args[n], XtNradioData, (XtPointer)VRML);       n++;
    buttons[count] = XtCreateManagedWidget("previewVRML",toggleWidgetClass,
                                        form, args, n);
    if ( ! count )
        target_widget = buttons[count];
    XtVaSetValues(buttons[count], XtNradioGroup, target_widget, NULL);
    count++;
#endif

    Match_Widths(buttons, count);

    /* Set up labels and strings. */

    /* Set the width/height strings. */
    XtVaGetValues(current_window->view_widget, XtNdesiredWidth, &width,
                  XtNdesiredHeight, &height, NULL);
    if ( ! width )
        XtVaGetValues(current_window->view_widget, XtNwidth, &width,
                      XtNheight, &height, NULL);
    sprintf(width_string, "%d"HPFIX, (int)width);
    sprintf(height_string, "%d"HPFIX, (int)height);

    /* The width label. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Width ");              n++;
    XtSetArg(args[n], XtNfromVert, buttons[count-1]);   n++;
    XtSetArg(args[n], XtNfromHoriz, NULL);              n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    buttons[0] =
    width_label = XtCreateManagedWidget("previewWidthLabel", labelWidgetClass,
                    form, args, n);

    /* Width string. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNlength, TEXT_LENGTH);          n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, width_string);         n++;
    XtSetArg(args[n], XtNfromVert, buttons[count-1]);   n++;
    XtSetArg(args[n], XtNfromHoriz, width_label);       n++;
    width_widget = XtCreateManagedWidget("previewWidthText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(width_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    n = m;
    XtSetArg(args[n], XtNlabel, "Height ");         n++;
    XtSetArg(args[n], XtNfromVert, width_widget);   n++;
    XtSetArg(args[n], XtNfromHoriz, NULL);          n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    buttons[1] =
    height_label = XtCreateManagedWidget("previewHeightLabel", labelWidgetClass,
                    form, args, n);

    /* Height string. */
    n = m;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, TEXT_LENGTH);      n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, height_string);    n++;
    XtSetArg(args[n], XtNfromVert, width_widget);   n++;
    XtSetArg(args[n], XtNfromHoriz, height_label);  n++;
    height_widget = XtCreateManagedWidget("previewHeightText",
                    asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(height_widget,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    Match_Widths(buttons, 2);

    /* All toggle. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Preview All");     n++;
    XtSetArg(args[n], XtNfromVert, height_label);   n++;
    all_widget = XtCreateManagedWidget("previewAll", toggleWidgetClass,
                                        form, args, n);

    /* Camera toggle. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Camera");          n++;
    XtSetArg(args[n], XtNfromVert, height_label);   n++;
    XtSetArg(args[n], XtNfromHoriz, all_widget);    n++;
    camera_widget = XtCreateManagedWidget("previewCamera", toggleWidgetClass,
                                          form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Go !");            n++;
    XtSetArg(args[n], XtNfromVert, all_widget);     n++;
    XtSetArg(args[n], XtNfromHoriz, NULL);          n++;
    buttons[0] =
    go = XtCreateManagedWidget("previewGo", commandWidgetClass,
                                form, args, n);
    XtAddCallback(go, XtNcallback, Perform_Preview_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");      n++;
    XtSetArg(args[n], XtNfromVert, all_widget); n++;
    XtSetArg(args[n], XtNfromHoriz, go);        n++;
    buttons[1] =
    cancel = XtCreateManagedWidget("previewCancel", commandWidgetClass,
                                    form, args, n);
    XtAddCallback(cancel, XtNcallback, Cancel_Preview, NULL);

    Match_Widths(buttons, 2);

    /* Set the target. */
    if ( target_widget && target_renderer != NoTarget )
        XawToggleSetCurrent(target_widget, (XtPointer)target_renderer);

    XtRealizeWidget(preview_dialog);
}

static void
Cancel_Preview(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(preview_dialog);
}


static void
Perform_Preview_Callback(Widget w, XtPointer cl_data, XtPointer ca)
{
    InstanceList    instances;
    int         preview_width = 0;
    int         preview_height = 0;
    Boolean     preview_all;
    Boolean     use_camera;

    /* Check for a target. */
    if ( ! target_widget )
    {
        Popup_Error("No target supported!", current_window->shell, "Error");
        return;
    }
    preview_target = (Renderer)XawToggleGetCurrent(target_widget);
    if ( preview_target == NoTarget )
    {
        Popup_Error("No target selected!", current_window->shell, "Error");
        return;
    }

    XtPopdown(preview_dialog);

    if ( XtIsSensitive(all_widget) )
    {
        XtVaGetValues(all_widget, XtNstate, &preview_all, NULL);
        instances = preview_all ?
                Instances_Build_Visible_List(current_window->all_instances) :
                Merge_Selection_Lists(current_window->selected_instances, NULL);
    }
    else
        instances = Merge_Selection_Lists(current_window->selected_instances,
                                          NULL);
    if ( XtIsSensitive(camera_widget) )
        XtVaGetValues(camera_widget, XtNstate, &use_camera, NULL);
    else
        use_camera = FALSE;

    if ( ! instances )
    {
        Popup_Error("Nothing To Preview!", current_window->shell, "Error");
        return;
    }

    sscanf(width_string, "%d\n", &preview_width);
    sscanf(height_string, "%d\n", &preview_height);

    Perform_Preview(current_window, preview_target, instances,
                    preview_width, preview_height, use_camera);
}


static Boolean
Preview_Check_Target(Renderer target, WindowInfoPtr window)
{
    switch ( target )
    {
        case POVray:
            if ( povray_path[0] == '\0' )
            {
                Popup_Error("POVRay does not exist on your system.",
                            window->shell, "Error");
                return FALSE;
            }
            break;
        case Rayshade:
            if ( rayshade_path[0] == '\0' )
            {
                Popup_Error("Rayshade does not exist on your system.",
                            window->shell, "Error");
                return FALSE;
            }
            break;
        case Radiance:
            if ( radiance_path[0] == '\0' )
            {
                Popup_Error("Radiance does not exist on your system.",
                            window->shell, "Error");
                return FALSE;
            }
            break;
        case Renderman:
            if ( renderman_path[0] == '\0' )
            {
                Popup_Error(
                    "A RenderMan renderer does not exist on your system.",
                    window->shell, "Error");
                return FALSE;
            }
            break;
        case VRML:
            if ( vrml_path[0] == '\0' )
            {
                Popup_Error("A VRML browser does not exist on your system.",
                            window->shell, "Error");
                return FALSE;
            }
            break;
        default:;
    }

    return TRUE;

}

void
Perform_Preview(WindowInfoPtr window, Renderer target, InstanceList insts,
                int width, int height, Boolean use_camera)
{
    SceneStruct preview_scene;
    char        *infilename;
    FILE        *outfile;
    char        *system_string = NULL;
    char        *prompt_string = NULL;
    int		i, j, chars;

    if ( ! insts )
        return;

    if ( ! Preview_Check_Target(target, window) )
        return;

    preview_scene.instances = insts;

    preview_scene.target = target;

    if ( use_camera && ! camera.defined )
    {
        Popup_Error("The Camera has not been defined. Using the Viewport.",
                    window->shell, "Error");
        use_camera = FALSE;
    }
    if ( use_camera )
        preview_scene.camera = camera;
    else
    {
        preview_scene.camera.defined = TRUE;
        Viewport_To_Camera(&(window->viewport), window->view_widget,
                           &(preview_scene.camera), TRUE);
    }

    if ( width )
        preview_scene.camera.scr_width = (Dimension)width;
    if ( height )
        preview_scene.camera.scr_height = (Dimension)height;

    /* Try to find a light amongst the instances. */
    if ( Light_In_Instances(preview_scene.instances) )
        preview_scene.light = NULL;
    else
    {
        if ( target == Renderman )
        {
            preview_scene.light =
                Create_Instance(Get_Base_Object_From_Label("dirlight"),
                                "PreviewLight");
            VScalarMul(preview_scene.camera.location, 1.2,
                       preview_scene.light->o_world_verts[5]);
            preview_scene.light->o_world_verts[0] =preview_scene.camera.look_at;
        }
        else
        {
            preview_scene.light =
                Create_Instance(Get_Base_Object_From_Label("light"),
                                "PreviewLight");

            /* Put a light near the eye position. */
            VScalarMul(preview_scene.camera.location, 1.2,
                       preview_scene.light->o_transform.displacement);
        }
        preview_scene.light->o_attribs->intensity.red =
        preview_scene.light->o_attribs->intensity.green =
        preview_scene.light->o_attribs->intensity.blue = 1.0;
    }

    infilename = New(char, 32 + strlen(start_directory_name));
    sprintf(infilename, "%s/previewXXXXXX", start_directory_name);
    /* Get a temporary filename to use. */
    if ( ! ( infilename = (char*)mktemp(infilename) ) )
    {
        Popup_Error("Unable to create file name for preview",
                    window->shell, "Error");
        return;
    }


    if ( ( outfile = fopen(infilename, "w")) == NULL )
    {
        prompt_string = New(char, strlen(infilename) + 48);
        sprintf(prompt_string,
            "Unable to open file %s for preview", infilename);
        Popup_Error(prompt_string, window->shell, "Error");
        free(prompt_string);
        return;
    }

    /* Store the filename for later deletion. */
    Save_Temp_Filename(infilename);

    preview_scene.ambient = ambient_light;

    if ( ! Export_File(outfile, infilename, &preview_scene, TRUE) )
        return;

    if ( preview_scene.light ) Destroy_Instance(preview_scene.light);

    switch ( target )
    {
        case Rayshade:
            system_string = New(char,
                            strlen(rayshade_path) + strlen(rayshade_options) +
                            strlen(infilename) + strlen(start_directory_name) +
                            30);
            sprintf(system_string, "%s %s -O %s/preview.rle < %s",
                    rayshade_path, rayshade_options,
                    start_directory_name, infilename);
            break;
        case Radiance:
            system_string = New(char,
                            strlen(radiance_path) + strlen(radiance_options) +
                            strlen(infilename) + 30);
            sprintf(system_string, "%s %s %s",
                    radiance_path, radiance_options, infilename);
            break;
        case Renderman:
            system_string = New(char,
                            strlen(renderman_path) + strlen(renderman_options) +
                            strlen(infilename)  + 30);
            sprintf(system_string, "%s %s %s",
                    renderman_path, renderman_options, infilename);
            break;
        case VRML:
            system_string = New(char,
                            strlen(vrml_path) + strlen(vrml_options) +
                            strlen(infilename) + 40);
            sprintf(system_string, "%s %s %s",
                    vrml_path, vrml_options, infilename);
            break;
        case POVray:
            system_string = New(char,
                            strlen(povray_path) + strlen(povray_options) +
                            strlen(infilename) + strlen(start_directory_name) +
                            50);
            sprintf(system_string, "%s -w%d -h%d -I%s -O%s/preview.tga %s",
                    povray_path, preview_scene.camera.scr_width,
                    preview_scene.camera.scr_height, infilename,
                    start_directory_name, povray_options);
            break;
        default: ;
    }
    prompt_string = New(char, strlen(system_string) + 128);
    sprintf(prompt_string, "Previewing using:\n");
    /* Break the preview string accross multiple lines if it's long. */
    j = strlen(prompt_string);
    for ( i = 0, chars = 0 ; i < strlen(system_string) ; i++, j++, chars++ )
    {
	if ( chars > 40 && isspace(system_string[i]) )
	{
	    prompt_string[j] = '\n';
	    chars = 0;
	}
	else
	    prompt_string[j] = system_string[i];
    }
    prompt_string[j] = '\0';

    /* Fork a shell to do the preview. */
    /* Actually, fork once, then fork again. By doing this, the second shell
    ** is reparented with init (pid 1). The first shell exits immediately.
    ** Why?, you ask. We want to get rid of zombie processes, but don't want
    ** to block and wait for the renderer. This way we can wait on the
    ** immediate child (which exits straight away), and let init take care
    ** of the child. Thanks to Stephen Norris (who got sick of his process
    ** table filling up) and Bruce Janson, Basser programmer extraordinair.
    */
    if ( ! fork() )
    {
        switch ( fork() )
        {
            case 0: /* The child. */
                execl("/bin/sh", "sh", "-c", system_string, 0);
                /* Here if the exec failed. */
                sprintf(prompt_string, "Couldn't exec previewer");
                exit(1);
            case -1:
                Popup_Error("Couldn't exec previewer", window->shell,
                            "Error");
                break;
        }
        exit(0);
    }
    else
    {
        wait(0);
        Popup_Error(prompt_string, window->shell, "Preview");
    }

    free(system_string);
    free(prompt_string);
    free(infilename);
    Free_Selection_List(insts);
}


