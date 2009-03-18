
class DynMemStream : public MemStream {
	typedef MemStream Parent;
	
	protected:
		U32 m_blockSize;	///< Chunk size for new allocations
		U32 m_writSize;	///< Bytes we have written
	
	public:
		DynMemStream(const U32  in_blockSize,
						const bool in_allowRead  = true,
						const bool in_allowWrite = true);
		~DynMemStream();

		U32 getStreamSize() {return m_writSize;}
		U8 *getData() {return (U8*)m_pBufferBase;}
	protected:
		bool _write(const U32 in_numBytes, const void* in_pBuffer);
};
