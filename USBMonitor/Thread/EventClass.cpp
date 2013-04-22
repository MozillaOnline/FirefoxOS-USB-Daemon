//
// EventClass.cpp: implementation file
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
// Date: 11/02/07
// Name: Walter E. Capers
// Description: removed unnessary code identified by On Freund from Code Project
//
//
#include "StdAfx.h"
#include "Thread.h"

CEventClass::CEventClass(void)
	:m_bCreated(TRUE)
{
	memset(&m_owner,0,sizeof(ThreadId_t));
	m_event = CreateEvent(NULL,FALSE,FALSE,NULL);
	if( !m_event )
	{
		m_bCreated = FALSE;
	}
}

CEventClass::~CEventClass(void)
{
	CloseHandle(m_event);
}


/**
*
* Set
* set an event to signaled
*
**/
void
	CEventClass::Set()
{
	SetEvent(m_event);
}

/**
*
* Wait
* wait for an event -- wait for an event object
* to be set to signaled.  must be paired with a
* call to reset within the same thread.
*
**/
BOOL
	CEventClass::Wait()
{

	try
	{
		ThreadId_t id = CThread::ThreadId();
		if( CThread::ThreadIdsEqual(&id,&m_owner) )
		{
			throw "invalid Wait call, Wait can not be called more than once\n"
				"without a corresponding call to Reset!\n";
		}
		ThreadId_t zero;
		memset(&zero,0,sizeof(ThreadId_t));

		if( memcmp(&zero,&m_owner,sizeof(ThreadId_t)) != 0 )
		{
			throw "another thread is already waiting on this event!\n";
		}

		m_owner = CThread::ThreadId();
		if( WaitForSingleObject(m_event,INFINITE) != WAIT_OBJECT_0 )
		{
			return FALSE;
		}
	}
	catch( char *psz )
	{
		MessageBoxA(NULL,psz,"Fatal exception CEventClass::Wait",MB_ICONHAND);
		exit(-1);
	}
	return TRUE;
}

/**
*
* Reset
* reset an event flag to unsignaled
* wait must be paired with reset within the same thread.
*
**/
void
	CEventClass::Reset()
{
	try 
	{
		ThreadId_t id = CThread::ThreadId();
		if( !CThread::ThreadIdsEqual(&id,&m_owner) )
		{
			throw "unbalanced call to Reset, Reset must be called from\n"
				"the same Wait-Reset pair!\n";
		}

		memset(&m_owner,0,sizeof(ThreadId_t));
	}
	catch( char *psz )
	{
		MessageBoxA(NULL,psz,"Fatal exception CEventClass::Reset",MB_ICONHAND);
		exit(-1);
	}
}

