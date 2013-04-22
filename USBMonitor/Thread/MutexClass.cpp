//
// MutexClass.cpp: implementation file
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
#include "StdAfx.h"
#include "Thread.h"

CMutexClass::CMutexClass(void)
	:m_bCreated(TRUE)
{
	m_mutex = CreateMutex(NULL,FALSE,NULL);
	if( !m_mutex ) m_bCreated = FALSE;

	memset(&m_owner,0,sizeof(ThreadId_t));
}

CMutexClass::~CMutexClass(void)
{
	WaitForSingleObject(m_mutex,INFINITE);
	CloseHandle(m_mutex);
}

/**
*
* Lock
* the same thread can not lock the same mutex
* more than once
*
**/
void
	CMutexClass::Lock()
{
	ThreadId_t id = CThread::ThreadId();
	try {
		if(CThread::ThreadIdsEqual(&m_owner,&id) )
			throw "the same thread can not acquire a mutex twice!\n"; // the mutex is already locked by this thread
		WaitForSingleObject(m_mutex,INFINITE);
		m_owner = CThread::ThreadId();
	}
	catch( char *psz )
	{
		MessageBoxA(NULL,psz,"Fatal exception CMutexClass::Lock",MB_ICONHAND);
		exit(-1);
	}
}

/**
*
* Unlock
* releases a mutex.  only the thread that acquires
* the mutex can release it.
*
**/
void 
	CMutexClass::Unlock()
{
	ThreadId_t id = CThread::ThreadId();
	try 
	{
		if( ! CThread::ThreadIdsEqual(&id,&m_owner) )
			throw "only the thread that acquires a mutex can release it!"; 

		memset(&m_owner,0,sizeof(ThreadId_t));
		ReleaseMutex(m_mutex);
	}
	catch ( char *psz)
	{
		MessageBoxA(NULL,psz,"Fatal exception CMutexClass::Unlock",MB_ICONHAND);
		exit(-1);
	}
}
