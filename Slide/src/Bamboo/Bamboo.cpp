/***********************************************************************
 * Bamboo-TUIO Copyright (c) 2010, Bartosz Zawislak All rights reserved.
 *  
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/
#pragma comment(lib, "Psapi")

#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

//#include "Bamboo.h"
#include "../Globals.h"

Bamboo::Bamboo()
{
	system = new System();
	isx64 = system->isWow64();
	delete system;

	finger1.fingerId = 0;
	finger2.fingerId = 1;
	finger1.fingerDown = false;
	finger2.fingerDown = false;
	finger1.prevX = finger1.prevY = finger2.prevX = finger2.prevY = 0;
	fingersFound = false;

	printf("Bamboo - TUIO\n");
	printf("%s system detected\n", (isx64 ? "x64" : "x86"));
	printf("Opening WTouchUser.exe......");
	hProcess = getHandler();
	if(hProcess != NULL) {
		printf(" done!\n");
		fingersFound = findFingers();
	}
	else {
		printf(" failed!\n");
	}
	width = 1;//480;
	height = 1;//320;
}

void Bamboo::update()
{
	processFinger(finger1);
	processFinger(finger2);
}

void Bamboo::processFinger(Finger &finger)
{
#ifdef SLIDE_USE_BAMBOO
	ReadProcessMemory(hProcess, (LPVOID)finger.addressX, &finger.x, 4, NULL);
	ReadProcessMemory(hProcess, (LPVOID)finger.addressY, &finger.y, 4, NULL);

	if (finger.x != 0 || finger.y != 0)
	{
		//static_cast<double>(finger.x) / width, static_cast<double>(finger.y) / height
		const int Deadzone = 1;
		if ((abs(finger.x - finger.prevX) > Deadzone || abs(finger.y - finger.prevY) > Deadzone) || !finger.fingerDown)
		{
			if (finger.fingerDown)
			{
				MyControlMode.ProcessTouchMove(finger.fingerId, static_cast<double>(finger.x - finger.prevX) / width, static_cast<double>(finger.y - finger.prevY) / height);
			}

			printf("ofNotifyEvent(fingerMoved [%d, %d], finger %d);\n", finger.x, finger.y, finger.fingerId);
			finger.prevX = finger.x;
			finger.prevY = finger.y;
			MyControlMode.ProcessTouchPos(finger.fingerId, static_cast<double>(finger.x) / width, static_cast<double>(finger.y) / height);
		}

		if (!finger.fingerDown)
		{
			finger.fingerDown = true;
			printf("ofNotifyEvent(fingerDown [%d, %d], finger %d);\n", finger.x, finger.y, finger.fingerId);
			MyControlMode.ProcessTouchButton(finger.fingerId, 1);
		}
	}
	else if (finger.fingerDown)
	{
		finger.fingerDown = false;
		printf("ofNotifyEvent(fingerUp [prev %d, %d], finger %d);\n", finger.prevX, finger.prevY, finger.fingerId);
		MyControlMode.ProcessTouchButton(finger.fingerId, 0);
	}
#endif // SLIDE_USE_BAMBOO
}

bool Bamboo::findFingers() {
	printf("Reading process' memory, please wait...\n");

	bool finger1Found = false, finger2Found = false;
        
	int startAddress = 0x00000000;
	int endAddress =   0x03000000;
	int memorySize = endAddress - startAddress;

	const int ReadStackSize = 512;
	int arraysDifference = sizeof(int) - 1;

	if (memorySize >= ReadStackSize)
	{
		int loopsCount = memorySize / ReadStackSize;
		int currentAddress = startAddress;

		byte * data = new byte[ReadStackSize];
		int prevAddress = 0;
                
		for (int i = 0; i < loopsCount; i++) {
			ReadProcessMemory(hProcess, (LPVOID)currentAddress, data, ReadStackSize, NULL);
			for (int j = 0; j < ReadStackSize - arraysDifference; j++) {
				if(byteToInt(data, j) == (isx64 ? 0x405056f0 : 0x0083fd9c)) {
					int current = currentAddress + j;
					if(current - prevAddress == (isx64 ? 80 : 72)) {
						finger1.addressX = prevAddress + (isx64 ? 8 : 4);
						finger1.addressY = prevAddress + (isx64 ? 12 : 8);
						finger2.addressX = current + (isx64 ? 8 : 4);
						finger2.addressY = current + (isx64 ? 12 : 8);
						printf("Finger1 found @ [0x%x, 0x%x]\n", finger1.addressX, finger1.addressY);
						printf("Finger2 found @ [0x%x, 0x%x]\n", finger2.addressX, finger2.addressY);
						printf("Ready!\n");
						return true;
					}
					prevAddress = current;
				}
			}
			currentAddress += ReadStackSize - arraysDifference;
		}
	}
	printf("Can't find fingers' coordinates in memory!\n");
	return false;
}

int Bamboo::byteToInt(byte b[], int offset = 0)
{
	int val = 0;
	val |= b[offset + 3] & 0xFF;
	val <<= 8;
	val |= b[offset + 2] & 0xFF;
	val <<= 8;
	val |= b[offset + 1] & 0xFF;
	val <<= 8;
	val |= b[offset + 0] & 0xFF;

	return val;
}

void Bamboo::enableDebugPriv()
{
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

	OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );

	LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luid );

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hToken, false, &tkp, sizeof( tkp ), NULL, NULL);

	CloseHandle(hToken); 
}

/**
 * Based on kitchen's code @ http://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c
 * Works with Windows Vista and Seven
 */
HANDLE Bamboo::getHandler()
{
	PROCESSENTRY32 entry;
	entry.dwFlags = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	HANDLE hProcess = NULL;

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp( entry.szExeFile, "WTouchUser.exe" ) == 0)
			{
				enableDebugPriv();
				hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
			}
		}
	}
	CloseHandle(snapshot);
        
	if (hProcess != NULL) {
		return hProcess;
	} else {
		return getHandler2();
	}
}

/**
 * Based on http://msdn.microsoft.com/en-us/library/ms682623(VS.85).aspx
 * Works with Windows XP an Vista
 */
HANDLE Bamboo::getHandler2() {
		// Get the list of process identifiers.

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return NULL;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

		for ( i = 0; i < cProcesses; i++ ) {
				if( aProcesses[i] != 0 ) {
			TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

						// Get a handle to the process.

						HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
																					PROCESS_VM_READ,
																					FALSE, aProcesses[i] );

						// Get the process name.

						if (NULL != hProcess ) {
								HMODULE hMod;
								DWORD cbNeeded;

								if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) ) {
										GetModuleBaseName( hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
								}
						}
                        
						if(_stricmp( szProcessName, "WTouchUser.exe" ) == 0 ) {
								return hProcess;
						}

						CloseHandle( hProcess );
				}
		}

		return NULL;
}
