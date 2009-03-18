//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/filterState.h"

class Delta8State : public FilterState
{
	private:
	S8 *mDataIn;
	U32 mDataInSize;
	S8 *mDataOut;
	U32 mDataOutSize;

	static Delta8State mMyself;
	public:

	virtual FilterState *init(U32 flags) {return new Delta8State(flags);}

	Delta8State()
	{
		static const char * const sn = "Delta8";
		static const char * const ln = "Process with Delta8";
		mTag = PROCESS_DELTA8;
		mShortName = (char*)sn;
		mInfoString = (char*)ln;
		registerHandler(this);
	}

	Delta8State(U32 flags)
	{
		mDataInSize = 0;
		mDataOutSize = 0;
		mDataIn = 0;
		mDataOut = 0;
	}

	~Delta8State()
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
		mDataIn = (S8*)buff;
		mDataInSize = size;
	}

	U32 dataOut()
	{
		return mDataOutSize;
	}

	void dataOut(U8 *buff, U32 size)
	{
		mDataOut = (S8*)buff;
		mDataOutSize = size;
	}

	bool process()
	{
		if ((mDataInSize == 0) || (mDataOutSize == 0))
			return false;

		U32 read = mDataInSize > mDataOutSize ? mDataOutSize : mDataInSize;

		S8 *din = mDataIn;
		S8 *dout = mDataOut;
		dout[0] = din[0];
		for (U32 i = 1; i < read; i++)
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
		if ((mDataInSize == 0) || (mDataOutSize == 0))
			return false;

		U32 read = mDataInSize > mDataOutSize ? mDataOutSize : mDataInSize;

		S8 *din = mDataIn;
		S8 *dout = mDataOut;
		dout[0] = din[0];
		for (U32 i = 1; i < read; i++)
			dout[i] = din[i] + dout[i - 1];

		// Less to read, less left in output
		mDataInSize -= read;
		mDataIn += read;
		mDataOut += read;
		mDataOutSize -= read;
		return true;
	}

};

Delta8State Delta8State::mMyself;
