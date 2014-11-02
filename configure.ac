AC_INIT([forgevision], [1.0], [jasonchin221@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_ARG_WITH(libopencv_includes,
			[  --with-libopencv-includes=DIR    libopencv include directory],
			[with_libopencv_includes="$withval"],
			[with_libopencv_includes=no])

AC_ARG_WITH(libopencv_libraries,
			[  --with-libopencv-libraries=DIR   libopencv library directory],
			[with_libopencv_libraries="$withval"],
			[with_libopencv_libraries=no])


CPPFLAGS="${CPPFLAGS} -I/usr/local/include"
LIBS="${LIBS} -L/usr/local/lib"

if test "$with_libopencv_includes" != "no"; then
	CPPFLAGS="${CPPFLAGS} -I${with_libopencv_includes}"
fi

if test "$with_libopencv_libraries" != "no"; then
	LIBS="${LIBS} -L${with_libopencv_libraries}"
fi

LPTHREAD=""
AC_CHECK_LIB([pthread], [pthread_create], , LPTHREAD="no")

if test "$LPTHREAD" = "no"; then
    echo
    echo "   ERROR!  pthread library not found, go get it from"
    echo "   or use the --with-pthread-* options, if you have it installed"
    echo "   in unusual place"
    exit
fi

OPENCVH=""
AC_CHECK_HEADERS([opencv/highgui.h], , OPENVCH="no")

if test "$OPENCVH" = "no"; then
    echo
    echo "   ERROR!  opencv headers not found, go get it from"
    echo "   http://www.xmlsoft.org/downloads.html"
    echo "   or use the --with-opencv-* options, if you have it installed"
    echo "   in unusual place"
    exit
fi

LOPENCV=""
AC_CHECK_LIB([opencv_highgui], [cvShowImage], , LOPENCV="no")

if test "$LOPENCV" = "no"; then
    echo
    echo "   ERROR!  opencv library not found, go get it from"
    echo "   or use the --with-opencv-* options, if you have it installed"
    echo "   in unusual place"
    exit
fi

LOPENCV_CORE=""
AC_CHECK_LIB([opencv_core], [cvReleaseImage], , LOPENCV_CORE="no")

if test "$LOPENCV_CORE" = "no"; then
    echo
    echo "   ERROR!  opencv library not found, go get it from"
    echo "   or use the --with-opencv-* options, if you have it installed"
    echo "   in unusual place"
    exit
fi

LOPENCV_IMGPROC=""
AC_CHECK_LIB([opencv_imgproc], [cvCvtColor], , LOPENCV_IMGPROC="no")

if test "$LOPENCV_IMGPROC" = "no"; then
    echo
    echo "   ERROR!  opencv library not found, go get it from"
    echo "   or use the --with-opencv-* options, if you have it installed"
    echo "   in unusual place"
    exit
fi

LOPENCV_VIDEO=""
AC_CHECK_LIB([opencv_video], [cvCalcOpticalFlowPyrLK], , LOPENCV_VIDEO="no")

if test "$LOPENCV_VIDEO" = "no"; then
    echo
    echo "   ERROR!  opencv library not found, go get it from"
    echo "   or use the --with-opencv-* options, if you have it installed"
    echo "   in unusual place"
    exit
fi


AC_CONFIG_FILES([
				 Makefile
				 src/Makefile
				 src/opencv/Makefile
				 src/apps/Makefile
				 ])

AC_OUTPUT
