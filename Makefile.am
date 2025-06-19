-include $(top_srcdir)/build/modmake.rulesam
AUTOMAKE_OPTIONS = foreign subdir-objects
MODNAME=mod_hacking

LIBS := $(if $(switch_builddir),$(switch_builddir)/libfreeswitch.la,)

mod_LTLIBRARIES = mod_hacking.la
mod_hacking_la_SOURCES = mod_hacking.c curl_http.c
mod_hacking_la_CFLAGS  = $(AM_CFLAGS) $(FREESWITCH_CFLAGS) $(FFMPEG_CFLAGS) $(LIBCURL_CFLAGS)
mod_hacking_la_LDFLAGS = -avoid-version -module -no-undefined -shared $(FREESWITCH_LIBS) $(FFMPEG_LIBS) $(LIBCURL_LIBS)
mod_hacking_la_LIBADD  = $(LIBS)
