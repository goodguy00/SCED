/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/agg_view.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: agg_view.c,v $
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
**  csg_view.c : Functions relating to the csg viewing window.
*/

#include <ctype.h>
#include <sced.h>
#include <base_objects.h>
#include <aggregate.h>
#include <layers.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/Viewport.h>
#include <View.h>


/* The translation table for object selection.  Defined in events.c. */
extern String selection_string;

extern Pixmap   menu_bitmap;

/*
**  Function prototypes.
*/
static Widget   Create_Agg_Buttons(int);
static void Create_Agg_Menu();
static void Create_Object_Menu();
static void Create_Lights_Menu();
static void Agg_View_Initialize();

/* Widgets. */
static Widget   agg_window_form_widget;
static Widget   agg_view_viewport;
static Widget   option_buttons[10];
static int      option_count = 0;

static Widget   agg_delete_button;
static Widget   agg_copy_button;
static Widget   agg_export_button;
extern Widget   new_agg_button;

extern Pixmap   icon_bitmap;

/* Response function for a Delete message from the window manager.
*/
void
Agg_Window_Delete_Message()
{
    /* Check for active objects. */
    if ( agg_window.all_instances )
    {
        Popup_Error("Cannot delete a non-empty window.", agg_window.shell,
                    "Sorry");
        return;
    }
    
    XtDestroyWidget(agg_window.shell);
    option_count = 0;
    agg_window.shell = NULL;
    agg_window.view_widget = NULL;
    agg_window.text_widget = NULL;
    agg_window.text_label = NULL;
    agg_window.apply_button = NULL;
    free(agg_window.axes.o_world_verts);
    free(agg_window.axes.o_main_verts);

    agg_window_form_widget =
    agg_view_viewport =
    agg_delete_button =
    agg_copy_button =
    agg_export_button = NULL;
}


