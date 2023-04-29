/*
 * debugSettings.h
 *
 *  Created on: 27 Apr. 2023
 *      Author: titus
 */

#ifndef DEBUGSETTINGS_H_
#define DEBUGSETTINGS_H_


/*
This header defines which places debug printf should be activated
*/

/*
Global debug settings
*/

//#define SHOW_DATA_STAT            1
//#define PC_VERBOSE                1
//#define DEBUG_SINGLE          1
//#define FILE_STAT             1
//#define USE_STATIC_DATA           1


/*
Function level debug settings
*/

//#define DEBUG_addData         1
//#define DEBUG_appendEvent     1
//#define DEBUG_processEvent        1
//#define DEBUG_timingFilter        1

/*
Debug macro generations
*/

#ifdef PC_VERBOSE
#define DV(x)                   x
#else
#define DV(x)
#endif

#ifdef FILE_STAT
#define DF(x)                   x
#else
#define DF(x)
#endif

#ifdef SHOW_DATA_STAT
#define DS(x)                   x
#else
#define DS(x)
#endif

#ifdef DEBUG_SINGLE
#define DX(x)                   x
#else
#define DX(x)
#endif


#ifdef DEBUG_addData
#define DAD(x)                  x
#else
#define DAD(x)
#endif

#ifdef DEBUG_appendEvent
#define DAE(x)                  x
#else
#define DAE(x)
#endif

#ifdef DEBUG_processEvent
#define DPE(x)                  x
#else
#define DPE(x)
#endif

#ifdef DEBUG_timingFilter
#define DTF(x)                  x
#else
#define DTF(x)
#endif




#endif /* DEBUGSETTINGS_H_ */
