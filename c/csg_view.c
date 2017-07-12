/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/csg_view.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: csg_view.c,v $
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
#include <csg.h>
#include <layers.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Tree.h>
#include <X11/Xaw/Viewport.h>
#include <View.h>


/* The translation table for object selection.  Defined in events.c. */
extern String selection_string;

extern Pixmap   menu_bitmap;

/*
**  Function prototypes.
*/
static Widget   Create_CSG_Buttons(int);
static void Create_CSG_Menu();
static void Create_Object_Menu();
static void Create_CSG_Tree_Menus();
static void CSG_View_Initialize();

/* Widgets. */
Widget  csg_tree_widget;
Widget  csg_display_label[2];
static Widget   csg_window_form_widget;
static Widget   csg_view_viewport;
Widget  csg_tree_viewport;

static Widget   csg_paned_widget;
static Widget   option_buttons[10];
static int      option_count = 0;

/* Translations for the tree. */
static String   tree_translations =
                "<BtnDown>: csg_tree_notify() \n\
                 <Motion>:  csg_motion()";

static Widget   csg_modify_button;
static Widget   csg_delete_button;
static Widget   csg_copy_button;
static Widget   csg_export_button;
extern Widget   new_csg_button;

extern Pixmap   icon_bitmap;


/* Response function for a Delete message from the window manager.
*/
void
CSG_Window_Delete_Message()
{
    /* Check for active objects. */
    if ( num_displayed_trees )
    {
        Popup_Error("Cannot delete a non-empty window.", csg_window.shell,
                    "Sorry");
        return;
    }
    
    XtDestroyWidget(csg_window.shell);
    option_count = 0;
    csg_window.shell = NULL;
    csg_window.view_widget = NULL;
    csg_window.text_widget = NULL;
    csg_window.text_label = NULL;
    csg_window.apply_button = NULL;
    free(csg_window.axes.o_world_verts);
    free(csg_window.axes.o_main_verts);

    csg_tree_widget =
    csg_display_label[0] =
    csg_display_label[1] =
    csg_window_form_widget =
    csg_view_viewport =
    csg_tree_viewport =
    csg_paned_widget =
    csg_modify_button =
    csg_delete_button =
    csg_copy_button =
    csg_export_button = NULL;
}


