//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/filterState.h"

class Delta16State : public FilterState
{
	private:
	U8 *mDataIn;
	U32 mDataInSize;
	U8 *mDataOut;
	U32 mDataOutSize;

	static Delta16State mMyself;
	public:

	virtual FilterState *init(U32 flags) {return new Delta16State(flags);}

	Delta16State()
	{
		static const char * const sn = "Delta16";
		static const char * const ln = "Process with Delta16";
		mTag = PROCESS_DELTA16;
		mShortName = (char*)sn;
		mInfoString = (char*)ln;
		registerHandler(this);
	}

	Delta16State(U32 flags)
	{
		mDataInSize = 0;
		mDataOutSize = 0;
		mDataIn = 0;
		mDataOut = 0;
	}

	~Delta16State()
	{
		// Nothing to do here
	}

	void reset()
	{
		mDataInSize = 0;
		mDataOutSize = 0;
		mDataIn = 0;
		mDataOut = 0;
	}

	U32 dataIn()
	{
		return mDataInSize;
	}

	void dataIn(U8 *buff, U32 size)
	{
		mDataIn = buff;
		mDataInSize = size;
	}

	U32 dataOut()
	{
		return mDataOutSize;
	}

	void dataOut(U8 *buff, U32 size)
	{
		mDataOut = buff;
		mDataOutSize = size;
	}

	bool process()
	{
		if (mDataInSize == 0)
			return false;

		U32 read = mDataInSize >= mDataOutSize ? mDataOutSize : mDataInSize;

		S16 *din = (S16*)mDataIn;
		S16 *dout = (S16*)mDataOut;
		U32 process = read / 2;
		if (process * 2 < read)
		{
			for (U32 i = process * 2; i < read; i++)
			*(((U8*)dout) + i) = *(((U8*)din) + i);
		}
		dout[0] = din[0];
		for (U32 i = 1; i < process; i++)
			dout[i] = din[i] - din[i - 1];

		// Less to read, less left in output
		mDataInSize -= read;
		mDataIn += read;
		mDataOut += read;
		mDataOutSize -= read;
		return true;
	}

	bool reverseProcess()
	{
		if (mDataInSize == 0) return false;
		U32 read = mDataInSize >= mDataOutSize ? mDataOutSize : mDataInSize;

		S16 *din = (S16*)mDataIn;
		S16 *dout = (S16*)mDataOut;
		U32 process = read / 2;
		if (process * 2 < read)
		{
			for (U32 i = process * 2; i < read; i++)
			*(((U8*)dout) + i) = *(((U8*)din) + i);
		}
		dout[0] = din[0];
		for (U32 i = 1; i < process; i++)
			dout[i] = din[i] + dout[i - 1];

		// Less to read, less left in output
		mDataInSize -= read;
		mDataIn += read;
		mDataOut += read;
		mDataOutSize -= read;
		return true;
	}

};

Delta16State Delta16State::mMyself;
