//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _H_DSBGENGAME_
#define _H_DSBGENGAME_

#ifndef _GAMEINTERFACE_H_
#include "platform/gameInterface.h"
#endif

class DMFArGame : public GameInterface
{
  public:
   S32 main(S32 argc, const char **argv);
};

#endif  // _H_DSBGENGAME_