/*  void
**  Create_CSG_Display()
**  Creates and initializes all the widgets for the csg display.
*/
void
Create_CSG_Display()
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
    int     csg_window_width;   /* Used for sizing widgets. */
    int     csg_window_height;
    int     temp_width, temp_height;
    int     junk;

    n = 0;
    XtSetArg(args[n], XtNtitle, "CSG Window");      n++;
    XtSetArg(args[n], XtNiconPixmap, icon_bitmap);  n++;
    csg_window.shell = XtCreatePopupShell("csgShell",
                        topLevelShellWidgetClass, main_window.shell, args, n);

    /* Find out the geometry of the top level shell.    */
    n = 0;
    XtSetArg(args[n], XtNgeometry, &shell_geometry);    n++;
    XtGetValues(csg_window.shell, args, n);
    XParseGeometry(shell_geometry, &junk, &junk, &form_width, &form_height);

    /* Create csg_window.shell's immediate child, a form widget. */
    n = 0;
    XtSetArg(args[n], XtNwidth, &form_width);       n++;
    XtSetArg(args[n], XtNheight, &form_height);     n++;
    csg_window_form_widget = XtCreateManagedWidget("csgWindowForm",
                    formWidgetClass, csg_window.shell, args, n);

    /* Get the spacing of the form.  I need it to lay things out. */
    n = 0;
    XtSetArg(args[n], XtNdefaultDistance, &gap);    n++;
    XtGetValues(csg_window_form_widget, args, n);

    csg_window_width = (int)form_width;
    csg_window_height = (int)form_height;

    /* Create all the buttons.  Any one is returned. */
    button = Create_CSG_Buttons(gap);

    apply_width = Match_Widths(option_buttons, option_count);

    /* Create a paned widget which will in turn hold the view and tree. */
    n = 0;
    temp_width = csg_window_width - (int)apply_width - 3 * gap - 4;
    temp_height = csg_window_height - 40;
    XtSetArg(args[n], XtNwidth, temp_width);                n++;
    XtSetArg(args[n], XtNheight, temp_height);              n++;
    XtSetArg(args[n], XtNorientation, XtorientVertical);    n++;
    XtSetArg(args[n], XtNfromHoriz, button);                n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);                n++;
    XtSetArg(args[n], XtNright, XtChainRight);              n++;
    XtSetArg(args[n], XtNtop, XtChainTop);                  n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);            n++;
    csg_paned_widget = XtCreateManagedWidget("csgPaneWidget", panedWidgetClass,
                                    csg_window_form_widget, args, n);


    /* Create a Viewport widget to hold the image. */
    /* It is a child of the paned widget. */
    n = 0;
    temp_height *= 0.75;
    XtSetArg(args[n], XtNwidth, temp_width);        n++;
    XtSetArg(args[n], XtNheight, temp_height);      n++;
    XtSetArg(args[n], XtNallowHoriz, TRUE);         n++;
    XtSetArg(args[n], XtNallowVert, TRUE);          n++;
    XtSetArg(args[n], XtNuseBottom, TRUE);          n++;
    XtSetArg(args[n], XtNuseRight, TRUE);           n++;
    XtSetArg(args[n], XtNresizeToPreferred, TRUE);  n++;
    XtSetArg(args[n], XtNallowResize, TRUE);        n++;
    csg_view_viewport = XtCreateManagedWidget("csgViewViewport",
                        viewportWidgetClass, csg_paned_widget, args, n);

    /* Then the view widget to go inside it. */
    n = 0;
    if ( csg_window.viewport.scr_width == 0 )
        XtSetArg(args[n], XtNwidth, temp_width);
    else
        XtSetArg(args[n], XtNwidth, csg_window.viewport.scr_width);
    n++;
    if ( csg_window.viewport.scr_height == 0 )
        XtSetArg(args[n], XtNheight, temp_height);
    else
        XtSetArg(args[n], XtNheight, csg_window.viewport.scr_height);
    n++;
    XtSetArg(args[n], XtNmaintainSize, FALSE);          n++;
    csg_window.view_widget = XtCreateManagedWidget("csgViewWindow",
                                viewWidgetClass, csg_view_viewport, args, n);
    csg_window.magnify = csg_window.viewport.magnify;


    /* Create a Viewport widget to hold the CSG trees. */
    /* It is a child of the paned widget also. */
    n = 0;
    temp_height /= 3;
    XtSetArg(args[n], XtNwidth, temp_width);        n++;
    XtSetArg(args[n], XtNheight, temp_height);      n++;
    XtSetArg(args[n], XtNallowHoriz, TRUE);         n++;
    XtSetArg(args[n], XtNallowVert, TRUE);          n++;
    XtSetArg(args[n], XtNuseBottom, TRUE);          n++;
    XtSetArg(args[n], XtNuseRight, TRUE);           n++;
    XtSetArg(args[n], XtNpreferredPaneSize, temp_height);   n++;
    XtSetArg(args[n], XtNresizeToPreferred, TRUE);  n++;
    /*XtSetArg(args[n], XtNallowResize, TRUE);      n++;*/
    csg_tree_viewport = XtCreateManagedWidget("csgTreeViewport",
                        viewportWidgetClass, csg_paned_widget, args, n);

    /* Then the tree widget to go inside it. */
    n = 0;
    XtSetArg(args[n], XtNwidth, temp_width);        n++;
    XtSetArg(args[n], XtNheight, temp_height);      n++;
    XtSetArg(args[n], XtNgravity, NorthGravity);    n++;
    XtSetArg(args[n], XtNautoReconfigure, FALSE);   n++;
    XtSetArg(args[n], XtNhSpace, 8);                n++;
    XtSetArg(args[n], XtNvSpace, 8);                n++;
    csg_tree_widget = XtCreateManagedWidget("csgTreeWindow",
                                treeWidgetClass, csg_tree_viewport, args, n);
    XtOverrideTranslations(csg_tree_widget,
                            XtParseTranslationTable(tree_translations));
    XawTreeForceLayout(csg_tree_widget);


    /* Add the Apply button. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Apply");               n++;
    XtSetArg(args[n], XtNwidth, apply_width);           n++;
    XtSetArg(args[n], XtNfromVert, csg_paned_widget);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainLeft);           n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);           n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);        n++;
    XtSetArg(args[n], XtNresizable, TRUE);              n++;
    csg_window.apply_button = XtCreateManagedWidget("applyButton",
                        commandWidgetClass, csg_window_form_widget, args, n);
    XtAddCallback(csg_window.apply_button, XtNcallback,
                    Apply_Button_Callback, (XtPointer)&csg_window);

    /* Add the text label. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Ready");                       n++;
    XtSetArg(args[n], XtNwidth, apply_width);                   n++;
    XtSetArg(args[n], XtNfromVert, csg_paned_widget);           n++;
    XtSetArg(args[n], XtNfromHoriz, csg_window.apply_button);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);                    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);                   n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);                   n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);                n++;
    XtSetArg(args[n], XtNresizable, TRUE);                      n++;
    XtSetArg(args[n], XtNborderWidth, 0);                       n++;
    csg_window.text_label = XtCreateManagedWidget("textLabel",
                        labelWidgetClass, csg_window_form_widget, args, n);

    /*
    **  Add the text widget.
    */
    /* Need to figure out how big to make it. */
    n = 0;
    XtSetArg(args[n], XtNwidth, &label_width);  n++;
    XtSetArg(args[n], XtNheight, &label_height);n++;
    XtGetValues(csg_window.text_label, args, n);

    n = 0;
    csg_window.text_string[0] = '\0';
    XtSetArg(args[n], XtNwidth,
            csg_window_width - (int)apply_width - (int)label_width -
            4 * gap - 6);                                       n++;
    XtSetArg(args[n], XtNheight, label_height);                 n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);                n++;
    XtSetArg(args[n], XtNlength, ENTRY_STRING_LENGTH);          n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);               n++;
    XtSetArg(args[n], XtNstring, csg_window.text_string);       n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);           n++;
    XtSetArg(args[n], XtNfromVert, csg_paned_widget);           n++;
    XtSetArg(args[n], XtNfromHoriz, csg_window.text_label); n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);                    n++;
    XtSetArg(args[n], XtNright, XtChainRight);                  n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);                   n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);                n++;
    XtSetArg(args[n], XtNresizable, TRUE);                      n++;
    csg_window.text_widget = XtCreateManagedWidget("textEntry",
                        asciiTextWidgetClass, csg_window_form_widget, args, n);
    /* Add translations. */
    XtOverrideTranslations(csg_window.text_widget,
        XtParseTranslationTable(":<Key>Return: Apply_Button()"));

    /* Initialize the main viewing window. */
    CSG_View_Initialize();

    /* Add the expose callback for the view widget. */
    XtAddCallback(csg_window.view_widget, XtNexposeCallback,
                    Redraw_Main_View, &csg_window);

    /* Create all the menus for editing the tree. */
    Create_CSG_Tree_Menus();

    /* Set CSG button sensitivity. */
    if ( ! num_csg_base_objects )
        Set_CSG_Related_Sensitivity(FALSE);

    /* Realize it. */
    XtRealizeWidget(csg_window.shell);

