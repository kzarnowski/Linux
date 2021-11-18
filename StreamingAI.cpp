/*******************************************************************************
Copyright (c) 1983-2012 Advantech Co., Ltd.
********************************************************************************
THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY INFORMATION
WHICH IS THE PROPERTY OF ADVANTECH CORP., ANY DISCLOSURE, USE, OR REPRODUCTION,
WITHOUT WRITTEN AUTHORIZATION FROM ADVANTECH CORP., IS STRICTLY PROHIBITED.

================================================================================
REVISION HISTORY
--------------------------------------------------------------------------------
$Log:  $
--------------------------------------------------------------------------------
$NoKeywords:  $
*/
/******************************************************************************
*
* Windows Example:
*    StreamingAI.cpp
*
* Example Category:
*    AI
*
* Description:
*    This example demonstrates how to use Streaming AI function.
*
* Instructions for Running:
*    1. Set the 'deviceDescription' which can get from system device manager for opening the device.
*    2. Set the 'startChannel' as the first channel for scan analog samples
*    3. Set the 'channelCount' to decide how many sequential channels to scan analog samples.
*    4. Set the 'intervalCount'to decide what occasion to signal event; when one section it is
*       capacity decided by 'intervalCount*channelCount' in kernel buffer(the capacity decided
*       by 'sampleCount*channelCount' )is filled,driver signal a 'DataReady' event to notify APP.
*       ( ***Notes: the buffer is divided up with many sections begin with buffer's head, the last section
*                   may not be equal to 'intervalCount*channelCount' if the 'sampleCount'
*                   is not an integer multiple of 'intervalCount',but the last section is filled ,
*                   driver signal 'DataReady' event too. ***)
*    5. Set the 'sampleCount' to decide the capacity of buffer in kernel.
*
* I/O Connections Overview:
*    Please refer to your hardware reference manual.
*
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "lib/compatibility.h"
#include "lib/bdaqctrl.h"
using namespace Automation::BDaq;
//-----------------------------------------------------------------------------------
// Configure the following five parameters before running the demo
//-----------------------------------------------------------------------------------
#define       deviceDescription  L"PCI-1710HG,BID#0"
int32         startChannel = 1;
const int32   channelCount = 1;
const int32   intervalCount = 2000;  // for each channel, Ref. 'Instructions for Running' on top of current document 
const int32   sampleCount = 2000;    // for each channel, to decide the capacity of buffer in kernel.
												 // Recommend: sampleCount is an integer multiple of intervalCount, and equal to twice or greater
// user buffer size should be equal or greater than raw data buffer length, because data ready count

// is equal or more than smallest section of raw data buffer and up to raw data buffer length.
// users can set 'USER_BUFFER_SIZE' according to demand.
#define		  USER_BUFFER_SIZE    channelCount*sampleCount 
double        userDataBuffer[USER_BUFFER_SIZE];

double constant;
double max;
int I;
#define F 10000.0

inline void waitAnyKey()
{
	do { SLEEP(1); } while (!kbhit());
}
// This class is used to deal with 'DataReady' Event, we should overwrite the virtual function BfdAiEvent.
class DataReadyHandler : public BfdAiEventListener
{
public:
	virtual void BDAQCALL BfdAiEvent(void * sender, BfdAiEventArgs * args)
	{
		//printf("Streaming AI data ready: count = %d\n", args->Count);
		BufferedAiCtrl * bufferedAiCtrl = (BufferedAiCtrl*)sender;
		int chanCount = bufferedAiCtrl->getScanChannel()->getChannelCount();
		int32 chanStart = bufferedAiCtrl->getScanChannel()->getChannelStart();
		int channelCountMax = bufferedAiCtrl->getFeatures()->getChannelCountMax();

		bufferedAiCtrl->GetData(args->Count, userDataBuffer);

		// in this demo, we show only the first sample of each channel's new data
		//printf("the first sample for each Channel are:\n");
		
		

			if (I % 10 == 0) {
				double sum = 0;
				for (int32 j = 0; j < sampleCount; j++) {
					//printf("j: %d :%10.6f \n", j,  userDataBuffer[j]);
					sum += userDataBuffer[j];

				}
				constant = sum / sampleCount;
				//printf("%lf\n", constant);

				sum = 0;
				for (int32 j = 0; j < sampleCount; j++) {
					userDataBuffer[j] = fabs(userDataBuffer[j] - constant);
					sum += userDataBuffer[j];
				}
				max = sum / sampleCount;
				printf("MAX: %lf\n", max);
			}
			else {
				for (int j = 0; j < sampleCount; j++) {
					userDataBuffer[j] = fabs(userDataBuffer[j] - constant);
				}

				int l = -1, r = -1;
				for (int32 j = 0; j < sampleCount; j++) {
					if (userDataBuffer[j] > 2 * max) {
						l = j;
						break;
					}
				}

				for (int32 j = sampleCount - 1; j >= 0; j--) {
					if (userDataBuffer[j] > 2 * max) {
						r = j;
						break;
					}
				}

				if (l != r && r >= l + 10) {
					//printf("L: %d %lf R: %d %lf Klask klask", l, userDataBuffer[l], r, userDataBuffer[r]);
					printf("Klasniecie\n");
				}
				else {
					//printf("l == r\n");
					printf("Sluchanie\n");
				}
			}
		
			I++;
   }
};

// This class is used to deal with 'Overrun' Event, we should overwrite the virtual function BfdAiEvent.
class OverrunHandler : public BfdAiEventListener
{
public:
   virtual void BDAQCALL BfdAiEvent(void * sender, BfdAiEventArgs * args)
   {
      printf("Streaming AI Overrun: offset = %d, count = %d\n", args->Offset, args->Count);
   }
};

// This class is used to deal with 'CacheOverflow' Event, we should overwrite the virtual function BfdAiEvent.
class CacheOverflowHandler : public BfdAiEventListener
{
public:
   virtual void BDAQCALL BfdAiEvent(void * sender, BfdAiEventArgs * args)
   {
      printf(" Streaming AI Cache Overflow: offset = %d, count = %d\n", args->Offset, args->Count);
   }
};

// This class is used to deal with 'Stopped' Event, we should overwrite the virtual function BfdAiEvent.
class StoppedHandler : public BfdAiEventListener
{
public:
   virtual void BDAQCALL BfdAiEvent(void * sender, BfdAiEventArgs * args)
   {
      printf("Streaming AI stopped: offset = %d, count = %d\n", args->Offset, args->Count);
   }
};

int main(int argc, char* argv[])
{
	I = 0;
   ErrorCode        ret = Success;

   // Step 1: Create a 'BufferedAiCtrl' for buffered AI function.
   BufferedAiCtrl * bfdAiCtrl = AdxBufferedAiCtrlCreate();
   bfdAiCtrl->getConvertClock()->setRate(F);

	// Step 2: Set the notification event Handler by which we can known the state of operation effectively.
	DataReadyHandler     onDataReady;
	OverrunHandler       onOverrun;
	CacheOverflowHandler onCacheOverflow;
	StoppedHandler       onStopped;
	bfdAiCtrl->addDataReadyListener(onDataReady);
	bfdAiCtrl->addOverrunListener(onOverrun);
	bfdAiCtrl->addCacheOverflowListener(onCacheOverflow);
	bfdAiCtrl->addStoppedListener(onStopped);
	do
	{
		// Step 3: Select a device by device number or device description and specify the access mode.
		// in this example we use AccessWriteWithReset(default) mode so that we can 
		// fully control the device, including configuring, sampling, etc.
		DeviceInformation devInfo(deviceDescription);
		ret = bfdAiCtrl->setSelectedDevice(devInfo);
		CHK_RESULT(ret);

		// Step 4: Set necessary parameters for Buffered AI operation, 
		// Note: some of operation of this step is optional(you can do these settings via "Device Configuration" dialog).
		ScanChannel* scanChannel = bfdAiCtrl->getScanChannel();
		ret = scanChannel->setChannelStart(startChannel);
		CHK_RESULT(ret);
		ret = scanChannel->setChannelCount(channelCount);
		CHK_RESULT(ret);
		ret = scanChannel->setIntervalCount(intervalCount);
		CHK_RESULT(ret);
		ret = scanChannel->setSamples(sampleCount);
		CHK_RESULT(ret);
		ret = bfdAiCtrl->setStreaming(true);// specify the running mode: streaming buffered.
		CHK_RESULT(ret);

		 // Step 5: Prepare the buffered AI. 
		ret = bfdAiCtrl->Prepare();
		CHK_RESULT(ret);

	
      // Step 6: Start buffered AI, the method will return immediately after the operation has been started.
      // We can get samples via event handlers.
      ret = bfdAiCtrl->Start();
      CHK_RESULT(ret);

      // Step 7: Do anything you are interesting while the device is acquiring data.
      printf("Streaming AI is in progress.\nplease wait...  any key to quit!\n\n");
      do
      {
         // do something yourself !
         SLEEP(1);
      }	while(!kbhit());

      // step 8: Stop the operation if it is running.
      ret = bfdAiCtrl->Stop(); 
      CHK_RESULT(ret);
   }while(false);

   // Step 9: Close device, release any allocated resource.
   bfdAiCtrl->Dispose();
	
	// If something wrong in this execution, print the error code on screen for tracking.
   if(BioFailed(ret))
   {
      printf("Some error occurred. And the last error code is Ox%X.\n", ret);
      waitAnyKey();// wait any key to quit!
   }
   return 0;
}



