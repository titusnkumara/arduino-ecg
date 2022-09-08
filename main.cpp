#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>   /* gettimeofday, timeval (for timestamp in microsecond) */

#include "eventECG.h"

#define WRITE_OUT	1


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
	float val;
	if(fscanf(dataFile,"%f",&val)!= EOF) {
		return val;
	} else {
		DF(printf("Data End\n");)
		fileEndFlag = true;
		return 0;
	}
}

int main(int argc, char** argv) {

#ifdef PROFILE_FUNC
	/* Example of timestamp in microsecond. */
	struct timeval timer_usec;
	uint64_t timestamp_usec1,timestamp_usec2; /* timestamp in microsecond */
	if (!gettimeofday(&timer_usec, NULL)) {
		timestamp_usec1 = ((long long int) timer_usec.tv_sec) * 1000000ll +
		                  (long long int) timer_usec.tv_usec;
	} else {
		timestamp_usec1 = -1;
	}
#endif

	int16_t curVal;
	int32_t successCount = 0;
	int32_t totalCount = 0;
	int32_t validPrevx = 0;
	QRS qrsHolder;
	int32_t t_tmp=0,HRval=0;
	int32_t minTThreshval = 0;


	if(argc<2) {
		printf("Arg error Usage ./file datafile.txt \n");
		return 0;
	}

	initProject(argv[1]);
	ECGProcessor processor(360);

	minTThreshval = processor.minTthresh/2;
	QRS qrsOld,qrsNew;
	qrsOld.valid = false;
	qrsNew.valid = false;
	qrsOld.idx = 0;
	qrsOld.V = 0;
	qrsNew.idx = 0;
	qrsNew.V = 0;

	while(1) {
		//read sample
#ifdef INVERT_INPUT
		curVal = (int32_t)(-1*readValue()*1000);
#else
		curVal = (int32_t)(readValue()*1000);
#endif

		if(fileEndFlag) {
			break;
		}

		qrsHolder = processor.process(curVal);

		if(qrsHolder.valid==true) {

			qrsNew = qrsHolder;
			if(qrsOld.valid && qrsHolder.prevGood) {
				//calculate HR
				t_tmp = qrsOld.idx-validPrevx;
				if(t_tmp>minTThreshval) {
					HRval = ((360*60000)/(qrsOld.idx-validPrevx));
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

#ifdef PROFILE_FUNC
	if (!gettimeofday(&timer_usec, NULL)) {
		timestamp_usec2 = ((long long int) timer_usec.tv_sec) * 1000000ll +
		                  (long long int) timer_usec.tv_usec;
	} else {
		timestamp_usec2 = -1;
	}
#endif


#ifdef WRITE_OUT
	fclose(outFile);
#endif



	//format: totaldatasize,hrevents,totalevents,timingfiltercalled,timeconsumed in microseconds
#ifdef PROFILE_FUNC
	DS(printf("%d,%d,%d,%d,%I64d\n",processor.totalDataCount,successCount,processor.eventGenCount,processor.timingFilter,timestamp_usec2-timestamp_usec1);)
#endif
}
