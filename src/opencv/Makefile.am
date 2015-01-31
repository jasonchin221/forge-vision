lib_LTLIBRARIES = libopencv_can.la
libopencv_can_la_SOURCES = fv_opencv.c ../fv_log.c fv_track.c fv_edge.c fv_matrix.c \
						   fv_thresh.c fv_morph.c fv_stat.c fv_pyramid.c fv_smooth.c \
						   fv_hough.c fv_dft.c
libopencv_can_la_LIBADD = ../core/libforge_vision.la

AM_CPPFLAGS = -I$(srcdir)/../include
AM_CFLAGS = -Wall -Werror
AUTOMAKE_OPTIONS = foreign subdir-objects
