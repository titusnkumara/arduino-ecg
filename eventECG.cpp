#include <stdio.h>
#include "eventECG.h"


ECGProcessor::ECGProcessor(uint32_t _Fs) {
	Fs = _Fs;
	ShiftN = round((Fs * 2) / SHIFT_NDIV);
	ShiftN_2 = round(Fs / SHIFT_NDIV);
	Tthresh = T_THRESH * Fs;
	minTthresh = (Fs * 1.0) * T_THRESH;
	readSampleArraySize = 4 * ShiftN;
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

#ifdef GET_LOCALMAXMIN
	vLocalMax = INT_MIN;
	vLocalMin = INT_MAX;
	vLocalMaxIdx = counter;
	vLocalMinIdx = counter;
#endif


}

void ECGProcessor::_addData(int32_t val) {
#ifdef FILTERING
	//averaging filter, could be inefficient
	//printf("%d, %d \n",val,prevVal);
	readSampleArray[curIdx] = ((val * NEW_COEFF) + (prevVal * OLD_COEFF))/FCO_MAX;
	DV(printf("%d,%d\n",val,readSampleArray[curIdx]);)
	prevVal = readSampleArray[curIdx];

#else
	readSampleArray[curIdx] = val;
#endif
	curIdx = (curIdx + 1) % readSampleArraySize;
	counter++;
	lastDataIdx = (lastDataIdx + 1) % readSampleArraySize;
	lastDataIdx_1 = (lastDataIdx_1 + 1) % readSampleArraySize;
	nBehind = (nBehind + 1) % readSampleArraySize;
	nBehind_1 = (nBehind_1 + 1) % readSampleArraySize;
#ifdef GET_LOCALMAXMIN
	if(val>vLocalMax) {
		vLocalMax = val;
		vLocalMaxIdx = counter;
	}
	if(val<vLocalMin) {
		vLocalMin = val;
		vLocalMinIdx = counter;
	}
#endif
	//printf("%d \n",curIdx);
}

void ECGProcessor::_appendEvent(int32_t absDiff, uint64_t counter, int32_t eventv) {
	//increment first, then add
	uint32_t prevEvIdx = eventIdx;
	eventIdx = (eventIdx + 1) % EVENTARR_SIZE;
	eventArray[eventIdx].diffSum = absDiff;
	eventArray[eventIdx].eventV = eventv;
	if (counter > (ShiftN_2 + 1)) {
		eventArray[eventIdx].idxSample = counter - ShiftN_2 - 1; //becasue I need the mid point
	} else {
		eventArray[eventIdx].idxSample = 1;//negative index
	}
	int32_t idxDiff = eventArray[eventIdx].idxSample - eventArray[prevEvIdx].idxSample;
	evFreq = (evFreq*9+idxDiff)/10;
	eventArray[eventIdx].deltaT = (float)(idxDiff)/Fs;
	DV(printf("AP EV %d,%f,%d\n",eventArray[eventIdx].diffSum,eventArray[eventIdx].deltaT,eventArray[eventIdx].idxSample);)

}

QRS ECGProcessor::process(int32_t val) {
	//check the crossing points
	//now check the values
	DV(printf("idx %d %d %d %d, idx:%d\n",nBehind_1,nBehind,lastDataIdx_1,lastDataIdx,curIdx);)
	//Add data first
	_addData(val);
	totalDataCount ++;
#ifdef USETIMING_FILTER
	QRSholder[0].valid = false; //mark this false and send to processing
#else
	QRSholder[1].valid = false; //mark this false and send to processing
#endif
	bool cond1 = (readSampleArray[lastDataIdx] <= readSampleArray[nBehind]) && (readSampleArray[lastDataIdx_1] > readSampleArray[nBehind_1]);
#ifdef ECG_NEG_POSSIBLE
	bool cond2 = (readSampleArray[lastDataIdx] > readSampleArray[nBehind]) && (readSampleArray[lastDataIdx_1] <= readSampleArray[nBehind_1]);
#else
	bool cond2 = false;
#endif
	if (cond1 || cond2) {
		DV(printf("event idx:%d\n",counter);)
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
		_appendEvent(absDiff, counter, readSampleArray[midPoint]);
		DV(printf("EV %d,%d,%d,%d,%d\n",midPoint_HalfN,midPoint_1,midPoint,midPlus1,lastDataIdx);)
		DV(printf("idx %d Ldiff %d %d , Rdiff %d %d\n",counter,ldiff0,ldiff1,rdiff0,rdiff1);)
		DV(printf("aDiff %d\n",absDiff);)
		_processEvent(eventIdx);
#ifdef GET_LOCALMAXMIN
		vLocalMax = INT_MIN;
		vLocalMin = INT_MAX;
		vLocalMaxIdx = counter;
		vLocalMinIdx = counter;
#endif

	}
#ifdef USETIMING_FILTER
	return QRSholder[0];
#else
	return QRSholder[1];
#endif

	DV(printf("%d,%d\n",readSampleArray[lastDataIdx],readSampleArray[nBehind]);)
}

