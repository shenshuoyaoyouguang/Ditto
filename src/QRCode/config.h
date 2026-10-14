#ifndef CONFIG_H
#define CONFIG_H

// Manual config.h for Microsoft Visual Studio (no autotools/cmake).
// Tracks libqrencode 4.1.1 (configure.ac: MAJOR 4, MINOR 1, MICRO 1).
// HAVE_CONFIG_H itself is defined project-wide in CP_Main.vcxproj.
// HAVE_LIBPTHREAD is intentionally left undefined: single-threaded build,
// same as before (rsecc.c guards pthread code with it).

#define HAVE_STRDUP 1

#define __STATIC static

/* Define to 'static' if no test programs will be compiled. */
#define STATIC_IN_RELEASE static
/* #undef WITH_TESTS */

#define MAJOR_VERSION 4
#define MINOR_VERSION 1
#define MICRO_VERSION 1
#define VERSION "4.1.1"

#endif