#if ( ! MISSING_ATOMS )
    XSetWMProtocols(XtDisplay(csg_window.shell), XtWindow(csg_window.shell), 
                    &delete_atom, 1);
#endif
}


/*  Widget
**  Create_CSG_Buttons(int gap)
**  Creates all the command buttons for the csg window.
**  Any button (the first) is returned.
*/
static Widget
Create_CSG_Buttons(int gap)
{
    Arg         args[15];
    int         n;


    /* Create the menus for menuButton popups. */
    Create_CSG_Menu();
    Create_Object_Menu();
    Create_View_Menu(csg_window.shell, &csg_window);
    Create_Window_Menu(csg_window.shell, &csg_window);
    Create_Edit_Menu(&csg_window);
    Layers_Create_Menu(&csg_window);

    /* All the option_buttons have the same chaining, so just set it once. */
    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);   n++;
    XtSetArg(args[n], XtNresizable, TRUE);      n++;


    /* A CSG menu button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "CSG");         n++;
    XtSetArg(args[n], XtNfromVert, NULL);       n++;
    XtSetArg(args[n], XtNmenuName, "CSGMenu");  n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    option_buttons[option_count] = XtCreateManagedWidget("csgButton",
                menuButtonWidgetClass, csg_window_form_widget, args, n);
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
                menuButtonWidgetClass, csg_window_form_widget, args, n);
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
                    menuButtonWidgetClass, csg_window_form_widget, args, n);
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
                    menuButtonWidgetClass, csg_window_form_widget, args, n);
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
                    menuButtonWidgetClass, csg_window_form_widget, args, n);
    option_count++;

    /* A reset button. */
    n = 5;
    XtSetArg(args[n], XtNlabel, "Clear CSG");                       n++;
    XtSetArg(args[n], XtNfromVert, option_buttons[option_count-1]); n++;
    XtSetArg(args[n], XtNvertDistance, 5*gap);                      n++;
    option_buttons[option_count] = XtCreateManagedWidget("resetButton",
                    commandWidgetClass, csg_window_form_widget, args, n);
    XtAddCallback(option_buttons[option_count], XtNcallback, Clear_Dialog_Func,
                    (XtPointer)&csg_window);
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
    csg_window.edit_menu->button = option_buttons[option_count] =
        XtCreateManagedWidget("editButton",
                menuButtonWidgetClass, csg_window_form_widget, args, n);
    option_count++;

    return option_buttons[option_count - 1];

}


