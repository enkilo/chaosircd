# $Id: program.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Dynamic program configuration
#
# Copyleft GPL (c) 2005 by Roman Senn <smoli@paranoya.ch>
#


# AC_GEN_PROGRAM_VAR([PROGRAM], [VARNAME], [PATTERN], [SED-SCRIPT])
# 
# generates shell script that lists files in subdir and puts them
# in a Makefile variable
# ---------------------------------------------------------------------------
AC_DEFUN([AC_GEN_PROGRAM_VAR],
[$2=\`ls \$srcdir/$1/$3 2>/dev/null | sed -e 's,^.*/,,' | sed -e '$4' | sed -e 's,$, \\\\\\\\,' $5\`
])

AC_DEFUN([AC_CONFIG_PROGRAMS],
[# test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured, run \"make distclean\" there.])
fi
m4_define([AC_PROGRAM_LIST], [m4_normalize($1)])dnl
dnl m4_define([AC_PROGRAMS], [m4_split(AC_PROGRAM_LIST)])
dnl echo AC_PROGRAM_LIST
m4_append([AC_OUTPUT_COMMANDS_POST], [
  ./$CONFIG_PROGRAM $config_library_args
])
m4_foreach([BIN], m4_split(AC_PROGRAM_LIST),
[config_files="$config_files BIN/Makefile"
dnl m4_define([BINFILE], BIN)
dnl m4_append([BINFILE], [[/]Makefile])
dnl AC_CONFIG_FILES(BINFILE)
])
PROGRAMS="AC_PROGRAM_LIST"
AC_GEN_CONFIG_PROGRAM([$1], [$2])
AC_SUBST(PROGRAMS)])

# generates config.bin script
AC_DEFUN([AC_GEN_CONFIG_PROGRAM],
[CONFIG_PROGRAM="config.bin"
AC_MSG_NOTICE([creating ./$CONFIG_PROGRAM])
cat > $CONFIG_PROGRAM << M4EOF
#! /bin/sh
# \$Id: program.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# Generated by $as_me.
# Run this file to recreate the current program configuration.
PROGRAMS="$PROGRAMS"
DEBUG="$DEBUG"
srcdir="."

if test "\$[1]" = "--srcdir"; then
  shift
  srcdir="\$[1]"
  shift
fi

if test "\$[1]" = "--silent-rules"; then
  exec 1>/dev/null
  shift
fi

echo -n "$CONFIG_PROGRAM: configuring..."

if test "\$[*]"; then
  for program in "\$[*]"
  do
    case \$program in
      m4_foreach([BIN], m4_split(AC_PROGRAM_LIST), BIN[)][
dnl        [output_]BIN="yes"
        BUILD="\$BUILD BIN"
        ;;
      ])[*][)]
        ;;
    esac
  done
else
  BUILD="\$PROGRAMS"
fi
for bin in \$BUILD
do
  name=\`echo \$bin | sed 's,.*/,,'\`
  dir=\`echo \$bin | sed 's,/.*,,'\`
  
  if test "\$DEBUG" = "yes"; then
    PATTERN='^$'
  else
    PATTERN='^debug_'
  fi
  
AC_GEN_PROGRAM_VAR([\${dir}], [MODULES], [*.c], [s,\.c$,,])
AC_GEN_PROGRAM_VAR([\${dir}], [OBJECTS], [*.c], [s,\.c$,\.o,], [ | grep -v "\$PATTERN"])
AC_GEN_PROGRAM_VAR([\${dir}], [ASM], [*.c], [s,\.c$,\.s,], [ | grep -v "\$PATTERN"])
AC_GEN_PROGRAM_VAR([\${dir}], [DEPS], [*.c], [s,\.c$,\.d,])
AC_GEN_PROGRAM_VAR([\${dir}], [HEADERS], [*.h], [])
AC_GEN_PROGRAM_VAR([\${dir}], [SOURCES], [*.c], [])

DEPFILES=\`ls \$srcdir/\$dir/*.c 2>/dev/null | sed -e 's,^.*/,,' | sed -e 's,\.c$,\.d,'\`
DEPINC=\`ls \$srcdir/\$dir/*.c 2>/dev/null | sed -e 's,^.*/,,' | sed -e 's,\.c$,\.d,' | sed -e 's,^,include ,'\`

  LDADD="m4_foreach(LDARG, $2, [\\\$(top_builddir)/LDARG])"
  dir_suffix=\$dir
  builddir="."
  top_builddir=".."
    
  case \$srcdir in
    .)  # No --srcdir option.  We are building in place.
      top_srcdir=".."
      srcdir="."
      ;;
    [[\\/]]* | ?:[[\\/]]* )  # Absolute path.
      top_srcdir="\$srcdir"
      srcdir="\$srcdir/\$dir_suffix"
      ;;
    *) # Relative path.
      top_srcdir="\$top_builddir/\$srcdir"
      srcdir="\$top_builddir/\$srcdir/\$dir_suffix"
      ;;
  esac
      
# create build dir and initial deps
mkdir -p \$dir
(cd \$dir
 for mod in \$MODULES
 do
   echo \${mod}.o: \$srcdir/\${mod}.c > \${mod}.d
 done)

# create targets
TARGETS=""
for mod in \$MODULES
do
  TARGETS="\${TARGETS}#\${mod}.o: \${mod}.c
#       \\\$(COMPILE) -c -o \${mod}.o \${mod}.c

"
done

  cat > \$dir/Makefile << EOF
AC_BIN_MAKEFILE([\${dir}], [\${name}])
EOF
  echo -n " ${COLOR_DIR}\$dir${NC}"
done
echo
M4EOF
chmod +x $CONFIG_PROGRAM


if test -n "$srcdir"; then
  config_program_args="--srcdir $srcdir"
fi

#./$CONFIG_PROGRAM $config_program_args
])


AC_DEFUN([AC_BIN_MAKEFILE], 
[# \$Id: program.m4,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# Makefile for program subdirectory
#
# relative directories
# ---------------------------------------------------------------------------
srcdir       = \$srcdir
top_srcdir   = \$top_srcdir
top_builddir = \$top_builddir

VPATH        = \\\$(srcdir)

# configuration for this directory
# ---------------------------------------------------------------------------
PROGRAM = [$2]

DEPS    = \$DEPS

SOURCES = \$SOURCES

OBJECTS = \$OBJECTS

HEADERS = \$HEADERS

LDADD   = \$LDADD

# targets for this directory
# ---------------------------------------------------------------------------
bin_EXEC = \\\$(PROGRAM)
all: Makefile \\\$(bin_EXEC)
install: install-exec

Makefile: \\\$(top_builddir)/config.bin
	@\\\$(ECHO_CONFIG)
	\\\$(top_builddir)/config.bin 

# dependencies
# ---------------------------------------------------------------------------
\$DEPINC

# targets
# ---------------------------------------------------------------------------
\$TARGETS

# include build.mk (makefile helper)
# ---------------------------------------------------------------------------
include \\\$(top_builddir)/build.mk
])
