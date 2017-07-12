# Generated automatically from Makefile.in by configure.
#
# $Header: /opt/sced/cvs/src/Makefile.in,v 1.1.1.1 1997/09/07 15:23:04 chipr Exp $
#
# $Log: Makefile.in,v $
# Revision 1.1.1.1  1997/09/07 15:23:04  chipr
# Original 1.0b2 sources
#
# Revision 1.0  1997/05/06 20:09:52  schenney
# Initial revision
#
#
SHELL=/bin/sh

.SUFFIXES:
.SUFFIXES: .c .o

srcdir=.
prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
mandir=${prefix}/man

INSTALL=/usr/bin/install -c

C_DIR=$(srcdir)/c
ELK_DIR=$(srcdir)/elk
H_DIR=$(srcdir)/h

TARGET=sced

# Set CC and CPP
CC=gcc
CPP=gcc -E

#	Use the following defs for flex.
LEX=flex
LEXLIB=-lfl

# Library bulding functions.
AR=ar
RANLIB=ranlib

# Put the location of you X header files here, if necessary.
# eg:
# XINCLUDE=-I/usr/openwin/include
XINCLUDE= -I/usr/X11R6/include

#
# Define where to find ELK
#
ELKINCLUDE=
ELKLIBS=

# Add any extra include directories to the end of this line, using -Idir
INCLUDE=-I$(srcdir)/h $(XINCLUDE) $(ELKINCLUDE)

# Add any extra C flags, such as -g for debug, here.
OTHER_FLAGS=

CFLAGS=$(OTHER_FLAGS) -O
ALL_CFLAGS=$(INCLUDE) $(CFLAGS)

# Add any extra libraries here.
LIBS=-lXaw -lXmu -lXext -lXt  -lSM -lICE -lX11  $(LEXLIB) $(ELKLIBS) -lm 

# Put the location of your X library files here, if necessary.
# eg:
# XLIBDIR=-L/usr/openwin/lib
XLIBDIR= -L/usr/X11R6/lib64

LDFLAGS=$(OPTIMISE) $(XLIBDIR) 

# Various support specific flags.
POVRAY_SRC=$(C_DIR)/povray.c
POVRAY_OBJ=$(C_DIR)/povray.o
RADIANCE_SRC=
RADIANCE_OBJ=
RAYSHADE_SRC=
RAYSHADE_OBJ=
RENDERMAN_SRC=
RENDERMAN_OBJ=
VRML_SRC=
VRML_OBJ=
PREV_SRC=$(C_DIR)/load_old.c
PREV_OBJ=$(C_DIR)/load_old.o

SRC=$(C_DIR)/ConstraintBox.c $(C_DIR)/ConstraintPt.c \
	$(C_DIR)/SimpleWire.c $(C_DIR)/Vectorlib.c $(C_DIR)/View.c \
	$(C_DIR)/add_constraint.c $(C_DIR)/add_dialogs.c \
	$(C_DIR)/agg_view.c $(C_DIR)/aggregate.c $(C_DIR)/alias.c \
	$(C_DIR)/align.c $(C_DIR)/attributes.c \
	$(C_DIR)/base_objects.c $(C_DIR)/base_select.c \
	$(C_DIR)/bezier.c \
	$(C_DIR)/bounds.c $(C_DIR)/camera.c \
	$(C_DIR)/constraint.c $(C_DIR)/constraint_list.c \
	$(C_DIR)/constraint_pt.c $(C_DIR)/constr_update.c \
	$(C_DIR)/control_pt.c \
	$(C_DIR)/conversions.c $(C_DIR)/copy.c $(C_DIR)/csg_combine.c \
	$(C_DIR)/csg_edit.c $(C_DIR)/csg_events.c \
	$(C_DIR)/csg_gen_wire.c $(C_DIR)/csg_intersect.c \
	$(C_DIR)/csg_reference.c \
	$(C_DIR)/csg_split.c \
	$(C_DIR)/csg_tree.c $(C_DIR)/csg_view.c $(C_DIR)/csg_wire.c \
	$(C_DIR)/defaults.c $(C_DIR)/delete.c $(C_DIR)/dense_wireframe.c \
	$(C_DIR)/dependency.c \
	$(C_DIR)/dfs.c $(C_DIR)/draw.c $(C_DIR)/edge_table.c \
	$(C_DIR)/edit.c $(C_DIR)/edit_extras.c \
	$(C_DIR)/edit_shell.c $(C_DIR)/edit_menu.c \
	$(C_DIR)/edit_undo.c $(C_DIR)/events.c \
	$(C_DIR)/export.c $(C_DIR)/features.c \
	$(C_DIR)/files.c $(C_DIR)/gen_wireframe.c \
	$(C_DIR)/hash.c \
	$(C_DIR)/instance_list.c $(C_DIR)/instances.c \
	$(C_DIR)/kd_tree.c \
	$(C_DIR)/layers.c $(C_DIR)/light.c $(C_DIR)/load.c \
	$(C_DIR)/load_internal.c $(C_DIR)/load_lex.l \
	$(PREV_SRC) \
	$(C_DIR)/load_simple.c $(C_DIR)/main_view.c \
	$(C_DIR)/maintain.c $(C_DIR)/misc.c $(C_DIR)/names.c \
	$(C_DIR)/new_object.c $(C_DIR)/new_view.c \
	$(C_DIR)/off.c \
	$(C_DIR)/opengl_out.c \
	$(C_DIR)/placement.c \
	$(POVRAY_SRC) \
	$(C_DIR)/preferences.c \
	$(C_DIR)/preview.c $(C_DIR)/quit.c \
	$(RADIANCE_SRC) \
	$(C_DIR)/radius.c \
	$(RAYSHADE_SRC) $(RENDERMAN_SRC) \
	$(C_DIR)/rotate.c $(C_DIR)/save.c \
	$(C_DIR)/scale.c $(C_DIR)/sced.c \
	$(C_DIR)/select_point.c $(C_DIR)/selection.c \
	$(C_DIR)/torus.c $(C_DIR)/triangle.c \
	$(C_DIR)/utils.c \
	$(C_DIR)/view_recall.c $(C_DIR)/viewport.c \
	$(VRML_SRC) \
	$(C_DIR)/wireframe.c $(C_DIR)/zoom.c


