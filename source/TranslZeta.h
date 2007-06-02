/*
 * Public domain, baby!
 *
 * Localization for Zeta users
 *
 */

#ifndef __LOCALIZATION_H__
#define __LOCALIZATION_H__

#ifdef ZETA_OS
	#include <Locale.h>
    #define TranslZeta(x) _T(x)
#else
    #define TranslZeta(x) x
#endif

#endif // __LOCALIZATION_H__

