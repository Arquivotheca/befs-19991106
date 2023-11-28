#
# Makefile for the linux BeOS filesystem routines.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

O_TARGET := befs.o
O_OBJS   := dir.o file.o inode.o namei.o super.o index.o debug.o symlink.o \
            util.o
M_OBJS   := $(O_TARGET)

include $(TOPDIR)/Rules.make
