#include <cstdio>
#include <cstdlib>
#include "eventECG.h"
//global printf macro for debugging on PC
#include "debugSettings.h"

#define DYNAMICTHRESH_3_4         ((dynamicThresh*3)/4)
#define DYNAMICTHRESH_1_4         ((dynamicThresh*1)/4)
#define DYNAMICTHRESH_1_5         ((dynamicThresh)/5)
#define DYNAMICTHRESH_3_2         ((dynamicThresh*3)/2)


//these constants are used to filter out the event frequency
const uint32_t prevEventsWeight = 3;
const uint32_t eventDivideCount = 10;
const int32_t errorRatioThreshold = 8;

const uint16_t gravityCounterMax = 100;

const uint32_t tThreshDivisor = 4;
const uint32_t tThreshSlowDivisor_3 = 3;
const uint32_t minTThreshDivisor = 3;
const uint32_t tThreshWeight = 2;

ECGProcessor::ECGProcessor(uint32_t _Fs) {
    Fs = _Fs;
    ShiftN = round((Fs * 2) / static_cast<float>((SHIFT_NDIV)));
    ShiftN_2 = round(Fs / static_cast<float>((SHIFT_NDIV)));
    Tthresh = T_THRESH * Fs;
    minTthresh = (Fs * 1.0) * (T_THRESH);
    TthreshMax = (Fs * 1.0) * (T_THRESH_MAX);
    //eventTdiffThreshold = Fs/NOISE_THRESH_DIV;
    readSampleArraySize = Fs/FS_ARRAY_DIV;
    readSampleArraySize_1 = readSampleArraySize - 1;
    readSampleArraySize_HN2 = readSampleArraySize - ShiftN;
    readSampleArraySize_HN2_1 = readSampleArraySize - ShiftN_2 - 1;
    readSampleArray = new int32_t[readSampleArraySize];

    for (uint32_t i = 0; i < readSampleArraySize; i++) {
        readSampleArray[i] = 0;
    }
    eventArray[0].diffSum = 0;
    eventArray[0].idxSample = 0;
    eventArray[0].deltaT = 0;
    eventArray[0].eventV = 0;

    eventArray[1].diffSum = 0;
    eventArray[1].idxSample = 0;
    eventArray[1].deltaT = 0;
    eventArray[1].eventV = 0;

    curIdx = ShiftN + 2; //plus 2 because I check ShiftN-1
    counter = 0; //global idx counter
    lastDataIdx = ShiftN + 1;
    lastDataIdx_1 = ShiftN;
    nBehind = lastDataIdx - ShiftN;
    nBehind_1 = lastDataIdx - ShiftN - 1;

    QRSholder[0].valid = false;
    QRSholder[1].valid = false;

}

void ECGProcessor::addData(int32_t val) {

#ifdef FILTERING
    //averaging filter, could be inefficient
    readSampleArray[curIdx] = ((val) * (NEW_COEFF) + (prevVal * (OLD_COEFF)))/(FCO_MAX); //original
    prevVal = readSampleArray[curIdx]; //original
#else
    readSampleArray[curIdx] = val;
#endif
    //DAD(printf("%d,%d\n",readSampleArray[curIdx],val);)

    curIdx = (curIdx + 1) % readSampleArraySize;
    counter++;
    lastDataIdx = (lastDataIdx + 1) % readSampleArraySize;
    lastDataIdx_1 = (lastDataIdx_1 + 1) % readSampleArraySize;
    nBehind = (nBehind + 1) % readSampleArraySize;
    nBehind_1 = (nBehind_1 + 1) % readSampleArraySize;

}

