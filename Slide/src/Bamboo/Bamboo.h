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
#ifndef _BAMBOO
#define _BAMBOO

#include "Finger.h"
#include "System.h"

class Bamboo {
public:
	Bamboo();
	void update();
	bool findFingers();
	int byteToInt(byte b[], int offset);

	Finger finger1, finger2;
	bool fingersFound;
	int width, height;

private:
	void processFinger(Finger &finger);
	void enableDebugPriv();
	HANDLE getHandler();
	HANDLE getHandler2();
	HANDLE hProcess;
	System* system;
	BOOL isx64;
};

#endif
