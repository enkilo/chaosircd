# check for SQLite
# ------------------------------------------------------------------
AC_DEFUN([AC_CHECK_SQLITE],
[

ac_cv_with_sqlite="/usr /usr/local"
AC_ARG_WITH(sqlite,
    [  --with-sqlite=DIR        support for SQLite],
    [ case "$withval" in
         yes) ;;
         no) ac_cv_with_sqlite="no" ;;
         *) ac_cv_with_sqlite="$withval" ;;
     esac ])

if test "$ac_cv_with_sqlite" != no; then
  AC_MSG_CHECKING(for SQLite headers)

  for i in $ac_cv_with_sqlite; do
    if test -r "$i/include/sqlite/sqlite3.h"; then
      SQLITE_DIR=$i
      SQLITE_INC_DIR=$i/include/sqlite
    elif test -r "$i/include/sqlite3.h"; then
      SQLITE_DIR=$i
      SQLITE_INC_DIR=$i/include
    fi
    test -d "$SQLITE_INC_DIR" && break
  done

  if test ! -d "$SQLITE_INC_DIR"; then
    AC_MSG_RESULT([not found])
    with_sqlite=no
#    FAIL_MESSAGE("SQLite Headers", "$SQLITE_DIR $SQLITE_INC_DIR")
  else
    AC_MSG_RESULT([$SQLITE_INC_DIR])
  fi

  AC_MSG_CHECKING([for SQLite client library])

  for i in lib64 lib lib64/sqlite lib/sqlite; do
    str="$SQLITE_DIR/$i/libsqlite3.*"
    for j in `echo $str`; do
      if test -r $j; then
        SQLITE_LIB_DIR="$SQLITE_DIR/$i"
      fi
      test -d "$SQLITE_LIB_DIR" && break
    done
    test -d "$SQLITE_LIB_DIR" && break
  done

  if test -z "$SQLITE_LIB_DIR"; then
    if test "$sqlite_fail" != no; then
      AC_MSG_RESULT(not found)
      with_sqlite=no
#      FAIL_MESSAGE("sqlite library",
#                   "$SQLITE_LIB_DIR")
    else
      AC_MSG_RESULT(no)
    fi
  else
    AC_MSG_RESULT($SQLITE_LIB_DIR)
    SQLITE=true
    SQLITE_LIBS="-L${SQLITE_LIB_DIR} -Wl,-rpath,${SQLITE_LIB_DIR} -lsqlite3"
    SQLITE_CFLAGS="${SQLITE_INC_DIR:+-I$SQLITE_INC_DIR}"
    AC_CHECK_LIB(z, compress)
    DBSUPPORT="$DBSUPPORT SQLite"
    AC_DEFINE_UNQUOTED(HAVE_SQLITE, "1", [Define this if you have SQLite])
  fi
fi
AM_CONDITIONAL([SQLITE],[test "$SQLITE" = true])
AC_SUBST([SQLITE_LIBS])
AC_SUBST([SQLITE_CFLAGS])
])

