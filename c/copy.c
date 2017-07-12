/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/copy.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: copy.c,v $
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
**  copy.c : Functions to handle copying objects.
*/

#include <sced.h>
#include <hash.h>
#include <instance_list.h>
#include <layers.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>

#define CONTINUE    0
#define NORMAL      1
#define SYSTEM      2
#define CANCEL      3

static void Copy_Object_List(WindowInfoPtr, InstanceList);
static void Copy_Create_Shell();

static Widget   copy_objects_shell = NULL;
static Widget   copy_dialog;

static int  finished;

void
Copy_Objects_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtAppContext    context;
    XEvent          event;
    char            *count_string;
    int             count = 1;
    int             i;

    if ( ! copy_objects_shell )
        Copy_Create_Shell();

    finished = CONTINUE;

    XtVaSetValues(copy_dialog, XtNvalue, "1", NULL);

    Position_Shell(copy_objects_shell, FALSE);
    XtPopup(copy_objects_shell, XtGrabExclusive);
        
    context = XtWidgetToApplicationContext(main_window.shell);
    while ( finished == CONTINUE )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }

    XtPopdown(copy_objects_shell);

    count_string = XawDialogGetValueString(copy_dialog);
    sscanf(count_string, "%d", &count);

    if ( finished == NORMAL )
        for ( i = 0 ; i < count ; i++ )
            Copy_Object_List((WindowInfoPtr)cl,
                             ((WindowInfoPtr)cl)->selected_instances);
    else if ( finished == SYSTEM )
        for ( i = 0 ; i < count ; i++ )

            Copy_Object_System((WindowInfoPtr)cl,
                               ((WindowInfoPtr)cl)->selected_instances, TRUE);
    else if ( finished == CANCEL )
        return;
}


static void
Copy_Object_List(WindowInfoPtr window, InstanceList list)
{
    InstanceList    new_insts = NULL;

    for ( ; list ; list = list->next )
        if ( ! Edit_Obj_On_Stack(list->the_instance) )
            Insert_Element(&new_insts,Copy_Object_Instance(list->the_instance));

    Add_Instance_To_Edit(window, new_insts, FALSE);

    Free_Selection_List(new_insts);
}


static void
Copy_Switch_Ref(ConstraintSpecPtr spec, ObjectInstancePtr inst, void *ptr,
                void *ptr2, int i)
{
    ObjectInstancePtr   new_obj;
    HashTable           table = (HashTable)ptr;

    if ( ! Spec_Is_Dependent(spec->spec_type) )
        return;

    if ( ( new_obj =
        (ObjectInstancePtr)Hash_Get_Value(table,
                            (unsigned long)(spec->spec_data))) != (void*)-1 )
    {
        Dependencies_Remove_Object(spec, inst, NULL, NULL, 0);
        spec->spec_data = (void*)new_obj;
        Add_Dependency(new_obj, inst);
    }
}

void
Copy_Object_System(WindowInfoPtr window, InstanceList list, Boolean do_edit)
{
    InstanceList        new_insts = NULL;
    InstanceList        elmt, new_elmt;
    ObjectInstancePtr   new_obj;
    HashTable           object_hash = Hash_New_Table();

    for ( ; list ; list = list->next )
    {
        if ( ! Edit_Obj_On_Stack(list->the_instance) )
        {
            new_obj = Copy_Object_Instance(list->the_instance);
            Insert_Element(&new_insts, new_obj);
            Hash_Insert(object_hash, (unsigned long)(list->the_instance),
                        (void*)new_obj);
        }
    }

    /* Switch the constraints over. */
    for ( elmt = new_insts ; elmt ; elmt = elmt->next )
        Constraint_Manipulate_Constraints(elmt->the_instance,
                (void*)object_hash, NULL, Copy_Switch_Ref);

    if ( do_edit )
        Add_Instance_To_Edit(window, new_insts, FALSE);
    else
        for ( elmt = new_insts ; elmt ; elmt = elmt->next )
        {
            new_elmt = Append_Element(&(window->all_instances),
                                      elmt->the_instance);
            if ( Layer_Is_Visible(new_elmt->the_instance->o_layer) )
                new_elmt->the_instance->o_flags |= ObjVisible;
        }

    Free_Selection_List(new_insts);
    Hash_Free(object_hash);
}


static void
Copy_Shell_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    finished = (int)cl;
}


static void
Copy_Create_Shell()
{
    Arg     args[5];
    int     n;

    copy_objects_shell = XtCreatePopupShell("Copy",
                        transientShellWidgetClass, main_window.shell, NULL, 0);

    /* Create the dialog widget to go inside the shell. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Copy:");   n++;
    XtSetArg(args[n], XtNvalue, "");        n++;
    copy_dialog = XtCreateManagedWidget("copyDialog", dialogWidgetClass,
                        copy_objects_shell, args, n);

    /* Add the button at the bottom of the dialog. */
    XawDialogAddButton(copy_dialog, "Individual", Copy_Shell_Callback,
                        (XtPointer)NORMAL);
    XawDialogAddButton(copy_dialog, "System", Copy_Shell_Callback,
                        (XtPointer)SYSTEM);
    XawDialogAddButton(copy_dialog, "Cancel", Copy_Shell_Callback,
                        (XtPointer)CANCEL);

    XtOverrideTranslations(XtNameToWidget(copy_dialog, "value"),
        XtParseTranslationTable(":<Key>Return: Copy_Action()"));

    XtVaSetValues(XtNameToWidget(copy_dialog, "label"),
                  XtNborderWidth, 0, NULL);

    XtRealizeWidget(copy_objects_shell);
}


void
Copy_Action_Func(Widget w, XEvent *e, String *s, Cardinal *c)
{
    Copy_Shell_Callback(NULL, (XtPointer)NORMAL, NULL);
}