OBJS=$(C_DIR)/ConstraintBox.o $(C_DIR)/ConstraintPt.o \
	 $(C_DIR)/SimpleWire.o $(C_DIR)/Vectorlib.o $(C_DIR)/View.o \
	 $(C_DIR)/add_constraint.o $(C_DIR)/add_dialogs.o \
	 $(C_DIR)/agg_view.o $(C_DIR)/aggregate.o $(C_DIR)/alias.o \
	 $(C_DIR)/align.o $(C_DIR)/attributes.o \
	 $(C_DIR)/base_objects.o $(C_DIR)/base_select.o \
	 $(C_DIR)/bezier.o \
	 $(C_DIR)/bounds.o $(C_DIR)/camera.o \
	 $(C_DIR)/constraint.o $(C_DIR)/constraint_list.o \
	 $(C_DIR)/constraint_pt.o $(C_DIR)/constr_update.o \
	 $(C_DIR)/control_pt.o \
	 $(C_DIR)/conversions.o $(C_DIR)/copy.o  \
	 $(C_DIR)/csg_combine.o $(C_DIR)/csg_edit.o \
	 $(C_DIR)/csg_events.o \
	 $(C_DIR)/csg_gen_wire.o $(C_DIR)/csg_intersect.o \
	 $(C_DIR)/csg_reference.o \
	 $(C_DIR)/csg_split.o $(C_DIR)/csg_tree.o $(C_DIR)/csg_view.o \
	 $(C_DIR)/csg_wire.o \
	 $(C_DIR)/defaults.o $(C_DIR)/delete.o $(C_DIR)/dense_wireframe.o \
	 $(C_DIR)/dependency.o \
	 $(C_DIR)/dfs.o $(C_DIR)/draw.o $(C_DIR)/edge_table.o \
	 $(C_DIR)/edit.o $(C_DIR)/edit_extras.o \
	 $(C_DIR)/edit_shell.o $(C_DIR)/edit_menu.o $(C_DIR)/edit_undo.o \
	 $(C_DIR)/events.o \
	 $(C_DIR)/export.o $(C_DIR)/features.o \
	 $(C_DIR)/files.o $(C_DIR)/gen_wireframe.o \
	 $(C_DIR)/hash.o \
	 $(C_DIR)/instance_list.o $(C_DIR)/instances.o \
	 $(C_DIR)/kd_tree.o \
	 $(C_DIR)/layers.o \
	 $(C_DIR)/light.o $(C_DIR)/load.o \
	 $(C_DIR)/load_internal.o $(C_DIR)/load_lex.o \
	 $(PREV_OBJ) \
	 $(C_DIR)/load_simple.o $(C_DIR)/main_view.o \
	 $(C_DIR)/maintain.o $(C_DIR)/misc.o $(C_DIR)/names.o \
	 $(C_DIR)/new_object.o $(C_DIR)/new_view.o \
	 $(C_DIR)/off.o \
	 $(C_DIR)/opengl_out.o \
	 $(C_DIR)/placement.o \
	 $(POVRAY_OBJ) \
	 $(C_DIR)/preferences.o \
	 $(C_DIR)/preview.o $(C_DIR)/quit.o \
	 $(RADIANCE_OBJ) \
	 $(C_DIR)/radius.o \
	 $(RAYSHADE_OBJ) $(RENDERMAN_OBJ) \
	 $(C_DIR)/rotate.o $(C_DIR)/save.o $(C_DIR)/sced.o \
	 $(C_DIR)/scale.o $(C_DIR)/select_point.o $(C_DIR)/selection.o \
	 $(C_DIR)/torus.o $(C_DIR)/triangle.o \
	 $(C_DIR)/utils.o \
	 $(VRML_OBJ) \
	 $(C_DIR)/view_recall.o $(C_DIR)/viewport.o \
	 $(C_DIR)/wireframe.o $(C_DIR)/zoom.o

