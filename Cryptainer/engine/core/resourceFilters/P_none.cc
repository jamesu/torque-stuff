//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/filterState.h"

class BasicState : public FilterState
{
	private:
	U8 *mDataIn;
	U32 mDataInSize;
	U8 *mDataOut;
	U32 mDataOutSize;

	static BasicState mMyself;
	public:

	virtual FilterState *init(U32 flags) {return new BasicState(flags);}

	BasicState()
	{
		static const char * const sn = "Basic";
		static const char * const ln = "Straight write";
		mTag = PROCESS_BASIC;
		mShortName = (char*)sn;
		mInfoString = (char*)ln;
		registerHandler(this);
	}

	BasicState(U32 flags)
	{
		mDataInSize = 0;
		mDataOutSize = 0;
		mDataIn = 0;
		mDataOut = 0;
	}

	~BasicState()
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

	bool end()
	{
		// NOTE: filter will be unusable following success!
		return true;
	}

	bool process()
	{
		if ((mDataInSize == 0) || (mDataOutSize == 0))
			return false;

		U32 read = mDataInSize > mDataOutSize ? mDataOutSize : mDataInSize;
		dMemcpy(mDataOut, mDataIn, read);

		// Less to read, less left in output
		mDataInSize -= read;
		mDataIn += read;
		mDataOut += read;
		mDataOutSize -= read;
		return true;
	}

	bool reverseProcess()
	{
		// Straight copy, same as process()
		return process();
	}

};

BasicState BasicState::mMyself;

