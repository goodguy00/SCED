/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/preferences.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: preferences.c,v $
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
**  preferences.c: Managment of preferences dialog box.
*/

#include <sced.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Toggle.h>

static void Preferences_Create_Shell();
static void Preferences_Set_Values(ScedPreferencesPtr);

static Widget   preferences_shell;

static Widget   autosave_text;
static Widget   gzip_toggle;
static Widget   save_simple_toggle;
static Widget   scene_path_text;


void
Preferences_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ! preferences_shell )
        Preferences_Create_Shell();

    Preferences_Set_Values(&sced_preferences);

    Position_Shell(preferences_shell, FALSE);
    XtPopup(preferences_shell, XtGrabExclusive);
}


static void
Preferences_Set_Values(ScedPreferencesPtr prefs)
{
    char    temp_s[10];

    sprintf(temp_s, "%d", prefs->autosave_time);
    XtVaSetValues(autosave_text, XtNstring, temp_s, NULL);

    XtVaSetValues(gzip_toggle, XtNstate, prefs->compress_output, NULL);

    XtVaSetValues(save_simple_toggle, XtNstate, prefs->save_simple_wires, NULL);

    XtVaSetValues(scene_path_text, XtNstring, prefs->scene_path, NULL);
}


static void
Preferences_Get_Values(ScedPreferencesPtr prefs)
{
    char    *temp_s;

    XtVaGetValues(autosave_text, XtNstring, &temp_s, NULL);
    sscanf(temp_s, "%d", &(prefs->autosave_time));

    XtVaGetValues(gzip_toggle, XtNstate, &(prefs->compress_output), NULL);

    XtVaGetValues(save_simple_toggle, XtNstate, &(prefs->save_simple_wires),
                  NULL);

    XtVaGetValues(scene_path_text, XtNstring, &temp_s, NULL);
    prefs->scene_path = Strdup(temp_s);
}


static void
Preferences_Save_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    ScedPreferences preferences;

    Preferences_Get_Values(&preferences);
    preferences.default_attributes = sced_preferences.default_attributes;
    Defaults_Save_Preferences(&preferences);
    free(preferences.scene_path);
}


static void
Preferences_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(preferences_shell);
    free(sced_preferences.scene_path);
    Preferences_Get_Values(&sced_preferences);

    Save_Auto_Cancel();
    Save_Auto_Startup();
}


static void
Preferences_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(preferences_shell);
}


static void
Preferences_Create_Shell()
{
    Widget  form;
    Arg     args[15];
    int     n, m;
    Widget  labels[3];
    Widget  def, pt, spot, area, done, cancel, save;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Preferences");     n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    preferences_shell = XtCreatePopupShell("preferencesShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("preferencesForm", formWidgetClass,
                                 preferences_shell, args, n);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;

    n = m;
    XtSetArg(args[n], XtNlabel, "Autosave every");  n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    labels[0] = XtCreateManagedWidget("autoLabel1", labelWidgetClass, form,
                                      args, n);

    n = m;
    XtSetArg(args[n], XtNfromHoriz, labels[0]);     n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNstring, "0"HPFIX);         n++;
    autosave_text = XtCreateManagedWidget("saveText", asciiTextWidgetClass,
                                          form, args, n);
    XtOverrideTranslations(autosave_text,
            XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    n = m;
    XtSetArg(args[n], XtNlabel, "minutes");         n++;
    XtSetArg(args[n], XtNfromHoriz, autosave_text); n++;
    XtSetArg(args[n], XtNborderWidth, 0);           n++;
    XtCreateManagedWidget("minutesLabel", labelWidgetClass, form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Compress Output"); n++;
    XtSetArg(args[n], XtNfromVert, autosave_text);  n++;
    gzip_toggle = XtCreateManagedWidget("gzipToggle", toggleWidgetClass, form,
                                        args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Save Simpified Wireframes");   n++;
    XtSetArg(args[n], XtNfromVert, gzip_toggle);                n++;
    save_simple_toggle = XtCreateManagedWidget("simpleToggle",
                                toggleWidgetClass, form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Scene Path:");         n++;
    XtSetArg(args[n], XtNfromVert, save_simple_toggle); n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    labels[2] = XtCreateManagedWidget("autoLabel3", labelWidgetClass, form,
                                      args, n);

    n = m;
    XtSetArg(args[n], XtNfromVert, save_simple_toggle); n++;
    XtSetArg(args[n], XtNfromHoriz, labels[2]);         n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);   n++;
    scene_path_text = XtCreateManagedWidget("sceneText", asciiTextWidgetClass,
                                            form, args, n);
    XtOverrideTranslations(scene_path_text,
            XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    n = m;
    XtSetArg(args[n], XtNlabel, "Default Attributes");  n++;
    XtSetArg(args[n], XtNfromVert, scene_path_text);    n++;
    def = XtCreateManagedWidget("defButton", commandWidgetClass, form, args, n);
    XtAddCallback(def, XtNcallback, Attributes_Set_Defaults_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Default Pointlight");  n++;
    XtSetArg(args[n], XtNfromVert, scene_path_text);    n++;
    XtSetArg(args[n], XtNfromHoriz, def);               n++;
    pt = XtCreateManagedWidget("ptButton", commandWidgetClass, form, args, n);
    XtAddCallback(pt, XtNcallback, Pointlight_Set_Defaults_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Default Spotlight");   n++;
    XtSetArg(args[n], XtNfromVert, scene_path_text);    n++;
    XtSetArg(args[n], XtNfromHoriz, pt);                n++;
    spot =
        XtCreateManagedWidget("spotButton", commandWidgetClass, form, args, n);
    XtAddCallback(spot, XtNcallback, Spotlight_Set_Defaults_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Default Arealight");   n++;
    XtSetArg(args[n], XtNfromVert, scene_path_text);    n++;
    XtSetArg(args[n], XtNfromHoriz, spot);              n++;
    area =
        XtCreateManagedWidget("areaButton", commandWidgetClass, form, args, n);
    XtAddCallback(area, XtNcallback, Arealight_Set_Defaults_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Done");    n++;
    XtSetArg(args[n], XtNfromVert, def);    n++;
    done = XtCreateManagedWidget("doneButton", commandWidgetClass, form,
                                 args, n);
    XtAddCallback(done, XtNcallback, Preferences_Done_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Save");    n++;
    XtSetArg(args[n], XtNfromVert, def);    n++;
    XtSetArg(args[n], XtNfromHoriz, done);  n++;
    save = XtCreateManagedWidget("savebutton", commandWidgetClass, form,
                                 args, n);
    XtAddCallback(save, XtNcallback, Preferences_Save_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");  n++;
    XtSetArg(args[n], XtNfromVert, def);    n++;
    XtSetArg(args[n], XtNfromHoriz, save);  n++;
    cancel = XtCreateManagedWidget("cancelbutton", commandWidgetClass, form,
                                   args, n);
    XtAddCallback(cancel, XtNcallback, Preferences_Cancel_Callback, NULL);

    XtRealizeWidget(preferences_shell);
}