ELK_SRC=\
	 $(ELK_DIR)/callbacks.c \
	 $(ELK_DIR)/elk_aggregate.c \
	 $(ELK_DIR)/elk_view.c \
	 $(ELK_DIR)/evalstr.c \
	 $(ELK_DIR)/init.c \
	 $(ELK_DIR)/object3d.c \
	 $(ELK_DIR)/csg.c \
	 $(ELK_DIR)/view.c
ELK_OBJS=\
	 $(ELK_DIR)/callbacks.o \
	 $(ELK_DIR)/elk_aggregate.o \
	 $(ELK_DIR)/elk_view.o \
	 $(ELK_DIR)/evalstr.o \
	 $(ELK_DIR)/init.o \
	 $(ELK_DIR)/view.o \
	 $(ELK_DIR)/object3d.o \
	 $(ELK_DIR)/csg.o
#
# The configure program will set NO to either YES or NO.
# and thus this next make variable will become either ELK_LIB_YES
# or ELK_LIB_NO. 'sced' will only try to link with ELK_LIB_YES.
#
ELK_LIB_NO=$(ELK_DIR)/libElk.a


all: $(TARGET)

install: all
	${INSTALL} -d ${bindir}
	${INSTALL} sced ${bindir}
	${INSTALL} -d ${mandir}/man1
	${INSTALL} -m 644 docs/man/man1/sced.1 ${mandir}/man1

$(TARGET): $(OBJS) $(ELK_LIB_YES) Makefile
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(ELK_LIB_YES) $(LIBS)

purify: $(OBJS) $(ELK_LIB_YES) Makefile
	purify $(CC) $(LDFLAGS) -o $(TARGET).pure $(OBJS) $(ELK_LIB_YES) $(LIBS)

$(ELK_DIR)/libElk.a: $(ELK_OBJS)
	$(AR) rc $@ $(ELK_OBJS)
	$(RANLIB) $@

$(C_DIR)/load_lex.o: $(C_DIR)/load_lex.l
	$(LEX) -t $(C_DIR)/load_lex.l > $(C_DIR)/load_lex.c
	$(CC) $(ALL_CFLAGS) -c -o $(C_DIR)/load_lex.o $(C_DIR)/load_lex.c
	rm $(C_DIR)/load_lex.c

.c.o :
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

depend:
	makedepend -- $(ALL_CFLAGS) -- $(SRC)
	sed 's/^[a-zA-Z_]*\.o:/$$(C_DIR)\/&/' Makefile > Makefile.new
	mv Makefile.new Makefile
	
clean:
	rm -f $(OBJS) $(ELK_OBJS)

#
# dependencies not handled by makedepend
#
$(ELK_DIR)/callbacks.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/callbacks.o : $(srcdir)/h/View.h
$(ELK_DIR)/csg.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/csg.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/csg.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/csg.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/csg.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/csg.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/csg.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/elk_aggregate.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/elk_aggregate.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/elk_aggregate.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/elk_aggregate.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/elk_aggregate.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/elk_aggregate.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/elk_aggregate.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/elk_view.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/elk_view.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/elk_view.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/elk_view.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/elk_view.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/elk_view.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/elk_view.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/evalstr.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/evalstr.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/evalstr.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/evalstr.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/evalstr.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/evalstr.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/evalstr.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/init.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/init.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/init.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/init.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/init.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/init.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/init.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/object3d.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/object3d.o : $(srcdir)/h/edit.h
$(ELK_DIR)/view.o : $(ELK_DIR)/elk_private.h $(srcdir)/h/sced.h
$(ELK_DIR)/view.o : $(srcdir)/h/base_objects.h $(srcdir)/h/csg.h
$(ELK_DIR)/view.o : $(srcdir)/h/instance_list.h $(srcdir)/h/config.h
$(ELK_DIR)/view.o : $(srcdir)/h/defs.h $(srcdir)/h/enum.h
$(ELK_DIR)/view.o : $(srcdir)/h/types.h $(srcdir)/h/globals.h
$(ELK_DIR)/view.o : $(srcdir)/h/protos.h $(srcdir)/h/macros.h
$(ELK_DIR)/view.o : $(srcdir)/h/Vector.h $(srcdir)/h/aggregate.h
$(ELK_DIR)/view.o : $(srcdir)/h/View.h
