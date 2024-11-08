/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2burn/main.cpp
	@brief		Demonstration application that "burns" timecode into frames captured from SDI input,
				and playout those modified frames to SDI output.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.  All rights reserved.
**/


//	Includes
#include "ntv2burn.h"
#include <signal.h>
#include <iostream>
#include <iomanip>

using namespace std;


//	Globals
static bool	gGlobalQuit		(false);	///< @brief	Set this "true" to exit gracefully


static void SignalHandler (int inSignal)
{
	(void) inSignal;
	gGlobalQuit = true;
}


int main (int argc, const char ** argv)
{
	char *			pDeviceSpec		(AJA_NULL);		//	Which device to use
	char *			pInputSrcSpec	(AJA_NULL);		//	SDI source spec
	char *			pOutputDest		(AJA_NULL);		//	SDI output spec (sdi1 ... sdi8)
	char *			pTcSource		(AJA_NULL);		//	Time code source string
	char *			pPixelFormat	(AJA_NULL);		//	Pixel format spec
	char *			pInFramesSpec	(AJA_NULL);		//	Input AutoCirculate frames spec
	char *			pOutFramesSpec	(AJA_NULL);		//	Output AutoCirculate frames spec
	int				doMultiFormat	(0);			//	MultiFormat mode?
	int				showVersion		(0);			//	Show version?
	int				noAudio			(0);			//	Disable audio?
	int				noVideo			(0);			//	Disable video?
	int				noAnc			(0);			//	Disable capture/playback of anc data?
	int				doVanc			(0);			//	Use tall-frame VANC?
	int				verbose			(0);			//	Verbose mode?
	AJADebug::Open();

	//	Command line option descriptions:
	const struct poptOption optionsTable [] =
	{
		{"version",		  0,	POPT_ARG_NONE,		&showVersion,	0,	"show version & exit",		AJA_NULL					},
		{"device",		'd',	POPT_ARG_STRING,	&pDeviceSpec,	0,	"device to use",			"index#, serial#, or model"	},
		{"multiFormat",	'm',	POPT_ARG_NONE,		&doMultiFormat,	0,	"use multi-format/channel",	AJA_NULL					},
		{"pixelFormat",	'p',	POPT_ARG_STRING,	&pPixelFormat,	0,	"pixel format to use",		"'?' or 'list' to list"		},
		{"input",		'i',	POPT_ARG_STRING,	&pInputSrcSpec,	0,	"input to use",             "1-8, ?=list"				},
		{"output",		'o',	POPT_ARG_STRING,	&pOutputDest,	0,	"output to use",			"'?' or 'list' to list"		},
		{"noaudio",		0,		POPT_ARG_NONE,		&noAudio,		0,	"disable audio?",			AJA_NULL					},
		{"novideo",		0,		POPT_ARG_NONE,		&noVideo,		0,	"disable video?",			AJA_NULL					},
		{"noanc",		0,		POPT_ARG_NONE,		&noAnc,			0,	"disable anc?",				AJA_NULL					},
		{"vanc",		0,		POPT_ARG_NONE,		&doVanc,		0,	"enable tall-frame VANC?",	AJA_NULL					},
		{"verbose",		0,		POPT_ARG_NONE,		&verbose,		0,	"verbose mode?",			AJA_NULL					},
		{"iframes",		0,		POPT_ARG_STRING,	&pInFramesSpec,	0,	"input AutoCirc frames",	"num[@min] or min-max"		},
		{"oframes",		0,		POPT_ARG_STRING,	&pOutFramesSpec,0,	"output AutoCirc frames",	"num[@min] or min-max"		},
		{"tcsource",	't',	POPT_ARG_STRING,	&pTcSource,		0,	"time code source",			"'?' to list"				},
		POPT_AUTOHELP
		POPT_TABLEEND
	};
	CNTV2DemoCommon::Popt popt(argc, argv, optionsTable);
	if (!popt)
		{cerr << "## ERROR: " << popt.errorStr() << endl;  return 2;}
	if (showVersion)
		{cout << argv[0] << ", NTV2 SDK " << ::NTV2Version() << endl;  return 0;}

	//	Device
	const string deviceSpec (pDeviceSpec ? pDeviceSpec : "0");
	if (!CNTV2DemoCommon::IsValidDevice(deviceSpec))
		return 1;

	BurnConfig config(deviceSpec);

	//	Input source
	const string inputSourceStr	(pInputSrcSpec ? CNTV2DemoCommon::ToLower(string(pInputSrcSpec)) : "");
	const string legalSources (CNTV2DemoCommon::GetInputSourceStrings(NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : ""));
	config.fInputSource = CNTV2DemoCommon::GetInputSourceFromString(inputSourceStr, NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : "");
	if (inputSourceStr == "?" || inputSourceStr == "list")
		{cout << legalSources << endl;  return 0;}
	if (!inputSourceStr.empty()  &&  !NTV2_IS_VALID_INPUT_SOURCE(config.fInputSource))
		{cerr << "## ERROR:  Input source '" << inputSourceStr << "' not one of:" << endl << legalSources << endl;	return 1;}

	//	Output destination
	const string outputDestStr (pOutputDest ? CNTV2DemoCommon::ToLower(string(pOutputDest)) : "");
	const string legalOutputs (CNTV2DemoCommon::GetOutputDestinationStrings(NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : ""));
	config.fOutputDest = CNTV2DemoCommon::GetOutputDestinationFromString(outputDestStr, NTV2_IOKINDS_ALL, pDeviceSpec ? deviceSpec : "");
	if (outputDestStr == "?" || outputDestStr == "list")
		{cout << legalOutputs << endl;  return 0;}
	if (!outputDestStr.empty() && !NTV2_IS_VALID_OUTPUT_DEST(config.fOutputDest))
		{cerr << "## ERROR:  Output '" << outputDestStr << "' not of:" << endl << legalOutputs << endl;	return 1;}

	//	Pixel Format
	const string pixelFormatStr (pPixelFormat  ?  pPixelFormat  :  "");
	config.fPixelFormat = pixelFormatStr.empty() ? NTV2_FBF_8BIT_YCBCR : CNTV2DemoCommon::GetPixelFormatFromString(pixelFormatStr);
	if (pixelFormatStr == "?"  ||  pixelFormatStr == "list")
		{cout << CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, pDeviceSpec ? deviceSpec : "") << endl;  return 0;}
	if (!pixelFormatStr.empty()  &&  !NTV2_IS_VALID_FRAME_BUFFER_FORMAT(config.fPixelFormat))
	{
		cerr	<< "## ERROR:  Invalid '--pixelFormat' value '" << pixelFormatStr << "' -- expected values:" << endl
				<< CNTV2DemoCommon::GetPixelFormatStrings(PIXEL_FORMATS_ALL, deviceSpec) << endl;
		return 2;
	}

	//	Timecode source...
	const string tcSourceStr (pTcSource ? CNTV2DemoCommon::ToLower(pTcSource) : "");
	const string legalTCSources (CNTV2DemoCommon::GetTCIndexStrings(TC_INDEXES_ALL, pDeviceSpec ? deviceSpec : ""));
	config.fTimecodeSource = CNTV2DemoCommon::GetTCIndexFromString(tcSourceStr);
	if (tcSourceStr == "?"  ||  tcSourceStr == "list")
		{cout << legalTCSources << endl;  return 0;}
	if (!tcSourceStr.empty()  &&  !NTV2_IS_VALID_TIMECODE_INDEX(config.fTimecodeSource))
		{cerr << "## ERROR:  Timecode source '" << tcSourceStr << "' not one of these:" << endl << legalTCSources << endl;	return 1;}

	//	InputFrames
	string framesSpec(pInFramesSpec ? pInFramesSpec : "");
	static const string	legalFrmSpec("{frameCount}[@{firstFrameNum}]  or  {firstFrameNum}-{lastFrameNum}");
	if (!framesSpec.empty())
	{	const string parseResult(config.fInputFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad '--iframes' spec '" << framesSpec << "'" << endl << "## " << parseResult << endl;  return 1;}
	}
	if (!config.fInputFrames.valid())
		{cerr << "## ERROR:  Bad '--iframes' spec '" << framesSpec << "'" << endl << "## Expected " << legalFrmSpec << endl;  return 1;}

	//	OutputFrames
	framesSpec = pOutFramesSpec ? pOutFramesSpec : "";
	if (!framesSpec.empty())
	{	const string parseResult(config.fOutputFrames.setFromString(framesSpec));
		if (!parseResult.empty())
			{cerr << "## ERROR:  Bad '--oframes' spec '" << framesSpec << "'" << endl << "## " << parseResult << endl;  return 1;}
	}
	if (!config.fOutputFrames.valid())
		{cerr << "## ERROR:  Bad '--oframes' spec '" << framesSpec << "'" << endl << "## Expected " << legalFrmSpec << endl;  return 1;}

	if (noVideo  &&  noAudio)
		{cerr	<< "## ERROR:  '--novideo' and '--noaudio' cannot both be specified" << endl;  return 1;}
	config.fDoMultiFormat	= doMultiFormat ? true : false;
	config.fSuppressAudio	= noAudio ? true  : false;
	config.fSuppressVideo	= noVideo ? true  : false;
	config.fVerbose			= verbose ? true  : false;
	config.fWithAnc			= noAnc   ? false : true;
	config.fWithTallFrames	= doVanc  ? true  : false;
	if (!config.fWithAnc  &&  config.fWithTallFrames)
		{cerr	<< "## ERROR:  '--noanc' and '--vanc' are contradictory" << endl;  return 1;}
	if (!config.fWithAnc  &&  config.fWithHanc)
		{cerr	<< "## ERROR:  '--noanc' and '--hanc' are contradictory" << endl;  return 1;}

	//	Instantiate the NTV2Burn object...
	NTV2Burn burner (config);

	//	Initialize the NTV2Burn instance...
	AJAStatus status (burner.Init());
	if (AJA_FAILURE(status))
		{cerr << "## ERROR:  Initialization failed, status=" << status << endl;  return 4;}

	::signal (SIGINT, SignalHandler);
	#if defined (AJAMac)
		::signal (SIGHUP, SignalHandler);
		::signal (SIGQUIT, SignalHandler);
	#endif

	//	Start the burner's capture and playout threads...
	burner.Run();

	//	Loop until told to stop...
	cout	<< "           Capture  Playout  Capture  Playout" << endl
			<< "   Frames   Frames   Frames   Buffer   Buffer" << endl
			<< "Processed  Dropped  Dropped    Level    Level" << endl;
	do
	{
		AUTOCIRCULATE_STATUS inputStatus, outputStatus;
		burner.GetStatus (inputStatus, outputStatus);
		cout	<< setw(9) << inputStatus.GetProcessedFrameCount()
				<< setw(9) << inputStatus.GetDroppedFrameCount()
				<< setw(9) << outputStatus.GetDroppedFrameCount()
				<< setw(9) << inputStatus.GetBufferLevel()
				<< setw(9) << outputStatus.GetBufferLevel()
				<< "\r" << flush;
		AJATime::Sleep(2000);
	} while (!gGlobalQuit);	//	loop until signaled

	cout << endl;
	return 0;

}	//	main
