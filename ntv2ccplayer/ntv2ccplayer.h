/**
	@file		ntv2ccplayer.cpp
	@brief		Header file for NTV2CCPlayer demonstration class
	@copyright	(C) 2012-2020 AJA Video Systems, Inc.  All rights reserved.
**/


#ifndef _NTV2CCPLAYER_H
#define _NTV2CCPLAYER_H

#include "ntv2devicescanner.h"
#include "ntv2democommon.h"
#include "ntv2testpatterngen.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/info.h"
#include "ajacc/includes/ajacc.h"
#include <vector>
#include <map>

#define	SIG_AJA_STOP	31		//	Our own user-defined "stop" signal


/**
	@brief	These are the actions that can be taken after the last file is "played".
**/
typedef enum _AtEndAction_
{
	AtEndAction_Quit,		///	Terminate
	AtEndAction_Repeat,		///	Repeat the file list (must Ctrl-C to terminate)
	AtEndAction_Idle,		///	Continue to emit gray field on the device (must Ctrl-C to terminate)
	AtEndAction_Max
} AtEndAction;


//	Declaration for function that sets the master "quit" flag (see main.cpp).
void SignalHandler (int inSignal);


/**
	@brief	This class is used to configure a caption generator for a single caption channel.
**/
typedef struct CCGeneratorConfig
{
	public:
		NTV2StringList			fFilesToPlay;			///< @brief	A list of zero or more strings containing paths to text files to be "played"
		AtEndAction				fEndAction;				///< @brief	The action to take after the file list has finished playing
		NTV2Line21Mode			fCaptionMode;			///< @brief	The CEA-608 caption mode to use
		NTV2Line21Channel		fCaptionChannel;		///< @brief	The caption channel to use
		bool					fNewLinesAreNewRows;	///< @brief	If true, newlines break caption rows; otherwise are treated as whitespace
		double					fCharsPerMinute;		///< @brief	The rate at which caption characters get enqueued, in characters per minute
		NTV2Line21Attributes	fAttributes;			///< @brief	The character attributes to use

		/**
			@brief	Constructs a default generator configuration.
		**/
		inline explicit	CCGeneratorConfig ()
			:	fFilesToPlay		(NTV2StringList()),
				fEndAction			(AtEndAction_Quit),
				fCaptionMode		(NTV2_CC608_CapModeRollUp4),
				fCaptionChannel		(NTV2_CC608_CC1),
				fNewLinesAreNewRows	(false),
				fCharsPerMinute		(500),
				fAttributes			()
		{
		}

		AJALabelValuePairs Get(void)const;
}	CCGeneratorConfig;


typedef std::map <NTV2Line21Channel, CCGeneratorConfig>		CaptionChanGenMap;
typedef	CaptionChanGenMap::const_iterator					CaptionChanGenMapCIter;
typedef CaptionChanGenMap::iterator							CaptionChanGenMapIter;


