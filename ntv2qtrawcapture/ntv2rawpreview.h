/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2rawpreview.h
	@brief		Header file for the NTV2RawPreview class that is used to prepare frames
				in raw format for display on the computer monitor.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2RAWPREVIEW_H
#define _NTV2RAWPREVIEW_H

#include "ajabase/common/circularbuffer.h"
#include "ajabase/common/types.h"
#include "ajabase/system/thread.h"
#include "ntv2enums.h"
#include "ntv2democommon.h"

class NTV2RawFrameGrabber;


class NTV2RawPreview	:	public AJAThread
{
	//	Public Instance Methods
	public:

					NTV2RawPreview (NTV2RawFrameGrabber * pParent, AJACircularBuffer <AVDataBuffer *> & circularBuffer);
	virtual			~NTV2RawPreview (void);

	protected:

	NTV2RawFrameGrabber *					mpParent;
	AJACircularBuffer <AVDataBuffer *> &	mCircularBuffer;
	uint16_t *								mpUnpackedRawBuffer;
	uint8_t  *								mpDebayeredFrame;

	virtual	bool	ThreadLoop (void);
};

#endif	//	_NTV2RAWPREVIEW_H

