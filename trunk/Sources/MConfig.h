#ifndef MCONFIG_H
#define MCONFIG_H

#if defined(__FreeBSD__) and (__FreeBSD__ > 0)
#	define HAVE_EXTATTR_H	1
#endif

#if defined(__linux__)
#	define HAVE_ATTRIBUTES_H		1
#endif

#endif
