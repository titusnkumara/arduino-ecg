/*
 * eventECG.h
 *
 *  Created on: 27 Apr. 2023
 *      Author: titus
 */

#ifndef EventECG_h
#define EventECG_h

//#include "Arduino.h"
#include <stdint.h>
#include <math.h>

//macro for user settings
#define     USETIMING_FILTER    1
//#define FILTERING             1
#define     ECG_NEG_POSSIBLE        1
//#define   INVERT_INPUT        1
#define       PROFILE_FUNC        1
#define     NEED_EXACT_PEAK     1

#define     INPUT_MULTIPLIER    (1000)

#ifdef FILTERING
#define     FCO_MAX             10
#define     NEW_COEFF           3
#define     OLD_COEFF           FCO_MAX-NEW_COEFF
#endif


#ifdef NEED_EXACT_PEAK
#define     POSITIVE            1
#define     NEGATIVE            -1
#endif


//macros for getdifference

#define getLDiff(left0, left1)  ((left1-left0))
#define getRDiff(right0, right1) ((right1 - right0))


#define     SHIFT_NDIV          (100.0)
#define     FS_ARRAY_DIV        (13)
#define     MAX_FS              (1000)
#define     DEFAULT_THRESH      (1000)
#define     START_THRESH        (DEFAULT_THRESH*2)
#define     END_THESH           (0)
#define     THRES_DROPRATE      (START_THRESH)
#define     T_THRESH            (0.33)
#define     T_THRESH_MAX        (1.33)

#define     EVENTARR_SIZE       3
#define     THRESH_ARR_SIZE     3
#define     LEAKY_ARR_SIZE      5
#define     QRS_HOLDR_SIZE      2 //this is by default 2

#define     ECG_POSTIVE         1
#define     ECG_NEGATIVE        0

typedef struct event_t_ {
    int32_t diffSum;
    int32_t eventV;
    int32_t deltaS;
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
        bool HighDiff;
};

class ECGProcessor {
    /*
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
        uint32_t evCounter = 0;
        uint32_t eventIdx = 0;
        uint32_t curIdx = 0;
        uint32_t evFreq = 0;
        int32_t minTthresh  = 0;


        int32_t startThresh     = START_THRESH;
        int32_t endThresh           = END_THESH;
        int32_t threshDropRate  = THRES_DROPRATE;
        int32_t dynamicThresh   = DEFAULT_THRESH;

        uint32_t threshIdx = 0;
        int32_t Tthresh     = 0;
        int32_t TthreshMax = 0;

        int32_t leakyMaxV       = 0;
        int32_t leakyMinV       = 0;
        int32_t leakyV      = 0;

        int32_t minGravity = 1; //default assume min gravity
        int32_t maxGravity = 0;

        bool justHadEvent = false;
        int32_t prevGradDiff = 0;
        int32_t lastGoodThresh = 0;
        int32_t lastGoodThresh_P = 0;
        int32_t lastGoodThresh_N = 0;
        int32_t lastGoodThresh_H = 0;
        int32_t lastGoodThresh_HP = 0;
        int32_t lastGoodThresh_HN = 0;
        uint8_t ECGPolarity = ECG_POSTIVE; //Positive 1, Negative 0

        int32_t * readSampleArray = NULL;
        //int32_t * readSampleArrayDiff = NULL;
        event_t eventArray[EVENTARR_SIZE]{};
        int32_t thresholdArray[THRESH_ARR_SIZE] =  {0};
        int32_t leakyMaxArray[LEAKY_ARR_SIZE] = {1};
        int32_t leakyMinArray[LEAKY_ARR_SIZE] = {0};
        QRS QRSholder[QRS_HOLDR_SIZE]{};
*/
        // Sample processing variables
        uint32_t Fs = 0;
        uint32_t ShiftN = 0;
        uint32_t ShiftN_2 = 0;
        uint32_t counter = 0;
        uint32_t curIdx = 0;
        int32_t * readSampleArray = NULL;

        // Event-related variables
        uint32_t evCounter = 0;
        uint32_t eventIdx = 0;
        bool justHadEvent = false;
        event_t eventArray[EVENTARR_SIZE]{};
        QRS QRSholder[QRS_HOLDR_SIZE]{};

        // Index-related variables
        int32_t lastDataIdx = 0;
        int32_t lastDataIdx_1 = 0;
        int32_t nBehind = 0;
        int32_t nBehind_1 = 0;

        // Sample array size variables
        uint32_t readSampleArraySize = 0;
        uint32_t readSampleArraySize_1 = 0;
        uint32_t readSampleArraySize_HN2 = 0;
        uint32_t readSampleArraySize_HN2_1 = 0;

        // Threshold-related variables
        int32_t startThresh = START_THRESH;
        int32_t endThresh = END_THESH;
        int32_t threshDropRate = THRES_DROPRATE;
        int32_t dynamicThresh = DEFAULT_THRESH;
        uint32_t threshIdx = 0;
        int32_t Tthresh = 0;
        int32_t TthreshMax = 0;
        int32_t minTthresh = 0;
        int32_t lastGoodThresh = 0;
        int32_t lastGoodThresh_P = 0;
        int32_t lastGoodThresh_N = 0;
        int32_t lastGoodThresh_H = 0;
        int32_t lastGoodThresh_HP = 0;
        int32_t lastGoodThresh_HN = 0;
        uint32_t eventTdiffThreshold = 0;
        int32_t thresholdArray[THRESH_ARR_SIZE] = {0};

        // Leaky integrator variables
        int32_t leakyMaxV = 0;
        int32_t leakyMinV = 0;
        int32_t leakyV = 0;
        int32_t leakyMaxArray[LEAKY_ARR_SIZE] = {1};
        int32_t leakyMinArray[LEAKY_ARR_SIZE] = {0};

        // Gravity-related variables
        int32_t minGravity = 1; //default assume min gravity
        int32_t maxGravity = 0;

        // Miscellaneous variables
        int32_t prevGradDiff = 0;
        uint8_t ECGPolarity = ECG_POSTIVE; //Positive 1, Negative 0

#ifdef FILTERING
        int32_t prevVal = 0;
#endif

#ifdef NEED_EXACT_PEAK
        uint32_t lastSignChangeLocation = 0;
        uint32_t currSignDir = POSITIVE;
        uint32_t prevSignDir = POSITIVE;
#endif

    public:
        //variables for function profiling
#ifdef PROFILE_FUNC
        uint32_t eventGenCount = 0;//qualified events
        uint32_t timingFilter = 0;//thats been within threshold
        uint32_t totalDataCount = 0;
#endif
        // Singleton instance access method
        static ECGProcessor& getInstance(uint32_t fs) {
            static ECGProcessor instance(fs);
            return (instance);
        }

        QRS process(int32_t val);

        //public getters
        int32_t getMinTthresh();
    private:
        // Making constructor private for Singleton pattern
        ECGProcessor(uint32_t fs); //constructor

        // Deleted copy constructor and assignment operator
        ECGProcessor(const ECGProcessor&) = delete;
        ECGProcessor& operator=(const ECGProcessor&) = delete;

        //Other private functions
        void addData(int32_t _val);
        void appendEvent(int32_t _absDiff,uint64_t _counter,int32_t _eventv);
        void processEvent(uint32_t _eventIdx,int32_t _eventDir);
        void timingFilterFunction(void);
        void calcGravity(int32_t _val);
};

#endif