void ECGProcessor::_timingFilter() {
#ifdef PROFILE_FUNC
	timingFilter++;
#endif
	int32_t tdiffs = (QRSholder[1].idx - QRSholder[0].idx);

	DV(printf("%d Tdiff %d Thresh %d dv %d\n",QRSholder[1].idx,tdiffs,Tthresh,QRSholder[1].dV);)
	if (tdiffs < Tthresh) {
		int32_t dvPrev = QRSholder[0].dV;
		int32_t dvNow = QRSholder[1].dV;
		uint32_t VPrev = abs(QRSholder[0].V - leakyMinV);
		uint32_t vNow = abs(QRSholder[1].V - leakyMinV);
		uint32_t VPrev2 = abs(QRSholder[0].V - leakyMaxV);
		uint32_t vNow2 = abs(QRSholder[1].V - leakyMaxV);
		//timing problem
		//Two scenarios, the new one can be higher dV in that case previous one is wrong
		bool highNoiseCond = (4 * vNow > 5 * VPrev ) || (4 * vNow2 > 5 * VPrev2 );
		bool lowNoiseCond = (4 * dvNow > 5 * dvPrev) || highNoiseCond;
		bool overallNoiseCond = lowNoiseCond;
		if(evFreq<NOISE_THRESH) {
			//high frequency noise, evFreq measure how many samples, lower the sample value higher the noise
			overallNoiseCond = highNoiseCond;
		}
		if (overallNoiseCond) {
			//new one is correct
			//don't change the dynamic threshold
			DV(printf("Ignore prev %d dv1 %d dv0 %d \n",QRSholder[0].idx, dvNow,dvPrev);)
			DV(printf("%d,%d\n",QRSholder[0].idx,QRSholder[0].V*2);)
			Tthresh = ((2 * Tthresh + minTthresh + tdiffs)) / 4;
			QRSholder[0].valid = true;
			QRSholder[0].idx = QRSholder[1].idx;
			QRSholder[0].V = QRSholder[1].V;
			QRSholder[0].dV = QRSholder[1].dV;
			QRSholder[0].prevGood = false;
		} else {
			//new dv is smaller
			//ignore this
			QRSholder[0].valid = false;
			DV(printf("Potentiol FP at %d %d dv0 %d dv1 %d\n",QRSholder[1].idx,Tthresh,QRSholder[0].dV, QRSholder[1].dV);)
		}
	} else {
		//include this in tThresh
		Tthresh = ((Tthresh + tdiffs)) / 3;
		DV(printf("TT %d tdiff %d\n",Tthresh,tdiffs);)
		//good to go
		QRSholder[0].valid = true;
		QRSholder[0].idx = QRSholder[1].idx;
		QRSholder[0].V = QRSholder[1].V;
		QRSholder[0].dV = QRSholder[1].dV;
		QRSholder[0].prevGood = true;
	}
}


void ECGProcessor::_processEvent(uint32_t eventIdx) {
	DV(printf("Ev %d,%f, %d, %d\n",eventArray[eventIdx].diffSum,eventArray[eventIdx].deltaT,eventArray[eventIdx].eventV,eventArray[eventIdx].idxSample);)
	int32_t eventV = eventArray[eventIdx].eventV;
	float timeDiff = eventArray[eventIdx].deltaT;
	int32_t gradDiff = eventArray[eventIdx].diffSum;
	int32_t vTimeDiff = leakyV * timeDiff;
	leakyV = abs(leakyMaxV - leakyMinV) / ShiftN;
	leakyMaxV = leakyMaxV - vTimeDiff;
	leakyMinV = leakyMinV + vTimeDiff;
	if (leakyMaxV < leakyMinV) {
		leakyMaxV = leakyMinV;
	}
	if (eventV > leakyMaxV) {
		leakyMaxV = eventV;
	} else if (eventV < leakyMinV) {
		leakyMinV = eventV;
	}
	dynamicThresh = dynamicThresh - threshDropRate * timeDiff;
	if (dynamicThresh < endThresh) {
		dynamicThresh = endThresh;
	}
	int32_t vmid = (leakyMaxV - leakyMinV)/2;
	if (dynamicThresh > vmid) {
		dynamicThresh = vmid;
	}
	//if noisy, drop slower
	threshDropRate = dynamicThresh;
	//any process start with assuming preve event is good
	QRSholder[1].prevGood = true;

	if (gradDiff > dynamicThresh) {
		//we have possible event
		endThresh = dynamicThresh / 5;
		thresholdArray[threshIdx] = gradDiff;
		//find mean
		uint32_t i = 0;
		int32_t sum = 0;
		for (i = 0; i < THRESH_ARR_SIZE; i++) {
			sum = sum + thresholdArray[i];
		}
		dynamicThresh = abs(sum / THRESH_ARR_SIZE);
		//unfiltered ECG event here
		DV(printf("gradDif %d endThresh %d dthresh %d id %d\n",gradDiff,endThresh,dynamicThresh,eventArray[eventIdx].idxSample);)
		threshIdx = (threshIdx + 1) % THRESH_ARR_SIZE;

		//add this to qrsholder array
		//check which direction the slope is at
#ifdef GET_LOCALMAXMIN
		if(readSampleArray[lastDataIdx_1]>readSampleArray[lastDataIdx]) {
			//negative slope
			//get maxidx
			QRSholder[1].idx = vLocalMaxIdx;//eventArray[eventIdx].idxSample;//changed recently
			QRSholder[1].V = vLocalMax;
			//printf("max\n");
		} else {
			QRSholder[1].idx = vLocalMinIdx;//eventArray[eventIdx].idxSample;//changed recently
			QRSholder[1].V = vLocalMin;
			//printf("min\n");
		}
#else
		QRSholder[1].V = eventV;
		QRSholder[1].idx = eventArray[eventIdx].idxSample;
#endif

		QRSholder[1].dV = gradDiff;

#ifdef USETIMING_FILTER
		_timingFilter();
#else
		QRSholder[1].valid = true;
#endif
		//if noisy, drop slower
		//update drop rate
		if(evFreq<NOISE_THRESH) {
			threshDropRate = (dynamicThresh*2)/3;
			DV(printf("Droptate changed %d %d %d\n",counter,evFreq,NOISE_THRESH);)
		}
		DV(printf("%ld,%d\n",(long)eventArray[eventIdx].idxSample,eventV);)
	}

}
