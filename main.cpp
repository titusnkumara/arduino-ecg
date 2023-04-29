#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>   /* gettimeofday, timeval (for timestamp in microsecond) */

#include "eventECG.h"
//global printf macro for debugging on PC
#include "debugSettings.h"

//static array for performance testing
#ifdef USE_STATIC_DATA
#include "100.h"
uint32_t my_array_size = sizeof(my_array) / sizeof(my_array[0]); // calculate the size of the array
uint32_t mydataCounter = 0;
#endif

#define WRITE_OUT   1

//const variables
const uint32_t _Fs = 360;//for MIT-BIH database
const int32_t SecPerMinTimes1000 = 60000;//time 1000 because I use inter arithmatics, HR should be divided by 1000
const long long int usPerSecond = 1000000ll;

static FILE * dataFile = NULL;
#ifdef WRITE_OUT
static FILE * outFile = NULL;
#endif

bool fileEndFlag = false;

void initProject(char * fileLocation) {
    //read the data file
    //read the file first
    if(!(dataFile = fopen(fileLocation,"r"))) {
        DF(printf("Error reading data files\n");)
        exit(0);
    } else {
        DF(printf("Data open\n");)
    }

#ifdef WRITE_OUT
    if(!(outFile = fopen("./output.txt","w"))) {
        DF(printf("Error open out file\n");)
        exit(0);
    } else {
        DF(printf("Write open\n");)
    }
#endif
}

float readValue() {
#ifdef USE_STATIC_DATA
    if(mydataCounter<my_array_size){
        mydataCounter++;
        return my_array[mydataCounter];
    }else{
        fileEndFlag = true;
        return 0;
    }
#endif
    float val;
    if(fscanf(dataFile,"%f",&val)!= EOF) {
        return (val);
    } else {
        DF(printf("Data End\n");)
        fileEndFlag = true;
        return (0);
    }
}

int main(int argc, char** argv) {

#ifdef PROFILE_FUNC
    /* Example of timestamp in microsecond. */
    struct timeval timer_usec;
    uint64_t timestamp_usec1;
    uint64_t timestamp_usec2; /* timestamp in microsecond */
    if (!gettimeofday(&timer_usec, NULL)) {
        timestamp_usec1 = (static_cast<long long int>(timer_usec.tv_sec)) * usPerSecond + static_cast<long long int>(timer_usec.tv_usec);
    } else {
        timestamp_usec1 = -1;
    }
#endif

    int16_t curVal;
    int32_t successCount = 0;
    int32_t totalCount = 0;
    int32_t validPrevx = 0;
    QRS qrsHolder;
    int32_t t_tmp=0;
    int32_t HRval=0;
    int32_t minTThreshval = 0;


    if(argc<2) {
        printf("Arg error Usage ./file datafile.txt \n");
        return (0);
    }

#ifndef USE_STATIC_DATA
    initProject(argv[1]);
#endif
    ECGProcessor& processor = ECGProcessor::getInstance(_Fs);

    minTThreshval = (processor.getMinTthresh())/2;
    QRS qrsOld; QRS qrsNew;
    qrsOld.valid = false;
    qrsNew.valid = false;
    qrsOld.idx = 0;
    qrsOld.V = 0;
    qrsNew.idx = 0;
    qrsNew.V = 0;

    while(1) {
        //read sample
#ifdef INVERT_INPUT
        curVal = static_cast<int32_t>(-1*(readValue() * INPUT_MULTIPLIER));
#else
        curVal = static_cast<int32_t>((readValue() * INPUT_MULTIPLIER));
#endif

        if(fileEndFlag) {
            break;
        }

        qrsHolder = processor.process(curVal);

        if(qrsHolder.valid) {

            qrsNew = qrsHolder;
            if(qrsOld.valid && qrsHolder.prevGood) {
                //calculate HR
                t_tmp = qrsOld.idx-validPrevx;
                if(t_tmp>minTThreshval) {
                    HRval = ((_Fs*SecPerMinTimes1000)/(qrsOld.idx-validPrevx));
                    //dispatch old
                    successCount++;
#ifdef WRITE_OUT
                    fprintf(outFile,"%u,%d,%d\n",qrsOld.idx,qrsOld.V,HRval);
#else
                    DV(printf("%u,%d\n",qrsOld.idx,qrsOld.V);)
#endif
                    //DS(printf("%u,%d\n",qrsOld.idx,processor.evFreq);)

                    validPrevx = qrsOld.idx;
                    qrsOld = qrsNew;
                } else {
                    DV(printf("old %d, valid %d\n",qrsOld.idx,minTThreshval);)
                    qrsOld = qrsNew;
                }
            } else {
                qrsOld = qrsNew;
            }
        }
        totalCount++;
    }

#ifndef USE_STATIC_DATA
    fclose(dataFile);
#endif
#ifdef WRITE_OUT
    fclose(outFile);
#endif
#ifdef PROFILE_FUNC
    if (!gettimeofday(&timer_usec, NULL)) {
        timestamp_usec2 = (static_cast<long long int>(timer_usec.tv_sec)) * usPerSecond + static_cast<long long int>(timer_usec.tv_usec);
    } else {
        timestamp_usec2 = -1;
    }
    printf("%I64d\n", timestamp_usec2-timestamp_usec1);
#endif

#ifdef PROFILE_FUNC
    printf("%d,%d,%d,%d\n",processor.totalDataCount,successCount,processor.eventGenCount,processor.timingFilter);
#endif
}
