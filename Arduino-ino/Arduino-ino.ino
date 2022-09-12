
#define PROCESS_DATA  1

#ifdef PROCESS_DATA
#include "eventECG.h"
#endif
#include <Arduino.h>
#include <pins_arduino.h>

#define GPIOPIN       9
#define GPIOPIN2      11

int processinit = 0;

void setup() {
  pinMode(GPIOPIN, OUTPUT);
  pinMode(GPIOPIN2, OUTPUT);
  Serial.begin(3686400);
  Serial1.begin(2000000);
  while (!Serial || !Serial1) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

}


void loop() {
  
#ifdef PROCESS_DATA
  int16_t curVal;
  int32_t successCount = 0;
  int32_t totalCount = 0;
  int32_t validPrevx = 0;
  QRS qrsHolder;
  int32_t t_tmp = 0, HRval = 0;
  int32_t minTThreshval = 0;
  ECGProcessor processor(360);
  minTThreshval = processor.minTthresh / 2;
  QRS qrsOld, qrsNew;
  qrsOld.valid = false;
  qrsNew.valid = false;
  qrsOld.idx = 0;
  qrsOld.V = 0;
  qrsNew.idx = 0;
  qrsNew.V = 0;
#endif
  while (1) {
    // print the string when a newline arrives:
    int bcount = 0;
    int16_t input = 0;
    while (Serial.available()) {
      // get the new byte:
      digitalWrite(GPIOPIN2, HIGH);
      byte inChar = (byte)Serial.read();
      input = (input << 8) | inChar;
      bcount++;
      if (bcount == 2) {
        digitalWrite(GPIOPIN2, LOW);
        //Serial1.println(input);
#ifdef PROCESS_DATA
        digitalWrite(GPIOPIN, HIGH);
        qrsHolder = processor.process((int32_t)input);
        if (qrsHolder.valid == true) {
          qrsNew = qrsHolder;
          if (qrsOld.valid && qrsHolder.prevGood) {
            //calculate HR
            t_tmp = qrsOld.idx - validPrevx;
            if (t_tmp > minTThreshval) {
              HRval = ((360 * 60000) / (qrsOld.idx - validPrevx));
              //dispatch old
              successCount++;
              //print numbers
              Serial1.print(qrsHolder.idx);
              Serial1.print(",");
              Serial1.println(qrsHolder.V);

              validPrevx = qrsOld.idx;
              qrsOld = qrsNew;
            } else {
              qrsOld = qrsNew;
            }
          } else {
            qrsOld = qrsNew;
          }
        }
        digitalWrite(GPIOPIN, LOW);
        totalCount++;

#endif
        bcount = 0;
        break;
      }
    }
  }
}
