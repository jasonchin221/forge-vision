lib_LTLIBRARIES = libforge_vision.la
libforge_vision_la_SOURCES = fv_array.c ../fv_log.c fv_mem.c fv_track.c \
							 fv_corner.c fv_edge.c fv_filter.c fv_debug.c \
							 fv_matrix.c fv_thresh.c fv_morph.c fv_stat.c \
							 fv_samplers.c fv_lkpyramid.c fv_border.c \
							 fv_pyramid.c fv_time.c fv_smooth.c fv_hough.c \
							 fv_math.c

AM_CPPFLAGS = -I$(srcdir)/../include
AM_CFLAGS = -Wall -Werror
AUTOMAKE_OPTIONS = foreign subdir-objects
