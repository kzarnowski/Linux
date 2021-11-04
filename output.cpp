/*******************************************************************************
Copyright (c) 1983-2012 Advantech Co., Ltd.
********************************************************************************
THIS IS AN UNPUBLISHED WORK CONTAINING CONFIDENTIAL AND PROPRIETARY INFORMATION
WHICH IS THE PROPERTY OF ADVANTECH CORP., ANY DISCLOSURE, USE, OR REPRODUCTION,
WITHOUT WRITTEN AUTHORIZATION FROM ADVANTECH CORP., IS STRICTLY PROHIBITED. 
================================================================================
REVISION HISTORY
--------------------------------------------------------------------------------
$Log: $
--------------------------------------------------------------------------------
$NoKeywords:  $
*/
/******************************************************************************
*
* Windows Example:
*    StaticAO.cpp
*
* Example Category:
*    AO
*
* Description:
*    This example demonstrates how to use Static AO  voltage function.
*
* Instructions for Running:
*    1  Set the 'deviceDescription' for opening the device. 
*    2  Set the 'channelStart' as the first channel for analog data Output  .
*    3  Set the 'channelCount' to decide how many sequential channels to output analog data.
*
* I/O Connections Overview:
*    Please refer to your hardware reference manual.
*
******************************************************************************/
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include "compatibility.h"
#include "bdaqctrl.h"
#include <profileapi.h>
#include<windows.h>
using namespace Automation::BDaq;
//-----------------------------------------------------------------------------------
// Configure the following three parameters before running the demo
//-----------------------------------------------------------------------------------
#define     ONE_WAVE_POINT_COUNT  512 //define how many data to makeup a waveform period.
#define     deviceDescription  L"PCI-1710HG,BID#0"

int32       channelStart = 0;
int32       channelCount = 1;


#define N 100
double result[N] = { 0 };
#define A 1.0
#define PI 3.14159265
#define Hz 1
int32 a = 1;
int T = 1 / Hz;
LARGE_INTEGER li;
void sinus();

LARGE_INTEGER xd(int i) {
	LARGE_INTEGER li;
	li.QuadPart = i;
	return li;

}

LARGE_INTEGER time = xd(((T * 1000 / N)*2922.344));


void sinus() {
	for (int i = 0; i < N; i++) result[i] = A + A*sin((2 *PI*i )/ N);
}

enum WaveStyle{ Sine, Sawtooth, Square };
//function GenerateWaveform: generate one waveform for each selected analog data output channel 
ErrorCode GenerateWaveform( InstantAoCtrl * instantAoCtrl,int32 channelStart,int32 channelCount,  double * waveBuffer, int32 SamplesCount,WaveStyle style);

inline void waitAnyKey()
{
   do{SLEEP(1);} while(!kbhit());
} 

