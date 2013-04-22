//
// MutexClass.h: header file
//
// Copyright (C) Walter E. Capers.  All rights reserved
//
// This source is free to use as you like.  If you make
// any changes please keep me in the loop.  Email them to
// walt.capers@comcast.net.
//
// PURPOSE:
//
//  To implement mutexes as a C++ object
//
// REVISIONS
// =======================================================
// Date: 10.25.07        
// Name: Walter E. Capers
// Description: File creation
//
// Date:
// Name:
// Description:
//
//

#pragma once

#include "Thread.h"

class CMutexClass
{
private:
	HANDLE m_mutex;
	ThreadId_t m_owner;
public:
	BOOL m_bCreated;
	void Lock();
	void Unlock();
	CMutexClass(void);
	virtual ~CMutexClass(void);
};