/*  void
**  Create_Agg_Display()
**  Creates and initializes all the widgets for the aggregate display.
*/
void
Create_Agg_Display()
{
    Arg args[15];
    int n;
#if ( ! MISSING_ATOMS )
    Atom    delete_atom = XInternAtom(XtDisplay(main_window.shell),
                                      "WM_DELETE_WINDOW", FALSE);
#endif

    Widget  button;             /* Any button. */

    String      shell_geometry;
    unsigned    form_width, form_height;    /* The size of the window. */
    Dimension   apply_width, label_width, label_height; /* To lay out widgets.*/
    int     gap;                /* The gap between widgets in the form. */
    int     agg_window_width;   /* Used for sizing widgets. */
    int     agg_window_height;
    int     temp_width, temp_height;
    int     junk;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Aggregate Window");        n++;
    XtSetArg(args[n], XtNiconPixmap, icon_bitmap);  n++;
    agg_window.shell = XtCreatePopupShell("aggShell",
                        topLevelShellWidgetClass, main_window.shell, args, n);

    /* Find out the geometry of the top level shell.    */
    n = 0;
    XtSetArg(args[n], XtNgeometry, &shell_geometry);    n++;
    XtGetValues(agg_window.shell, args, n);
    XParseGeometry(shell_geometry, &junk, &junk, &form_width, &form_height);

    n = 0;
    XtSetArg(args[n], XtNwidth, &form_width);       n++;
    XtSetArg(args[n], XtNheight, &form_height);     n++;
    agg_window_form_widget = XtCreateManagedWidget("aggWindowForm",
                    formWidgetClass, agg_window.shell, args, n);

    /* Get the spacing of the form.  I need it to lay things out. */
    n = 0;
    XtSetArg(args[n], XtNdefaultDistance, &gap);    n++;
    XtGetValues(agg_window_form_widget, args, n);

    agg_window_width = (int)form_width;
    agg_window_height = (int)form_height;

    /* Create all the buttons.  Any one is returned. */
    button = Create_Agg_Buttons(gap);

    apply_width = Match_Widths(option_buttons, option_count);

    n = 0;
    temp_width = agg_window_width - (int)apply_width - 3 * gap - 4;
    temp_height = agg_window_height - 40;
    XtSetArg(args[n], XtNwidth, temp_width);    n++;
    XtSetArg(args[n], XtNheight, temp_height);  n++;
    XtSetArg(args[n], XtNfromHoriz, button);    n++;
    XtSetArg(args[n], XtNallowHoriz, TRUE);     n++;
    XtSetArg(args[n], XtNallowVert, TRUE);      n++;
    XtSetArg(args[n], XtNuseBottom, TRUE);      n++;
    XtSetArg(args[n], XtNuseRight, TRUE);       n++;
    agg_view_viewport = XtCreateManagedWidget("aggViewViewport",
                        viewportWidgetClass, agg_window_form_widget, args, n);

    /* Then the view widget to go inside it. */
    n = 0;
    if ( agg_window.viewport.scr_width == 0 )
        XtSetArg(args[n], XtNwidth, temp_width);
    else
        XtSetArg(args[n], XtNwidth, agg_window.viewport.scr_width);
    n++;
    if ( agg_window.viewport.scr_height == 0 )
        XtSetArg(args[n], XtNheight, temp_height);
    else
        XtSetArg(args[n], XtNheight, agg_window.viewport.scr_height);
    n++;
    XtSetArg(args[n], XtNmaintainSize, TRUE);           n++;
    agg_window.view_widget = XtCreateManagedWidget("aggViewWindow",
                                viewWidgetClass, agg_view_viewport, args, n);
    agg_window.magnify = agg_window.viewport.magnify;


    /* Add the Apply button. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Apply");               n++;
    XtSetArg(args[n], XtNwidth, apply_width);           n++;
    XtSetArg(args[n], XtNfromVert, agg_view_viewport);  n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainLeft);           n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);           n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);        n++;
    XtSetArg(args[n], XtNresizable, TRUE);              n++;
    agg_window.apply_button = XtCreateManagedWidget("applyButton",
                        commandWidgetClass, agg_window_form_widget, args, n);
    XtAddCallback(agg_window.apply_button, XtNcallback,
                    Apply_Button_Callback, (XtPointer)&agg_window);

    /* Add the text label. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Ready");                       n++;
    XtSetArg(args[n], XtNwidth, apply_width);                   n++;
    XtSetArg(args[n], XtNfromVert, agg_view_viewport);          n++;
    XtSetArg(args[n], XtNfromHoriz, agg_window.apply_button);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);                    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);                   n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);                   n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);                n++;
    XtSetArg(args[n], XtNresizable, TRUE);                      n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    agg_window.text_label = XtCreateManagedWidget("textLabel",
                        labelWidgetClass, agg_window_form_widget, args, n);

    /*
    **  Add the text widget.
    */
    /* Need to figure out how big to make it. */
    n = 0;
    XtSetArg(args[n], XtNwidth, &label_width);  n++;
    XtSetArg(args[n], XtNheight, &label_height);n++;
    XtGetValues(agg_window.text_label, args, n);

    n = 0;
    agg_window.text_string[0] = '\0';
    XtSetArg(args[n], XtNwidth,
            agg_window_width - (int)apply_width - (int)label_width -
            4 * gap - 6);                                       n++;
    XtSetArg(args[n], XtNheight, label_height);                 n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);                n++;
    XtSetArg(args[n], XtNlength, ENTRY_STRING_LENGTH);          n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);               n++;
    XtSetArg(args[n], XtNstring, agg_window.text_string);       n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);           n++;
    XtSetArg(args[n], XtNfromVert, agg_view_viewport);          n++;
    XtSetArg(args[n], XtNfromHoriz, agg_window.text_label);     n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);                    n++;
    XtSetArg(args[n], XtNright, XtChainRight);                  n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);                   n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);                n++;
    XtSetArg(args[n], XtNresizable, TRUE);                      n++;
    agg_window.text_widget = XtCreateManagedWidget("textEntry",
                        asciiTextWidgetClass, agg_window_form_widget, args, n);
    /* Add translations. */
    XtOverrideTranslations(agg_window.text_widget,
        XtParseTranslationTable(":<Key>Return: Apply_Button()"));

    /* Initialize the main viewing window. */
    Agg_View_Initialize();

    /* Add the expose callback for the view widget. */
    XtAddCallback(agg_window.view_widget, XtNexposeCallback,
                    Redraw_Main_View, &agg_window);

    /* Set CSG button sensitivity. */
    if ( ! num_agg_base_objects )
        Set_Agg_Related_Sensitivity(FALSE);

    /* Realize it. */
    XtRealizeWidget(agg_window.shell);

