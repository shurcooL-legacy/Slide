// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 5.0.0 (2010/01/01)

#ifndef WM5EDGEKEY_H
#define WM5EDGEKEY_H

#include "Wm5MathematicsLIB.h"

namespace Wm5
{

class EdgeKey
{
public:
    EdgeKey (int v0 = -1, int v1 = -1);

    bool operator< (const EdgeKey& key) const;
    operator size_t () const;

    int V[2];
};

#include "Wm5EdgeKey.inl"

}

#endif