/**
	@brief	This class is used to configure an NTV2CCPlayer instance.
**/
typedef struct CCPlayerConfig
{
	public:
		std::string						fDeviceSpecifier;		///< @brief	The AJA device to use
		NTV2Channel						fOutputChannel;			///< @brief	The device channel to use
		NTV2OutputDestination			fOutputDestination;		///< @brief	The desired output connector to use
		CNTV2DemoCommon::ACFrameRange	fFrames;				///< @brief	AutoCirculate frame count or range
		bool							fEmitStats;				///< @brief	If true, show stats while playing; otherwise echo caption text being played
		bool							fDoMultiFormat;			///< @brief	If true, use multi-format/multi-channel mode, if device supports it; otherwise normal mode
		bool							fForceVanc;				///< @brief	If true, force the use of Vanc, even if the device supports Anc insertion
		bool							fSuppressLine21;		///< @brief	SD output only:  if true, do not encode Line 21 waveform;  otherwise encode Line 21 waveform
		bool							fSuppress608;			///< @brief	If true, don't transmit CEA608 packets;  otherwise include 608 packets
		bool							fSuppress708;			///< @brief	If true, don't transmit CEA708 packets;  otherwise include 708 packets
		bool							fSuppressAudio;			///< @brief	If true, suppress audio;  otherwise generate audio tones
		bool							fSuppressTimecode;		///< @brief	If true, suppress timecode;  otherwise embed VITC/LTC
		bool							fDualLinkRGB;			///< @brief	If true, route dual-link RGB output;  otherwise normal YCbCr
		bool							fSquareDivision;		///< @brief	If true, square-division used for 4K/UHD;  otherwise default is TSI
		uint16_t						fForceRTP;				///< @brief	BIT(0):0=normal,1=forceRTP  BIT(1):0=uniPkt,1=multiPkt  BIT(2):0=normal,1=patchDeviceID
		NTV2VideoFormat					fVideoFormat;			///< @brief	The video format to use
		NTV2FrameBufferFormat			fPixelFormat;			///< @brief	The pixel format to use
		std::string						fTestPatternName;		///< @brief	The test pattern to use
		CaptionChanGenMap				fChannelGenerators;		///< @brief	Caption channel generators

		/**
			@brief	Constructs a default CCPlayer configuration.
		**/
		inline explicit	CCPlayerConfig (const std::string & inDeviceSpecifier	= "0")
			:	fDeviceSpecifier	(inDeviceSpecifier),
				fOutputChannel		(NTV2_CHANNEL1),
				fOutputDestination	(NTV2_OUTPUTDESTINATION_INVALID),
				fFrames				(7),
				fEmitStats			(true),
				fDoMultiFormat		(false),
				fForceVanc			(false),
				fSuppressLine21		(false),
				fSuppress608		(false),
				fSuppress708		(false),
				fSuppressAudio		(false),
				fSuppressTimecode	(false),
				fDualLinkRGB		(false),
				fSquareDivision		(false),
				fForceRTP			(0),
				fVideoFormat		(NTV2_FORMAT_525_5994),
				fPixelFormat		(NTV2_FBF_10BIT_YCBCR),
				fTestPatternName	("Flat Field"),
				fChannelGenerators	()
		{
		}
		AJALabelValuePairs Get(const bool inCompact = false)const;

}	CCPlayerConfig;

std::ostream &	operator << (std::ostream & ioStrm, const CCPlayerConfig & inObj);



/**
	@brief	I am an object that can inject text captions into an SDI output of an AJA device in real time. I'm capable
			of simultaneously generating and injecting text into more than one NTV2Line21Channel, at independent rates,
			and NTV2Line21Modes. Each caption channel's configuration is specified by a CCGeneratorConfig struct.
			I make use of the AJACircularBuffer, which simplifies implementing a producer/consumer model,
			in which a "producer" thread produces the background video frames, and a "consumer" thread
			(i.e., the "play" thread) sends those frames to the AJA device.
			I demonstrate how to use the "AJA CC LIB" caption library to embed captions into an SDI output signal.
**/

class NTV2CCPlayer
{
	//	Public Instance Methods
	public:
		/**
			@brief	Constructs me using the given configuration settings.
			@note	I'm not completely initialized and ready for use until after my Init method has been called.
			@param[in]	inConfigData		Specifies the player configuration.
		**/
								NTV2CCPlayer (const CCPlayerConfig & inConfigData);

		virtual 				~NTV2CCPlayer (void);

		/**
			@brief	Initializes me and prepares me to Run.
		**/
		virtual AJAStatus		Init (void);

		/**
			@brief	Runs me.
			@note	Do not call this method without first calling my Init method.
		**/
		virtual AJAStatus		Run (void);

		/**
			@brief	Stops me.
			@param[in]	inQuitImmediately	If true, terminates the caption player immediately; otherwise,
											waits until all captions enqueued in the encoder have finished
											playing. Defaults to false;
			@note	This method will block until my caption generator and playout threads have terminated.
		**/
		virtual void			Quit (const bool inQuitImmediately = false);


		/**
			@brief	Returns status information from my caption encoder.
			@param[out]	outMessagesQueued	Receives the current number of messages currently enqueued in my caption encoder.
			@param[out]	outBytesQueued		Receives the current number of bytes currently enqueued in my caption encoder.
			@param[out]	outTotMsgsEnq		Receives the total number of messages ever enqueued by my caption encoder.
			@param[out]	outTotBytesEnq		Receives the total number of bytes ever enqueued by my caption encoder.
			@param[out]	outTotMsgsDeq		Receives the total number of messages ever dequeued by my caption encoder.
			@param[out]	outTotBytesDeq		Receives the total number of bytes ever dequeued by my caption encoder.
			@param[out]	outMaxQueDepth		Receives the encoder's highest queue depth.
			@param[out]	outDroppedFrames	Receives the dropped frame count.
		**/
		virtual void			GetStatus (size_t & outMessagesQueued, size_t & outBytesQueued,
											size_t & outTotMsgsEnq, size_t & outTotBytesEnq,
											size_t & outTotMsgsDeq, size_t & outTotBytesDeq,
											size_t & outMaxQueDepth, size_t & outDroppedFrames) const;