void ECGProcessor::appendEvent(int32_t _absDiff, uint64_t _counter, int32_t _eventv) {
    //increment first, then add
    uint32_t prevEvIdx = eventIdx;
    eventIdx = (eventIdx + 1) % EVENTARR_SIZE;
    eventArray[eventIdx].diffSum = _absDiff;
    eventArray[eventIdx].eventV = _eventv;
    if (_counter > (ShiftN_2 + 1)) {
#ifdef NEED_EXACT_PEAK
        eventArray[eventIdx].idxSample = lastSignChangeLocation;
#else
        eventArray[eventIdx].idxSample = _counter - ShiftN_2 - 1; //becasue I need the mid point
#endif
    } else {
        eventArray[eventIdx].idxSample = 1;//negative index
    }
    int32_t idxDiff = eventArray[eventIdx].idxSample - eventArray[prevEvIdx].idxSample;
    //eventPeriod = ((eventPeriod*prevEventsWeight)+ (idxDiff * (eventDivideCount-prevEventsWeight)))/eventDivideCount;
    eventArray[eventIdx].deltaS = idxDiff;
    eventArray[eventIdx].deltaT = static_cast<float>((idxDiff)) / Fs;
    //add sum, update local event counts
//    TdiffSum = TdiffSum+idxDiff;
//    localEventCounter++;
//    if(TdiffSum>Fs){
//        //reset sum
//        TdiffSum = 0;
//        //save how many I had to count
//        eventsPerFsCounter = localEventCounter;
//        //reset counter
//        localEventCounter = 0;
//    }
    DAE(printf("fun %s, Ev %d,%f,%d\n","_appendEvent",eventArray[eventIdx].diffSum,eventArray[eventIdx].deltaT,eventArray[eventIdx].idxSample);)
    //printf("fun %s, idxDiff %d eventsPerFsCounter %d\n","_appendEvent",idxDiff,eventsPerFsCounter);

}


inline void ECGProcessor::calcGravity(int32_t val){
    if(abs(val - leakyMinV) < abs(val - leakyMaxV)){
        //this is now closer to min value
        minGravity ++;
        //handle overflow
        if(minGravity > gravityCounterMax){
            minGravity = minGravity/2;
            maxGravity = maxGravity/2;
        }
    }else{
        maxGravity++;
        //handle overflow
        if(maxGravity > gravityCounterMax){
            minGravity = minGravity/2;
            maxGravity = maxGravity/2;
        }
    }
}


