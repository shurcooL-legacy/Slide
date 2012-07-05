#ifndef _SYSTEM
#define _SYSTEM

class System
{
public:
	BOOL isWow64();

private:
	typedef BOOL (WINAPI * LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
};

#endif