/*
**  static void
**  Close_CSG_Callback(Widget w, XtPointer cl, XtPointer ca)
**  Just unmaps the window - iconifying it would be just as useful.
*/
static void
Close_CSG_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XUnmapWindow(XtDisplay(csg_window.shell), XtWindow(csg_window.shell));
}


/*  void
**  Create_CSG_Menu()
**  Creates the menu which pops up from the CSG button.
*/
static void
Create_CSG_Menu()
{
    Widget  menu_widget;
    Widget  close_button;

    menu_widget = XtCreatePopupShell("CSGMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);

    csg_modify_button = XtCreateManagedWidget("Modify",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(csg_modify_button, XtNcallback,
                  CSG_Modify_Existing_Callback, NULL);

    csg_copy_button = XtCreateManagedWidget("Copy",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(csg_copy_button, XtNcallback,
                  CSG_Copy_Existing_Callback, NULL);

    csg_delete_button = XtCreateManagedWidget("Delete",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(csg_delete_button, XtNcallback,
                  CSG_Delete_Existing_Callback, NULL);

    csg_export_button = XtCreateManagedWidget("Export",
                            smeBSBObjectClass, menu_widget, NULL, 0);
    XtAddCallback(csg_export_button, XtNcallback, CSG_Export_Callback, NULL);

    close_button = XtCreateManagedWidget("Close", smeBSBObjectClass,
                                                    menu_widget, NULL, 0);
    XtAddCallback(close_button, XtNcallback, Close_CSG_Callback, NULL);
}

/*  void
**  Create_Object_Menu()
**  Creates the menu which pops up from the Edit button.
**  Functions included: Shape, Move, Copy, Name, Attributes
*/
static void
Create_Object_Menu()
{
    Widget  object_widget;
    Widget  object_children[8];

    int     count = 0;


    object_widget = XtCreatePopupShell("ObjectMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);

    object_children[count] = XtCreateManagedWidget("New", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                    New_Object_Popup_Callback, (XtPointer)&csg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Edit", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Edit_Objects_Function,
                    (XtPointer)&csg_window);
    count++;

    XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

    object_children[count] = XtCreateManagedWidget("Name", smeBSBObjectClass,
                                                    object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Rename_Callback,
                  (XtPointer)&csg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Attributes",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Set_Attributes_Callback,
                    (XtPointer)&csg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Alias",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                  Alias_Object_Alias_Callback, (XtPointer)&csg_window);
    count++;

    XtCreateManagedWidget("line1", smeLineObjectClass, object_widget, NULL, 0);

    object_children[count] = XtCreateManagedWidget("Dense Wire",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                  Wireframe_Denser_Callback, (XtPointer)&csg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Thin Wire",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback,
                  Wireframe_Thinner_Callback, (XtPointer)&csg_window);
    count++;

    object_children[count] = XtCreateManagedWidget("Change Base",
                                smeBSBObjectClass, object_widget, NULL, 0);
    XtAddCallback(object_children[count], XtNcallback, Base_Change_Callback,
                    (XtPointer)&csg_window);
    count++;
}


static void
Create_Common_Menu_Widgets(Widget parent)
{
    Widget  current;

    current = XtCreateManagedWidget("Copy", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Copy_Callback, NULL);
                                
    current = XtCreateManagedWidget("Delete", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Delete_Callback, NULL);
}

static Widget
Create_Root_Menu_Widgets(Widget parent)
{
    Widget  result;
    Widget  current;

    result =
    current = XtCreateManagedWidget("Display", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Display_Callback, NULL);
                                
    current = XtCreateManagedWidget("Move", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, Init_Motion_Sequence,
                  (XtPointer)csg_move);
                                
    current = XtCreateManagedWidget("Attach", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, Init_Motion_Sequence,
                  (XtPointer)csg_attach);

    XtCreateManagedWidget("menuLine", smeLineObjectClass, parent, NULL, 0);
                                
    current = XtCreateManagedWidget("Complete", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Complete_Callback,(XtPointer)FALSE);

    return result;
}

static void
Create_Internal_Menu_Widgets(Widget parent)
{
    Widget  current;

    current = XtCreateManagedWidget("ReOrder", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Reorder_Callback, NULL);

    current = XtCreateManagedWidget("Preview", smeBSBObjectClass, parent,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Preview_Callback, NULL);
}


static void
Create_Internal_Root_Menu()
{
    Widget  menu_widg;

    menu_widg = XtCreatePopupShell("InternalRootMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);
    csg_display_label[0] =
    Create_Root_Menu_Widgets(menu_widg);
    XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
    Create_Internal_Menu_Widgets(menu_widg);
    XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
    Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_Internal_Menu()
{
    Widget  menu_widg;
    Widget  current;

    menu_widg = XtCreatePopupShell("InternalMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);
    Create_Internal_Menu_Widgets(menu_widg);
    XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
    current = XtCreateManagedWidget("Evaluate", smeBSBObjectClass, menu_widg,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Complete_Callback, (XtPointer)TRUE);
    current = XtCreateManagedWidget("Break", smeBSBObjectClass, menu_widg,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Break_Callback, NULL);
    Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_External_Root_Menu()
{
    Widget  menu_widg;

    menu_widg = XtCreatePopupShell("ExternalRootMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);
    csg_display_label[1] =
    Create_Root_Menu_Widgets(menu_widg);
    XtCreateManagedWidget("menuLine", smeLineObjectClass, menu_widg, NULL, 0);
    Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_External_Menu()
{
    Widget  menu_widg;
    Widget  current;

    menu_widg = XtCreatePopupShell("ExternalMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);
    current = XtCreateManagedWidget("Break", smeBSBObjectClass, menu_widg,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, CSG_Break_Callback, NULL);
    Create_Common_Menu_Widgets(menu_widg);
}

static void
Create_Attach_Menu()
{
    Widget  menu_widg;
    Widget  current;

    menu_widg = XtCreatePopupShell("AttachMenu", simpleMenuWidgetClass,
                                        csg_window.shell, NULL, 0);

    current = XtCreateManagedWidget("Union", smeBSBObjectClass, menu_widg,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, Combine_Menu_Callback,
                  (XtPointer)csg_union_op);

    current = XtCreateManagedWidget("Intersection", smeBSBObjectClass,
                                    menu_widg, NULL,0);
    XtAddCallback(current, XtNcallback, Combine_Menu_Callback,
                  (XtPointer)csg_intersection_op);

    current = XtCreateManagedWidget("Difference", smeBSBObjectClass,
                                    menu_widg, NULL, 0);
    XtAddCallback(current, XtNcallback, Combine_Menu_Callback,
                  (XtPointer)csg_difference_op);

    current = XtCreateManagedWidget("Cancel", smeBSBObjectClass, menu_widg,
                                    NULL, 0);
    XtAddCallback(current, XtNcallback, Cancel_Combine_Menu_Callback, NULL);

}


static void
Create_CSG_Tree_Menus()
{
    /* There are different menus depending on what sort of node it is and
    ** where it is in the tree. */
    Create_Internal_Root_Menu();
    Create_Internal_Menu();
    Create_External_Root_Menu();
    Create_External_Menu();
    Create_Attach_Menu();

}


/*  void
**  CSG_View_Initialize()
**  Initializes the csg viewport.
*/
static void
CSG_View_Initialize()
{

    /* Set up the axes. */
    Window_Axes_Initialize(&csg_window);

    /* Install translations. */
    XtOverrideTranslations(csg_window.view_widget,
        XtParseTranslationTable(selection_string));

}


/*  void
**  Sensitize_CSG_Buttons(Boolean state)
**  A function to sensitize or desensitize all the buttons.
*/
void
Sensitize_CSG_Buttons(Boolean state, int mask) 
{
    Arg arg;
    int i;

    if ( ! csg_window.shell ) return;

    XtSetArg(arg, XtNsensitive, state);
    for ( i = 0 ; i < option_count - 1 ; i++ )
        if ( mask & ( 1 << i ) )
            XtSetValues(option_buttons[i], &arg, 1);

    if ( csg_window.edit_menu->num_children != 0 && ( mask & SenCSGEdit ) )
        XtSetValues(csg_window.edit_menu->button, &arg, 1);

    if ( mask & SenCSGTree )
        XtSetSensitive(csg_tree_widget, state);
}



/*  void
**  Set_CSG_Related_Sensitivity(Boolean state)
**  Sets the state of buttons which are only relevent if CSG objects are
**  defined.
*/
void
Set_CSG_Related_Sensitivity(Boolean state)
{
    Arg arg;
    XtSetArg(arg, XtNsensitive, state);
    if ( csg_modify_button )
    {
        XtSetValues(csg_modify_button, &arg, 1);
        XtSetValues(csg_copy_button, &arg, 1);
        XtSetValues(csg_delete_button, &arg, 1);
        XtSetValues(csg_export_button, &arg, 1);
    }
    if ( new_csg_button )
        XtSetValues(new_csg_button, &arg, 1);
}


