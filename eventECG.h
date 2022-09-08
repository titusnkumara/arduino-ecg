#ifndef EventECG_h
#define EventECG_h

//#include "Arduino.h"
#include <stdint.h>
#include <math.h>

#define 	USETIMING_FILTER	1

//#define 	FILTERING				1
#define 	ECG_NEG_POSSIBLE		1
//#define 	INVERT_INPUT			1
#define 	GET_LOCALMAXMIN			1

//#define	PROFILE_FUNC			1

#ifdef FILTERING
#define 	FCO_MAX				10
#define		NEW_COEFF			9
#define		OLD_COEFF			(FCO_MAX-NEW_COEFF)
#endif

#ifdef ECG_NEG_POSSIBLE
#define		NOISE_THRESH		8 //because high event rate, period should be lower
#else
#define		NOISE_THRESH		16 //because high event rate, period should be lower
#endif


//global printf macro for debugging on PC

//#define PC_VERBOSE				1
//#define FILE_STAT				1
#define SHOW_DATA_STAT			1

#ifdef PC_VERBOSE
#define DV(x) 					x
#else
#define DV(x)
#endif

#ifdef FILE_STAT
#define DF(x) 					x
#else
#define DF(x)
#endif

#ifdef SHOW_DATA_STAT
#define DS(x) 					x
#else
#define DS(x)
#endif


#define		SHIFT_NDIV			(float)100.0
#define 	MAX_FS				1000
#define		DEFAULT_THRESH		1000
#define		START_THRESH		DEFAULT_THRESH*2
#define		END_THESH			0
#define		THRES_DROPRATE		START_THRESH
#define		T_THRESH			0.33

#define 	EVENTARR_SIZE		3
#define 	THRESH_ARR_SIZE		3
#define 	LEAKY_ARR_SIZE		5
#define 	QRS_HOLDR_SIZE		2 //this is by default 2


//macros for getdifference

#define getLDiff(left0, left1)  ((left1-left0))
#define getRDiff(right0, right1) ((right1 - right0))

typedef struct event_t_ {
	int32_t diffSum;
	int32_t eventV;
	float deltaT;
	uint32_t idxSample;//global sample position for time calculation
} event_t;

class QRS {
	public:
		uint32_t idx;
		int32_t V;
		int32_t dV;
		bool valid;
		bool prevGood;
};

class ECGProcessor {
		uint32_t Fs = 0;
		uint32_t ShiftN = 0;
		uint32_t ShiftN_2 = 0;
		int32_t lastDataIdx = 0;
		int32_t lastDataIdx_1 = 0;
		int32_t nBehind = 0;
		int32_t nBehind_1 = 0;
		uint32_t readSampleArraySize = 0;
		uint32_t readSampleArraySize_1 = 0;
		uint32_t readSampleArraySize_HN2 = 0;
		uint32_t readSampleArraySize_HN2_1 = 0;
		uint32_t counter = 0;
		uint32_t eventIdx = 0;
		uint32_t curIdx = 0;

		int32_t startThresh		= START_THRESH;
		int32_t endThresh			= END_THESH;
		int32_t threshDropRate 	= THRES_DROPRATE;
		int32_t dynamicThresh 	= DEFAULT_THRESH;

		uint32_t threshIdx = 0;
		int32_t Tthresh		= 0;

		int32_t leakyMaxV		= 0;
		int32_t leakyMinV		= 0;
		int32_t leakyV		= 0;
		
		#ifdef GET_LOCALMAXMIN
		int32_t vLocalMax = 0;
		uint32_t vLocalMaxIdx = 0;
		int32_t vLocalMin = 0;
		uint32_t vLocalMinIdx = 0;
#endif

		int32_t * readSampleArray = NULL;
		event_t eventArray[EVENTARR_SIZE];
		int32_t thresholdArray[THRESH_ARR_SIZE];
		int32_t leakyMaxArray[LEAKY_ARR_SIZE];
		int32_t leakyMinArray[LEAKY_ARR_SIZE];
		QRS QRSholder[QRS_HOLDR_SIZE];

#ifdef FILTERING
		int32_t prevVal = 0;
#endif



	public:
		//variables for function profiling
#ifdef PROFILE_FUNC
		uint32_t eventGenCount = 0;//qualified events
		uint32_t timingFilter = 0;//thats been within threshold
#endif
		uint32_t totalDataCount = 0;
		int32_t minTthresh  = 0;
		uint32_t evFreq		= 0;
		ECGProcessor(uint32_t);//constructor
		QRS process(int32_t val);

	private:
		void _addData(int32_t val);
		void _appendEvent(int32_t absDiff,uint64_t counter,int32_t eventv);
		void _processEvent(uint32_t eventIdx);
		void _timingFilter(void);
};

#endif