QRS ECGProcessor::process(int32_t val) {
    //check the crossing points
    //now check the values
    //DV(printf("idx %d %d %d %d, idx:%d\n",nBehind_1,nBehind,lastDataIdx_1,lastDataIdx,curIdx);)
    //Add data first
    addData(val);
#ifdef PROFILE_FUNC
    totalDataCount ++;
#endif
#ifdef USETIMING_FILTER
    QRSholder[0].valid = false; //mark this false and send to processing
    //I can calculate the gravity of the signal
    calcGravity(val);
#else
    QRSholder[1].valid = false; //mark this false and send to processing
#endif
    bool cond1 = (readSampleArray[lastDataIdx] <= readSampleArray[nBehind]) && (readSampleArray[lastDataIdx_1] > readSampleArray[nBehind_1]);
#ifdef ECG_NEG_POSSIBLE
    bool cond2 = (readSampleArray[lastDataIdx] > readSampleArray[nBehind]) && (readSampleArray[lastDataIdx_1] <= readSampleArray[nBehind_1]);
#else
    bool cond2 = false;
#endif

#ifdef NEED_EXACT_PEAK
    bool conddir = readSampleArray[lastDataIdx] > readSampleArray[lastDataIdx_1];
    currSignDir = conddir ? POSITIVE : NEGATIVE;

    if(currSignDir!=prevSignDir){
        //DPE(printf("fun %s, Dir cur %d, prev %d, crosspoint %d, vals %d %d \n","process",currSignDir,prevSignDir,counter,readSampleArray[lastDataIdx_1],readSampleArray[lastDataIdx]);)
        //DX(printf("Vals lastDataIdx %d, %d\n",lastDataIdx,readSampleArray[lastDataIdx]);)
        prevSignDir = currSignDir;
        lastSignChangeLocation = counter;
    }
#endif
    if (cond1 || cond2) {
        DPE(printf("fun %s, event idx:%d\n","process",counter);)
        //crossing points,
#ifdef PROFILE_FUNC
                eventGenCount++;
#endif
                //find out the interested points
        int32_t midPoint = (lastDataIdx + readSampleArraySize_HN2_1) % readSampleArraySize; //lastDataIdx-ShiftN_2-1;
        int32_t midPoint_1 = (midPoint + readSampleArraySize_1) % readSampleArraySize; //midPoint-1;
        int32_t midPlus1 = (midPoint + 1) % readSampleArraySize;
        int32_t midPoint_HalfN = (midPoint + readSampleArraySize_HN2) % readSampleArraySize; //midPoint - ShiftN_2;

        int32_t ldiff0 = getLDiff(readSampleArray[midPoint_HalfN], readSampleArray[midPoint_1]);
        int32_t ldiff1 = getLDiff(readSampleArray[midPoint_1], readSampleArray[midPoint]);

        int32_t rdiff0 = getRDiff(readSampleArray[midPoint], readSampleArray[midPlus1]);
        int32_t rdiff1 = getRDiff(readSampleArray[midPlus1], readSampleArray[lastDataIdx]);

        int32_t absDiff = abs(ldiff0 + ldiff1) + abs(rdiff0 + rdiff1);
        appendEvent(absDiff, counter, readSampleArray[midPoint]);
        DPE(printf("fun %s, EV %d,%d,%d,%d,%d\n","process",midPoint_HalfN,midPoint_1,midPoint,midPlus1,lastDataIdx);)
        DPE(printf("fun %s, idx %d Ldiff %d %d , Rdiff %d %d\n","process",counter,ldiff0,ldiff1,rdiff0,rdiff1);)
        DPE(printf("fun %s, aDiff %d\n","process",absDiff);)
        //getting the event direction
        int32_t eventDir = ECG_POSTIVE;
        if((ldiff0+ldiff1 > 0) && (rdiff0+rdiff1)<0){
            eventDir = ECG_POSTIVE;
        }
        else if((ldiff0+ldiff1 < 0) && (rdiff0+rdiff1)>0){
            eventDir = ECG_NEGATIVE;
        }else{
            eventDir = ECG_POSTIVE;
        }
        processEvent(eventIdx,eventDir);
    }
#ifdef USETIMING_FILTER
    return (QRSholder[0]);
#else
    return (QRSholder[1]);
#endif

    DPE(printf("fun %s, %d,%d\n","process",readSampleArray[lastDataIdx],readSampleArray[nBehind]);)
}

