/*
 * Public domain, baby!
 *
 * Localization for Zeta users
 *
 */

#ifndef __TRANSL_ZETA_H__
#define __TRANSL_ZETA_H__

#ifdef B_ZETA_VERSION
	#include <locale/Locale.h>
    #define TranslZeta(x) _T(x)
#else
    #define TranslZeta(x) x
#endif

#endif // __TRANSL_ZETA_H__

