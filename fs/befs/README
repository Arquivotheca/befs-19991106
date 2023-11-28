BeOS filesystem for Linux  1999-11-06


WARNING
=======
Make sure you understand that this is alpha software.  This means that the
implementation is neither complete nor well-tested.  So using this driver,
by own risk.

WHAT'S THIS DRIVER
==================
This module is BeOS filesystem (using filesystem of BeOS operating system)
driver for Linux 2.3.25 later. If you want to use on 2.2.x kernel, please
use old version.

Wich BFS or BEFS
================
Be, Inc said, "BeOS filesystem is called BFS by official, but doesn't
 called BeFS".  But Unixware boot filesystem is called bfs, too.
iSince conflict occur, on Linux, BeOS filesystem is befs.

HOW TO INSTALL
==============
step 1.  Install sources code to source code tree of linux.

  i.e.
    cd /usr/src/linux
    tar ~/befs-19991106.tar.gz
    patch -p1 < linux-2.3.x.patch

step 2.  Configuretion & make kernel

  i.e. 
    make config
    make dep; make clean
    make bzImage
    make modules

  Note that at 'BeOS Filesystem support', you should answer y(es) ori
  m(odule).  If you want to use BeOS filesystem of other platforms , you
  should answer y(es) aginst 'BeOS filesystem of other platforms support'.

step 3.  Install

  i.e.
    cp /usr/src/arch/i386/boot/zImage /vmlinuz
    make modules_install

USING BFS
=========
To use the BeOS filesystem, use filesystem type 'befs'.

ex)
    mount -t befs -o type=x86,iocharset=iso8859-1 /dev/fd0 /beos

MOUNT OPTIONS
=============
uid=nnn             All files in the partition will be owned by user id nnn.
gid=nnn	            All files in the partition will be in group nnn.
type=nnn            Set filesystem type.  nnn is x86 or ppc. Default value is
                    platform depends (linux-x86 is x86, linux-ppc is ppc).
iocharset=nnn       charactor-set. But not support DBCS.  

KNOWLEDGE ISSUE
===============
o Current implement supports read-only.

HOW TO GET LASTEST VERSION
==========================
The source and more information can be found in

  http://hp.vector.co.jp/authors/VA008030/bfs/

SPECIAL THANKS
==============
Dominic Giampalo ... Writing "Practical file system design with Be file
                     system"
Hiroyuki Yamada  ... Testing LinuxPPC.

AUTHOR
======
Makoto Kato <m_kato@ga2.so-net.ne.jp>