void ECGProcessor::timingFilterFunction() {
#ifdef PROFILE_FUNC
    timingFilter++;
#endif

    //For the timing filter, I get two events, QRSholder 0 and 1
    //Printing these information
    DTF(printf("fun %s\n","_timingFilter -----------------------------------");)
    DTF(printf("%s\n","I get following data at QRSholder[0] and QRSHolder[1]");)
    DTF(printf("QRSholder[0].idx: %d \t| QRSholder[1].idx: %d\n",QRSholder[0].idx,QRSholder[1].idx);)
    DTF(printf("QRSholder[0].V: %d \t| QRSholder[1].V: %d\n",QRSholder[0].V,QRSholder[1].V);)
    DTF(printf("QRSholder[0].dV: %d \t| QRSholder[1].dV: %d\n",QRSholder[0].dV,QRSholder[1].dV);)
    DTF(printf("QRSholder[0].valid: %d \t| QRSholder[1].valid: %d\n",QRSholder[0].valid,QRSholder[1].valid);)
    DTF(printf("QRSholder[0].prevgood: %d \t| QRSholder[1].prevgood: %d\n",QRSholder[0].prevGood,QRSholder[1].prevGood);)
    DTF(printf("QRSholder[0].HighDiff: %d \t| QRSholder[1].HighDiff: %d\n",QRSholder[0].HighDiff,QRSholder[1].HighDiff);)

    //Calculate the timing difference between the two events. If thi is less than Tthresh, there is filtering needs to be done
    int32_t tdiffs = (QRSholder[1].idx - QRSholder[0].idx);
    uint8_t QRSrecomend = 0;//recomend the zero one default
    DTF(printf("tdiffs %d Tthresh%d\n",tdiffs,Tthresh);)
    if (tdiffs < Tthresh) {
        DTF(printf("I need to use the timing filter as tdiff %d is smaller than Tthresh %d\n",tdiffs,Tthresh);)
        if(tdiffs <minTthresh){
            QRSholder[1].valid = false;
            return;
        }
        if(QRSholder[0].HighDiff && !(QRSholder[1].HighDiff)){
            //recomend 0
            QRSrecomend = 0;
        }else if(!QRSholder[0].HighDiff && (QRSholder[1].HighDiff)){
            QRSrecomend = 1;
        }else if(QRSholder[0].HighDiff && QRSholder[1].HighDiff){
            //check the min tthresh
            //already checked
            //accept the new one
            QRSholder[0].valid = true;
            QRSholder[0].idx = QRSholder[1].idx;
            QRSholder[0].V = QRSholder[1].V;
            QRSholder[0].dV = QRSholder[1].dV;
            QRSholder[0].HighDiff = QRSholder[1].HighDiff;
            QRSholder[0].prevGood = true;

            Tthresh = (2*Tthresh + minTthresh + tdiffs)/tThreshDivisor;
        }else{
            //can't chose based on highdiff
            uint32_t VPrev = abs(QRSholder[0].V - leakyMinV);
            uint32_t vNow = abs(QRSholder[1].V - leakyMinV);
            uint32_t VPrev2 = abs(QRSholder[0].V - leakyMaxV);
            uint32_t vNow2 = abs(QRSholder[1].V - leakyMaxV);

            DTF(printf("%s\n","I am calculating the two abs values that gives me height difference");)
            DTF(printf("I use min/max LeakyminV %d LeakymaxV %d\n",leakyMinV,leakyMaxV);)
            DTF(printf("This gives me answers V-min [0]: %d V-min [1]: %d\n",VPrev,vNow);)
            DTF(printf("This gives me answers V-max [0]: %d V-max [1]: %d\n",VPrev2,vNow2);)

            //this polarity comes from events
            DTF(printf("Printing the gravity of current signal min: %d max: %d\n",minGravity,maxGravity);)
            if (ECGPolarity == ECG_POSTIVE ) { // Positive ECG signal
                QRSrecomend = (VPrev > vNow) ? 0 : 1;
            } else { // Negative ECG signal
                QRSrecomend = (VPrev2 > vNow2) ? 0 : 1;
            }

        }
        DTF(printf("Recomended index of correct peak is at %d\n",QRSholder[QRSrecomend].idx);)
        //reject the wrong one
        if(QRSrecomend == 1){
            //new one is correct
            //if this is the case, I am going to replace the old [0] with new one
            Tthresh = (tThreshWeight*Tthresh + minTthresh + tdiffs)/tThreshDivisor;
            minTthresh = Tthresh/minTThreshDivisor;
            QRSholder[0].valid = true;
            QRSholder[0].idx = QRSholder[1].idx;
            QRSholder[0].V = QRSholder[1].V;
            QRSholder[0].dV = QRSholder[1].dV;
            QRSholder[0].prevGood = false;
        }else{
            //previous one is correct, I need to ignore the current event
            Tthresh = (tThreshWeight*Tthresh + tdiffs)/tThreshSlowDivisor_3;
            QRSholder[0].valid = true;
            QRSholder[1].valid = false;
            QRSholder[1].prevGood = true;
        }
    } else {
        //If there is no events to be filtered out, I can include the timing difference into Tthreahold calculation as this is probaly a valid sample
        //include this in tThresh
        DTF(printf("fun %s, No need of timing filter as current tdiff %d larger than Tthresh %d\n","_timingFilter", tdiffs, Tthresh);)
        if(tdiffs>TthreshMax){
            Tthresh = ((Tthresh + TthreshMax)) / tThreshSlowDivisor_3;
        }else{
            Tthresh = ((Tthresh + tdiffs)) / tThreshSlowDivisor_3;
        }
        //good to go
        //copy the qrsholder[1] into [0] as with timing filter I always return [0] to the main
        QRSholder[0].valid = true;
        QRSholder[0].idx = QRSholder[1].idx;
        QRSholder[0].V = QRSholder[1].V;
        QRSholder[0].dV = QRSholder[1].dV;
        QRSholder[0].HighDiff = QRSholder[1].HighDiff;
        QRSholder[0].prevGood = true;
    }
    if(Tthresh> TthreshMax){
        Tthresh = TthreshMax;
    }else if(Tthresh<minTthresh){
        Tthresh = minTthresh;
    }
    DTF(printf("%s\n","-----------------------------------");)
#ifdef DEBUG_timingFilter
    if(counter > 27973 + 50){
        exit(0);
    }
#endif
}


