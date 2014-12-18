bin_PROGRAMS = fv_app fv_test
fv_app_SOURCES = fv_app.c
fv_app_LDADD = $(srcdir)/../opencv/libopencv_can.la $(srcdir)/../core/libforge_vision.la
fv_test_SOURCES = fv_test.c
fv_test_LDADD = $(srcdir)/../opencv/libopencv_can.la $(srcdir)/../core/libforge_vision.la

AM_CPPFLAGS = -I$(srcdir)/../include
AM_CFLAGS = -Wall -Werror
