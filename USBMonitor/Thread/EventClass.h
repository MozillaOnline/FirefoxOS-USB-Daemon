//
// EventClass.h: header file
//
// Copyright (C) Walter E. Capers.  All rights reserved
//
// This source is free to use as you like.  If you make
// any changes please keep me in the loop.  Email them to
// walt.capers@comcast.net.
//
// PURPOSE:
//
//  To implement event signals as a C++ object
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

class CEventClass
{
private:
	ThreadId_t m_owner;
	HANDLE m_event;
public:
	BOOL m_bCreated;
	void Set();
	BOOL Wait();
	void Reset();
	CEventClass(void);
	virtual ~CEventClass(void);
};