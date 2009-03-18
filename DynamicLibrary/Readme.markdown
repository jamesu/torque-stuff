# DynamicLibrary

This code implements a simple, yet useful abstration layer around a platform's (currently Win32 & mac) dynamic library loading code.

e.g.

    DynamicLibrary myLibrary("evil.dll", true);
    
    // Grab evilFunc
    void (__stdcall *evilFunc)();
		evilFunc = (void (__stdcall  *)())myLibrary.getProcAddress("evilFunc");
		
		// Call evilFunc
		if (evilFunc) evilFunc();