int main(int argc, char* argv[])
{
	sinus();
	ErrorCode ret = Success;
	// Step 1: Create a 'InstantAoCtrl' for Static AO function.
	InstantAoCtrl * instantAoCtrl = AdxInstantAoCtrlCreate();
	do
	{
		// Step 2: Select a device by device number or device description and specify the access mode.
		// in this example we use AccessWriteWithReset(default) mode so that we can 
		// fully control the device, including configuring, sampling, etc.
		DeviceInformation devInfo(deviceDescription);
		ret = instantAoCtrl->setSelectedDevice(devInfo);
		CHK_RESULT(ret);

		// Step 3: Output data 
		// Generate waveform data

		/*
		double *waveform = (double*)malloc( channelCount*ONE_WAVE_POINT_COUNT*sizeof(double));
		if( NULL  == waveform )
		{
		   printf( "Insufficient memory available\n" );
		   break;
		}
		ret = GenerateWaveform( instantAoCtrl,channelStart,channelCount, waveform,channelCount*ONE_WAVE_POINT_COUNT,Sine);
		CHK_RESULT(ret);

		printf("\n Outputting data...  any key to quit!\n");
		bool enforced = false;
		do
		{
		   for( int32 i = 0; i < ONE_WAVE_POINT_COUNT; i++ )
		   {
			  ret = instantAoCtrl->Write( channelStart,channelCount,&waveform[channelCount*i] );
			  CHK_RESULT(ret);
			  SLEEP(1);
			  if(kbhit())
			  {
				 printf("\n Static AO is over compulsorily");
				 enforced = true;
				 break;
			  }
		   }
		} while (false);
		free(waveform);
		if (!enforced)
		{
		   printf("\n Static AO is over, press any key to quit!\n");
		}
	 */} while(false);

	 

		while (1) {
			//printf("XD");
			
			for (int i = 0; i < N; i++) {

				LARGE_INTEGER tt;
				QueryPerformanceCounter(&tt);
				do {
					QueryPerformanceCounter(&li);
				} while (li.QuadPart - tt.QuadPart < time.QuadPart);
						
				//printf("XD");
				//QueryPerformanceFrequency(&li);
				//printf("%ld\t", li); 
				//printf("XD");
				instantAoCtrl->Write(0, result[i]);
				//instantAoCtrl->Write(0, a);
				//2922.344
			}
		}
		// Step 4: Close device and release any allocated resource.
		instantAoCtrl->Dispose();

		// If something wrong in this execution, print the error code on screen for tracking.
		if (BioFailed(ret))
		{
			printf("Some error occurred. And the last error code is Ox%X.\n", ret);
			waitAnyKey();// Wait any key to quit!.
		}

		waitAnyKey();// Wait any key to quit !
		return 0;
	}

	/*
	ErrorCode GenerateWaveform( InstantAoCtrl * instantAoCtrl, int32 channelStart,int32 channelCount,double * waveBuffer,int32 SamplesCount,WaveStyle style)
	{
	   ErrorCode ret = Success;
	   int32    channel = 0;
	   int32    channelCountMax = 0;
	   int32    oneWaveSamplesCount = SamplesCount/channelCount;
	   int32    i = 0;

	   MathInterval  ranges[64] ;
	   ValueRange valRange;
	   channelCountMax =  instantAoCtrl->getFeatures()->getChannelCountMax();
	   for(i = 0;i < channelCountMax ;i++ )
	   {
		  valRange = instantAoCtrl->getChannels()->getItem(i).getValueRange();
		  if ( V_ExternalRefBipolar == valRange || valRange == V_ExternalRefUnipolar )
		  {
			 if (instantAoCtrl->getFeatures()->getExternalRefAntiPolar())
			 {
				double referenceValue;

				if (valRange == V_ExternalRefBipolar)
				{
					referenceValue = instantAoCtrl->getExtRefValueForBipolar();
					if (referenceValue >= 0) {
						ranges[i].Max = referenceValue;
						ranges[i].Min = 0 - referenceValue;
					} else {
						ranges[i].Max = 0 - referenceValue;
						ranges[i].Min = referenceValue;
					}
				}
				else
				{
					referenceValue = instantAoCtrl->getExtRefValueForUnipolar();
					if (referenceValue >= 0) {
						ranges[i].Max = 0;
						ranges[i].Min = 0 - referenceValue;
					} else {
						ranges[i].Max = 0 - referenceValue;
						ranges[i].Min = 0;
					}
				}
			 }
			 else
			 {
				double referenceValue;

				if (valRange == V_ExternalRefBipolar)
				{
					referenceValue = instantAoCtrl->getExtRefValueForBipolar();
					if (referenceValue >= 0) {
						ranges[i].Max = referenceValue;
						ranges[i].Min = 0 - referenceValue;
					} else {
						ranges[i].Max = 0 - referenceValue;
						ranges[i].Min = referenceValue;
					}
				}
				else
				{
					referenceValue = instantAoCtrl->getExtRefValueForUnipolar();
					if (referenceValue >= 0) {
						ranges[i].Max = referenceValue;
						ranges[i].Min = 0;
					} else {
						ranges[i].Max = 0;
						ranges[i].Min = referenceValue;
					}
				}
			 }
		  }
		  else {
			 ret = AdxGetValueRangeInformation( valRange,0,NULL,&ranges[i],NULL);
			 if(BioFailed(ret))
			 {
				return ret;
			 }
		  }
	   }

	   //generate waveform data and put them into the buffer which the parameter 'waveBuffer' give in, the Amplitude these waveform
	   for(i = 0; i < oneWaveSamplesCount; i++ )
	   {
		  for( int32 j = channelStart; j < channelStart+channelCount; j++ )
		  {
			 //pay attention to channel rollback(when startChannel+channelCount>chNumberMax+1)
			 channel = j%channelCountMax;

			 double amplitude = (ranges[channel].Max - ranges[channel].Min) / 2;
			 double offset = (ranges[channel].Max + ranges[channel].Min) / 2;

			 switch ( style)
			 {
			 case Sine:
				*waveBuffer++ = amplitude*(sin((double)i*2.0*( 3.14159 )/oneWaveSamplesCount )) + offset;
				break;
			 case  Sawtooth:
				if ((i >= 0) && (i < (oneWaveSamplesCount / 4.0)))
				{
				   *waveBuffer++ =  amplitude*( i/(oneWaveSamplesCount/4.0)) + offset;
				}
				else
				{
				   if ((i >= (oneWaveSamplesCount / 4.0)) && (i < 3 * (oneWaveSamplesCount/4.0)))
				   {
					  *waveBuffer++ = amplitude* ((2.0*(oneWaveSamplesCount/4.0)-i)/(oneWaveSamplesCount/4.0)) + offset;
				   }
				   else
				   {
					  *waveBuffer++ = amplitude* ((i-oneWaveSamplesCount)/(oneWaveSamplesCount/4.0)) + offset;
				   }
				}
				break;
			 case  Square:
				if ((i >= 0) && (i < (oneWaveSamplesCount / 2)))
				{
				   *waveBuffer++ = amplitude * 1 + offset;
				}
				else
				{
				   *waveBuffer++ = amplitude * (-1) + offset;
				}
				break;
			 default:
				printf("invalid wave style,generate waveform error !");
				ret = ErrorUndefined;
			 }
		  }
	   }
	   return ret;
	};
	*/