	//	Protected Instance Methods
	protected:
		virtual AJAStatus		SetUpBackgroundPatternBuffer (void);		///< @brief	Sets up my gray background field.
		virtual AJAStatus		SetUpOutputVideo (void);					///< @brief	Sets up everything I need to play video.
		virtual AJAStatus		RouteOutputSignal (void);					///< @brief	Sets up signal routing for playout.
		virtual void			StartPlayoutThread (void);					///< @brief	Starts my playout thread.
		virtual void			PlayoutFrames (void);						///< @brief	Repeatedly plays frames (until time to quit).
		virtual void			StartCaptionGeneratorThreads (void);		///< @brief	Starts my caption generator threads.

		/**
			@brief	This is the thread function that produces caption messages for a given caption channel.
			@param[in]		inCCChannel		Specifies the caption channel to generate captions for.
		**/
		virtual void			GenerateCaptions (const NTV2Line21Channel inCCChannel);

		/**
			@return	True if the device supports ancillary data extraction, and the driver supports the new AutoCirculate calls.
		**/
		virtual bool			DeviceAncExtractorIsAvailable (void);


	//	Protected Class Methods
	protected:
		/**
			@brief	This is the playout thread's static callback function that gets called when the playout thread runs.
					This function gets "Attached" to the playout thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the playout thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2CCPlayer instance.)
		**/
		static void	PlayThreadStatic (AJAThread * pThread, void * pContext);

		/**
			@brief	This is the caption generator thread's static callback function that gets called when the
					caption generator thread runs. This function gets "Attached" to the generator thread's AJAThread instance.
			@param[in]	pThread		A valid pointer to the caption generator thread's AJAThread instance.
			@param[in]	pContext	Context information to pass to the thread.
									(For this application, this will be set to point to the NTV2CCPlayer instance.)
		**/
		static void	CaptionGeneratorThreadStatic (AJAThread * pThread, void * pContext);

		TimecodeFormat NTV2FrameRate2TimecodeFormat(NTV2FrameRate inFrameRate);

		typedef std::vector<AJAThread>			AJAThreadList;
		typedef AJAThreadList::const_iterator	AJAThreadListConstIter;


	//	Private Member Data
	private:
		CCPlayerConfig				mConfig;				///< @brief	My configuration
		AJAThread					mPlayThread;			///< @brief	My playout (consumer) thread object
		AJAThreadList				mGeneratorThreads;		///< @brief	My caption generator threads -- one per caption channel
		AUTOCIRCULATE_STATUS		mACStatus;				///< @brief	My AutoCirculate status
		CNTV2Card					mDevice;				///< @brief	My CNTV2Card instance
		NTV2DeviceID				mDeviceID;				///< @brief	My device (model) identifier
		NTV2TaskMode				mSavedTaskMode;			///< @brief	Used to restore the previous state
		NTV2VANCMode				mVancMode;				///< @brief	VANC mode
		NTV2Standard				mVideoStandard;			///< @brief	Output video standard
		bool						mPlayerQuit;			///< @brief	Set "true" to terminate player
		bool						mCaptionGeneratorQuit;	///< @brief	Set "true" to terminate caption generator(s)
		CNTV2CaptionEncoder608Ptr	m608Encoder;			///< @brief	My CEA-608 caption encoder
		CNTV2CaptionEncoder708Ptr	m708Encoder;			///< @brief	My 708 caption encoder
		NTV2_POINTER				mVideoBuffer;			///< @brief	My video buffer
		NTV2ChannelSet				mActiveFrameStores;		///< @brief	My active FrameStores
		NTV2XptConnections			mConnections;			///< @brief	Routing connections I make

};	//	NTV2CCPlayer

#endif	//	_NTV2CCPLAYER_H
