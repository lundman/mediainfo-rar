AC_INIT([mediainfo-rar],[1.4.0], [bug-report@lundman.net])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC

AC_CHECK_PROG(BUILD, build, build)

# Run ./configure in unrar
AC_CONFIG_SUBDIRS([unrar-3.7.8-seek])

AC_TYPE_SIGNAL

AC_HEADER_TIME

AC_CHECK_HEADERS([errno.h])

AC_SYS_LARGEFILE

AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([socket], [socket inet])
AC_SEARCH_LIBS([dlopen], [dl])

AC_CHECK_FUNC([snprintf],
                [AC_DEFINE(HAVE_SNPRINTF,1,[Presence of snprintf found])],
                [AC_MSG_WARN(Adding internal snprintf function)])

AC_CHECK_FUNC([strcasestr],
                [AC_DEFINE(HAVE_STRCASESTR,1,[Presence of strcasestr found])],
                [])

if test "x$ac_cv_prog_gcc" = xyes ; then
   CFLAGS="$CFLAGS -Wall -g "
fi

AC_CHECK_FUNCS([strtoul strtouq strtoull strtoll])

AC_ARG_WITH(mediainfo,
  [  --with-mediainfo=PATH      libMediaInfo headers and libraries])

dnl static build?
AC_ARG_ENABLE(mediainfo-static,
   [  --enable-mediainfo-static         Don't load library dynamically  [[default=no ]] ],
   [
    AC_DEFINE(MEDIAINFO_STATIC,1,[Compile in statically, no runtime loading])
    AC_CHECK_HEADERS(MediaInfoDLL/MediaInfoDLL_Static.h)
   ],
   [
    AC_CHECK_HEADERS(MediaInfoDLL/MediaInfoDLL.h)
   ]
   )


AC_SEARCH_LIBS(gzopen, z,,
                           AC_MSG_ERROR(Cannot find lib libz),
                           )

AC_SEARCH_LIBS(MediaInfo_New, mediainfo,
                           EXTRA_LIBS="",
                           AC_MSG_RESULT([Trying again with libzen]),
                           )

if test $ac_cv_search_MediaInfo_New == no; then

  unset ac_cv_search_MediaInfo_New

  AC_SEARCH_LIBS(MediaInfo_New, mediainfo,
                           EXRA_LIBS="-lzen",
                           AC_MSG_RESULT([Trying again with libzen libstd++]),
                           [-lzen])

fi

if test $ac_cv_search_MediaInfo_New == no; then
  unset ac_cv_search_MediaInfo_New

  AC_SEARCH_LIBS(MediaInfo_New, mediainfo,
                           EXTRA_LIBS="-lzen -lstdc++",
                           AC_MSG_RESULT([Failed to link with libmediainfo]),
                           [-lzen -lstdc++])
fi

if test $ac_cv_search_MediaInfo_New == no; then
  unset ac_cv_search_MediaInfo_New

  AC_SEARCH_LIBS(MediaInfo_New, mediainfo,
                           EXTRA_LIBS="-lzen -lstdc++ -lpthread",
                           AC_MSG_ERROR([Failed to link with libmediainfo]),
                           [-lzen -lstdc++ -lpthread])
fi

AC_SEARCH_LIBS(DVDOpen, dvdread,,
                        EXTRA_LIBS="$EXTRA_LIBS -ldvdread",
                           AC_MSG_ERROR(Cannot find lib libdvdread),
                           )
AC_SEARCH_LIBS(DVDOpenDir, dvdread,,
                           AC_MSG_ERROR(libdvdread needs to be plus version),
                           )


LIBS="$LIBS $EXTRA_LIBS"
echo "libs is now $LIBS"

AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile src/Makefile win32/Makefile])
AC_OUTPUT

