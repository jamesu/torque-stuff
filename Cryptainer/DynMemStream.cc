DynMemStream::DynMemStream(const U32 in_blockSize,
                     const bool   in_allowRead,
                     const bool   in_allowWrite)
{
   m_blockSize = in_blockSize;
   m_instCaps = 0;
   m_currentPosition = 0;
   m_writSize = 0;
   AssertFatal(in_blockSize > 0,  "Invalid block size");
   AssertFatal(in_allowRead || in_allowWrite, "Either write or read must be allowed");

   U8 *data = (U8*)dMalloc(in_blockSize);
   cm_bufferSize = in_blockSize;
   m_pBufferBase = (void*)data;

   if (in_allowRead)
      m_instCaps |= Stream::StreamRead;
   if (in_allowWrite)
      m_instCaps |= Stream::StreamWrite;

   setStatus(Ok);
}

DynMemStream::~DynMemStream()
{
	dFree(m_pBufferBase);
}

bool DynMemStream::_write(const U32 in_numBytes, const void *in_pBuffer)
{
   AssertFatal(getStatus() != Closed, "Attempted write to a closed stream");

   if (in_numBytes == 0)
      return true;

   AssertFatal(in_pBuffer != NULL, "Invalid input buffer");

   if (hasCapability(StreamWrite) == false) {
      AssertWarn(0, "Writing is disallowed on this stream");
      setStatus(IllegalCall);
      return false;
   }

   bool success     = true;
   if ((m_currentPosition + in_numBytes) > cm_bufferSize) {
      // TODO: could be a bit more accurate...
      U32 newSize = (((m_currentPosition + in_numBytes) - cm_bufferSize) / m_blockSize)+1;
      newSize *= m_blockSize;
      m_pBufferBase = dRealloc((U8*)m_pBufferBase, newSize);
      AssertFatal(m_pBufferBase, "Failed to reallocate buffer!");
      cm_bufferSize = newSize;
   }

   // Obtain a current pointer, and do the copy
   void* pCurrent = (void*)((U8*)m_pBufferBase + m_currentPosition);
   dMemcpy(pCurrent, in_pBuffer, in_numBytes);

   // Advance the stream position
   m_currentPosition += in_numBytes;
   if (m_currentPosition > m_writSize)
      m_writSize += in_numBytes;

   if (m_currentPosition == cm_bufferSize)
   //setStatus(EOS);
   // else
      setStatus(Ok);

   return success;
}