#if ( ! MISSING_ATOMS )
    XSetWMProtocols(XtDisplay(agg_window.shell), XtWindow(agg_window.shell), 
                    &delete_atom, 1);
#endif
}


static Widget
Create_Agg_Buttons(int gap)
{
    Arg         args[15];
    int         n;

    /* Create the menus for menuButton popups. */
    Create_Agg_Menu();
    Create_Object_Menu();
    Create_Lights_Menu();
    Create_View_Menu(agg_window.shell, &agg_window);
    Create_Window_Menu(agg_window.shell, &agg_window);
    Create_Edit_Menu(&agg_window);
    Layers_Create_Menu(&agg_window);

    /* All the option_buttons have the same chaining, so just set it once. */
    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;


    /* A Agg menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Aggregate");   n++;
    XtSetArg(args[n], XtNfromVert, NULL);       n++;
    XtSetArg(args[n], XtNmenuName, "AggMenu");  n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("aggButton",
                menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    /* A Combine button. */
    n = 5;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
    XtSetArg(args[n], XtNlabel, "Complete");                        n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    option_buttons[option_count] = XtCreateManagedWidget("objectButton",
                commandWidgetClass, agg_window_form_widget, args, n);
    XtAddCallback(option_buttons[option_count], XtNcallback,
                  Agg_Complete_Callback, NULL);
    option_count++;

    /* An Object menu button. */
    n = 5;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
    XtSetArg(args[n], XtNlabel, "Object");                          n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNmenuName, "ObjectMenu");                   n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);                  n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("objectButton",
                menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    /* A Lights menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Lights");                          n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
    XtSetArg(args[n], XtNmenuName, "LightsMenu");                   n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);                  n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("lightsButton",
                menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    /* A change viewport menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "View");                            n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNmenuName, "ViewMenu");                     n++;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);                  n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("viewButton",
                    menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    /* A window menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Window");                          n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNmenuName, "WindowMenu");                   n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);                  n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("windowButton",
                    menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    /* A layers menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Layers");                          n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNmenuName, "LayerMenu");                    n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);                  n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("layerButton",
                    menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    /* A reset button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Clear Agg");                       n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
    option_buttons[option_count] = XtCreateManagedWidget("resetButton",
                    commandWidgetClass, agg_window_form_widget, args, n);
    XtAddCallback(option_buttons[option_count], XtNcallback, Clear_Dialog_Func,
                    (XtPointer)&agg_window);
    option_count++;

    /* An Edit menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Edit");                            n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNmenuName, "EditMenu");                     n++;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
    XtSetArg(args[n], XtNsensitive, FALSE);                         n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);                  n++;
#endif
    agg_window.edit_menu->button = option_buttons[option_count] =
        XtCreateManagedWidget("editButton",
                menuButtonWidgetClass, agg_window_form_widget, args, n);
    option_count++;

    return option_buttons[option_count - 1];

}


static void
Close_Agg_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XUnmapWindow(XtDisplay(agg_window.shell), XtWindow(agg_window.shell));
}


static void
Agg_Copy_Existing_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Base_Select_Popup(aggregate_obj, AGG_COPY);
}


static void
Agg_Delete_Existing_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Base_Select_Popup(aggregate_obj, AGG_DELETE);
}


static void
Agg_Export_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    Base_Select_Popup(aggregate_obj, AGG_EXPORT);
}

/*  void
**  Create_Agg_Menu()
**  Creates the menu which pops up from the Aggregate button.
*/
static void
Create_Agg_Menu()
{
    Widget  menu_widget;
    Widget  close_button;

    menu_widget = XtCreatePopupShell("AggMenu", simpleMenuWidgetClass,
                                        agg_window.shell, NULL, 0);

    agg_copy_button = XtCreateManagedWidget("Copy",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(agg_copy_button, XtNcallback,
                  Agg_Copy_Existing_Callback, NULL);

    agg_delete_button = XtCreateManagedWidget("Delete",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(agg_delete_button, XtNcallback,
                  Agg_Delete_Existing_Callback, NULL);

    agg_export_button = XtCreateManagedWidget("Export",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(agg_export_button, XtNcallback,
                  Agg_Export_Callback, NULL);

    close_button = XtCreateManagedWidget("Close", smeBSBObjectClass,
                                                    menu_widget, NULL, 0);
    XtAddCallback(close_button, XtNcallback, Close_Agg_Callback, NULL);
}

/*  void
**  Create_Object_Menu()
**  Creates the menu which pops up from the Edit button.
**  Functions included: New, Move, Copy, Name, Attributes
*/
static void
Create_Object_Menu()
{
    Widget  object_widget;
    Widget  object_children[10];

    int     count = 0;


    object_widget = XtCreatePopupShell("ObjectMenu", simpleMenuWidgetClass,
                                        agg_window.shell, NULL, 0);

    object_children[count] = XtCreateManagedWidget("New", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                    New_Object_Popup_Callback, (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Edit", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Edit_Objects_Function,
                    (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Copy", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Copy_Objects_Callback,
                    (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Delete", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Delete_Objects_Callback,
                (XtPointer)&agg_window);
    count++;

    XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

    object_children[count] = XtCreateManagedWidget("Name", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Rename_Callback,
                  (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Attributes",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Set_Attributes_Callback,
                    (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Alias",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                  Alias_Object_Alias_Callback, (XtPointer)&agg_window);
    count++;

    XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

    object_children[count] = XtCreateManagedWidget("Dense Wire",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                  Wireframe_Denser_Callback, (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Thin Wire",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                  Wireframe_Thinner_Callback, (XtPointer)&agg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Change Base",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Base_Change_Callback,
                    (XtPointer)&agg_window);
    count++;
}

static void
Create_Lights_Menu()
{
    Widget  lights_widget;
    Widget  lights_children[5];

    int     count = 0;


    lights_widget = XtCreatePopupShell("LightsMenu", simpleMenuWidgetClass,
                                        agg_window.shell, NULL, 0);

    lights_children[count] = XtCreateManagedWidget("Point", smeBSBObjectClass,
                                                    lights_widget, NULL, 0);
    XtAddCallback(lights_children[count], XtNcallback, Create_Light_Callback,
                  &agg_window);
    count++;

    lights_children[count] = XtCreateManagedWidget("Spot", smeBSBObjectClass,
                                                    lights_widget, NULL, 0);
    XtAddCallback(lights_children[count], XtNcallback,
                  Create_Spotlight_Callback, &agg_window);
    count++;

    lights_children[count] = XtCreateManagedWidget("Area", smeBSBObjectClass,
                                                    lights_widget, NULL, 0);
    XtAddCallback(lights_children[count], XtNcallback,
                  Create_Arealight_Callback, &agg_window);
    count++;

    lights_children[count] = XtCreateManagedWidget("Directional",
                                smeBSBObjectClass, lights_widget, NULL, 0);
    XtAddCallback(lights_children[count], XtNcallback,
                  Create_Dirlight_Callback, &agg_window);
    count++;
}

static void
Agg_View_Initialize()
{

    /* Set up the axes. */
    Window_Axes_Initialize(&agg_window);

    /* Install translations. */
    XtOverrideTranslations(agg_window.view_widget,
                           XtParseTranslationTable(selection_string));

}



/*  void
**  Sensitize_Agg_Buttons(Boolean state)
**  A function to sensitize or desensitize all the buttons.
*/
void
Sensitize_Agg_Buttons(Boolean state, int mask) 
{
    Arg arg;
    int i;

    if ( ! agg_window.shell ) return;

    XtSetArg(arg, XtNsensitive, state);
    for ( i = 0 ; i < option_count - 1 ; i++ )
        if ( mask & ( 1 << i ) )
            XtSetValues(option_buttons[i], &arg, 1);

    if ( agg_window.edit_menu->num_children != 0 && ( mask & SenAggEdit ) )
        XtSetValues(agg_window.edit_menu->button, &arg, 1);
}



/*  void
**  Set_Agg_Related_Sensitivity(Boolean state)
**  Sets the state of buttons which are only relevent if Agg objects are
**  defined.
*/
void
Set_Agg_Related_Sensitivity(Boolean state)
{
    Arg arg;
    XtSetArg(arg, XtNsensitive, state);
    if ( agg_copy_button )
    {
        XtSetValues(agg_copy_button, &arg, 1);
        XtSetValues(agg_delete_button, &arg, 1);
        XtSetValues(agg_export_button, &arg, 1);
    }
    if ( new_agg_button )
        XtSetValues(new_agg_button, &arg, 1);
}