inline int32_t getMinValue(int32_t *readSampleArray, int32_t readSampleArraySize) {
    int32_t minValue = INT32_MAX;  // initialize to largest possible integer value
    for (int i = 0; i < readSampleArraySize; i++) {
        if (readSampleArray[i] < minValue) {
            minValue = readSampleArray[i];
        }
    }
    return (minValue);
}

inline int32_t getMaxValue(int32_t *readSampleArray, int readSampleArraySize) {
    int32_t maxValue = INT32_MIN;  // initialize to smallest possible integer value
    for (int i = 0; i < readSampleArraySize; i++) {
        if (readSampleArray[i] > maxValue) {
            maxValue = readSampleArray[i];
        }
    }
    return (maxValue);
}


void ECGProcessor::processEvent(uint32_t currentEventIdx, int32_t eventDir) {
    DPE(printf("fun %s-----------------------------\n","_processEvent");)
    DPE(printf("fun %s, Ev %d,%f, %d, %d , eventDir %d \n","_processEvent",eventArray[currentEventIdx].diffSum,eventArray[eventIdx].deltaT,eventArray[eventIdx].eventV,eventArray[eventIdx].idxSample,eventDir);)
    evCounter++;//increasing the event counter for noise calculation
    DPE(printf("Counter %d/evCounter %d ratio is %d-----------------------------\n",counter,evCounter,counter/evCounter);)

    int32_t eventV = eventArray[currentEventIdx].eventV;
    float timeDiff = eventArray[currentEventIdx].deltaT;
    int32_t gradDiff = eventArray[currentEventIdx].diffSum;
    int32_t vTimeDiff = leakyV * timeDiff;

    leakyV = abs(leakyMaxV - leakyMinV) / ShiftN;
    leakyMaxV = leakyMaxV - vTimeDiff;
    leakyMinV = leakyMinV + vTimeDiff;

    int32_t totalHeight = leakyMaxV - leakyMinV;

    if (leakyMaxV < leakyMinV) {
        leakyMaxV = leakyMinV;
    }
    if (eventV > leakyMaxV) {
        leakyMaxV = eventV;
    } else if (eventV < leakyMinV) {
        leakyMinV = eventV;
    }
    DPE(printf("dynamicThresh %d threshDropRate * timeDiff %d\n",dynamicThresh,(int)(threshDropRate * timeDiff));)

    dynamicThresh = static_cast<int>((dynamicThresh - (threshDropRate * timeDiff)));
    if (dynamicThresh < endThresh) {
        dynamicThresh = endThresh;
    }
    int32_t vmid = (totalHeight) / 2;
    int32_t errorRatio = counter/evCounter;
    bool noisy = (errorRatio <= 5);
    if (dynamicThresh > totalHeight) {
        //drop faster.
        threshDropRate = totalHeight*2;
    }else{
        //if noisy, drop slower
        if(noisy){
            //this means signal is noisy
            threshDropRate = dynamicThresh/tThreshSlowDivisor_3;
        }else{
            threshDropRate = dynamicThresh;
        }

    }
    //any process start with assuming preve event is good
    QRSholder[1].prevGood = true;

    //if noise value is affecting gradDiff
    bool noiseCond = false;
    if(errorRatio<errorRatioThreshold){
        noiseCond = (eventArray[currentEventIdx].deltaS >= errorRatio);//R waves are not noisy
    }else{
        noiseCond = true;
    }
    bool eventCond1 = (gradDiff > dynamicThresh);
    bool canceledEvent = false;
    if(eventCond1){

        //since this is a good event, record info
        if(eventDir == ECG_POSTIVE){
            lastGoodThresh_P = eventV;
            lastGoodThresh_HP = abs(eventV-vmid);
        }else{
            lastGoodThresh_N = eventV;
            lastGoodThresh_HN = abs(vmid - eventV);
        }

        //check if this should be invalidated
        if(justHadEvent && gradDiff<prevGradDiff){
            eventCond1 = false; //no need subsequent events
            canceledEvent = true;
            DPE(printf("%s\n","Canceled, just had event ");)

        }

        if(gradDiff < DYNAMICTHRESH_3_2 && !noiseCond){
            //insignificant event so need to satisfy noise cond as well
            DPE(printf("%s gradDiff %d < (dynamicThresh*3)/2 %d\n","Canceled, noise cond ",gradDiff,(dynamicThresh*3)/2);)
            canceledEvent = true;
            eventCond1 = false;
        }
    }else{
        justHadEvent = false;
        prevGradDiff = gradDiff;
    }
    DPE(printf("(gradDiff %d > dynamicThresh %d ) && (eventArray[currentEventIdx].deltaS %d noisecond %d\n",gradDiff , dynamicThresh, (eventArray[eventIdx].deltaS),noiseCond);)
    bool eventCond2 = false;
    if(minGravity > maxGravity){
        ECGPolarity = ECG_POSTIVE;
    }else{
        ECGPolarity = ECG_NEGATIVE;
    }
    if(!eventCond1 && noiseCond && !canceledEvent){
        DPE(printf("%s\n","Checking cond2");)
        DPE(printf("lastGoodThresh_P  %d, lastGoodThresh_HP %d,lastGoodThresh_N %d, lastGoodThresh_HN %d\n",lastGoodThresh_P,lastGoodThresh_HP,lastGoodThresh_N,lastGoodThresh_HN);)
        //if gradDiff is just missed, I am going to pass it based on just the proximity
        if(gradDiff > DYNAMICTHRESH_3_4){
            //gradDiff is very close
            DPE(printf("gradDiff %d > (dynamicThresh*3)/4 %d\n",gradDiff ,(dynamicThresh*3)/4);)
            eventCond2 = true; //start with true and AND it
            if(eventDir == ECG_POSTIVE){
                //this event should be closer to positive side than the negative side
                int32_t distanceToP = abs(eventV - lastGoodThresh_P);
                int32_t distanceToN = abs(eventV - lastGoodThresh_N);
                bool tmpcond = (distanceToP + lastGoodThresh_HP/2)<distanceToN;
                eventCond2 = eventCond2 && tmpcond;
            }else{
                //this event should be closer to negative side than the positive side
                int32_t distanceToP = abs(eventV - lastGoodThresh_P);
                int32_t distanceToN = abs(eventV - lastGoodThresh_N);
                bool tmpcond = (distanceToN + lastGoodThresh_HN/2) <distanceToP;
                eventCond2 = eventCond2 && tmpcond;
            }
        }else if(gradDiff > DYNAMICTHRESH_1_4){
            //gradDiff is too small
            //this should check more conditions
            DPE(printf("gradDiff %d < (dynamicThresh*1)/4 %d too small!\n",gradDiff ,(dynamicThresh*1)/4);)
            //I am going to do expensive operations, therefore omit unworthy events
            int32_t minVal = getMinValue(readSampleArray,readSampleArraySize);
            int32_t maxVal = getMaxValue(readSampleArray,readSampleArraySize);
            DPE(printf("minVal %d maxVal %d\n",minVal,maxVal);)
            if(maxVal-minVal > dynamicThresh/2){
                eventCond2 = true;
                DPE(printf("eventCond2 %d !!\n",eventCond2);)
                //this event should be closer to the previous event
                if(eventDir == ECG_POSTIVE){
                    //this event should be closer to positive side than the negative side
                    int32_t distanceToP = abs(eventV - lastGoodThresh_P);
                    int32_t distanceToN = abs(eventV - minVal);
                    bool tmpcond = (distanceToP + lastGoodThresh_HP/3)<distanceToN;
                    eventCond2 = eventCond2 &&  tmpcond;
                }else{
                //this event should be closer to negative side than the positive side
                    int32_t distanceToP = abs(maxVal - eventV);
                    int32_t distanceToN = abs(eventV - lastGoodThresh_N);
                    bool tmpcond = (distanceToN + lastGoodThresh_HN/3) <distanceToP;
                    eventCond2 = eventCond2 &&  tmpcond;
                }
            }else{
                DPE(printf("minVal - maxVal %d dynamicThresh/2 %d\n",maxVal-minVal,dynamicThresh/2);)
            }
        }else{
            eventCond2 = false;
        }
    }

    if (eventCond1 || eventCond2) {
        //we have possible event
        DPE(printf("Possible event at %d ********* cond1 %d, cond2 %d\n",eventArray[currentEventIdx].idxSample,eventCond1,eventCond2);)
        endThresh = DYNAMICTHRESH_1_5;
        QRSholder[1].HighDiff = false;
        if(eventCond1){
            justHadEvent = true;
            prevGradDiff = gradDiff;
            QRSholder[1].HighDiff = true;
            thresholdArray[threshIdx] = gradDiff;
            DPE(printf("lastGoodThresh_P  %d, lastGoodThresh_HP %d,lastGoodThresh_N %d, lastGoodThresh_HN %d\n",lastGoodThresh_P,lastGoodThresh_HP,lastGoodThresh_N,lastGoodThresh_HN);)
        }else{
            thresholdArray[threshIdx] = dynamicThresh;
        }
        //find mean
        uint32_t i = 0;
        int32_t sum = 0;
        for (i = 0; i < THRESH_ARR_SIZE; i++) {
            sum = sum + thresholdArray[i];
        }
        dynamicThresh = abs(sum / THRESH_ARR_SIZE);
        //unfiltered ECG event here
        DPE(printf("fun %s, gradDif %d endThresh %d dthresh %d id %d\n","_processEvent",gradDiff,endThresh,dynamicThresh,eventArray[currentEventIdx].idxSample);)
        threshIdx = (threshIdx + 1) % THRESH_ARR_SIZE;

        //add this to qrsholder array
#ifdef NEED_EXACT_PEAK
        QRSholder[1].idx = lastSignChangeLocation-1;//eventArray[eventIdx].idxSample;
        //DPE(printf("fun %s, QRS idx %d, last sign %d\n","_processEvent",eventArray[eventIdx].idxSample,lastSignChangeLocation);)
#else
        QRSholder[1].idx = eventArray[currentEventIdx].idxSample;
#endif
        QRSholder[1].V = eventV;
        QRSholder[1].dV = gradDiff;

#ifdef USETIMING_FILTER
        timingFilterFunction();
#else
        QRSholder[1].valid = true;
#endif
        //if noisy, drop slower
//        //update drop rate
//        if(eventsPerFsCounter>eventTdiffThreshold) {
//            threshDropRate = (dynamicThresh*2)/3;
//            DPE(printf("fun %s,Droprate changed %d %d %d\n","_processEvent",counter,eventPeriod,NOISE_THRESH);)
//            //printf("fun %s, eventsPerFsCounter %d eventTdiffThreshold %d\n","_processEvent",eventsPerFsCounter,eventTdiffThreshold);
//        }
//        DPE(printf("fun %s, evFreq %d\n","_processEvent",eventPeriod);)
//        //printf("fun %s, evFreq %d\n","_processEvent",evFreq);
    }
    DPE(printf("%s\n","-----------------------------\n");)

#ifdef DEBUG_processEvent
    if(counter > 35469 + 50){
        exit(0);
    }
#endif

}

//public getter
int32_t ECGProcessor::getMinTthresh(){
    return (minTthresh);
}
