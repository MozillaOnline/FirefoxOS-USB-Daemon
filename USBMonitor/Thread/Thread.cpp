//
// Thread.cpp: implementation file
//
// Copyright (C) Walter E. Capers.  All rights reserved
//
// This source is free to use as you like.  If you make
// any changes please keep me in the loop.  Email your changes
// to walt.capers@comcast.net.
//
// PURPOSE:
//
//  To implement threading as a C++ object
//
// NOTES:
//  This object supports two types of thread models, event driven and
//  interval driven.  Under the event driven model, a thread waits
//  in a paused state until the member function Event is called.  When
//  the Event function is called the thread wakes up and calls OnTask.
//  Under the interval driven model, the thread wakes up every
//  m_dwIdle milli-seconds and calls OnTask.
//
//  You can switch between the two models from within the same object.
//
// COMPILER NOTES:
// On Unix you must use -lpthread a -lrt
// On Windows you must specify threaded under C++ code generation
//
// REVISIONS
// =======================================================
// Date: 10.24.07        
// Name: Walter E. Capers
// Description: File creation
//
// Date: 10.24.07 11:49 am
// Name: Walter E. Capers
// Description: Added SetIdle function to allow the idle time to be altered
//              independent of the SetThreadType member function.
//              Added sleep interval to Stop function.
//
// Date: 10.25.07
// Name: Walter E. Capers
// Description: Added support for other non-windows platforms.
//
//              Added static functions: ThreadIdsEqual and ThreadId.
//
//              Added que for handling multiple events.
//
//              Created the CEventClass and CMutexClass classes to facilitate
//              platform independence.
//
// Date: 10.26.07
// Name: Walter E. Capers
// Description: Made object production ready...
//              Added more comments
//
//              Addressed various issues with threads on UNIX systems.
//                -- there was a defect in the Sleep function
//                -- there was a defect in the main thread function THKERNEL
//                   , when transitioning between thread models the CEvent::Reset
//                   function was not being called when it was necessary resulting
//                   in a lock up.
//              
//				 Transition between thread types also failed on WINDOWS since the Event
//               member function was being called from within SetThreadType.  This
//               resulted in an Event usage error.  To correct the problem m_event.Set
//               is called instead.  Also, eliminated unecessary logic.
//
//               Got rid of OnStart, OnStop, OnDestroy... Could not override with a derived
//				 class, not sure why I will come back to in a later release.
//
//				 Changed default behavior of thread.  If OnTask is not redefined in the derived
//               class the default version now expects a CTask object.  The Class for CTask 
//               is defined in thread.h.  A class must be derived from CTask to use it in
//               the default version of OnTask(LPVOID).
//
// Date: 11.01.07
// Name: Walter E. Capers
// Description: I introduced more logic and ASSERTIONS to insure the integrity of CThread objects.  
//              Both the Homogeneous and Specialized thread types can be physically set using the 
//              SetThreadType member function.  If the thread type is not set, the thread will
//              determine its type based on calls to member functions; however, this does not
//              apply to interval-based threads.  Interval-based threads must be implicitly
//              identified using the SetThreadType member function.  The new integrity tests
//              are implemented to insure usage consistency with a CThread object.   
//
//              New member functions AtCapacity and PercentCapacity were added to determine
//              if a thread is truly busy.  AtCapacity will return TRUE under one of two 
//              conditions: the thread is processing an event and its stack is full, the thread
//              is not running.  These new functions allow thread objects to be placed in arrays
//              and tasked based on their workloads.
//
//              The Event member function has been modified to verify that a thread is running
//              before posting an event.  This resolved a problem on SunOS were threads did not
//              start right away; there was a small delay of a few milliseconds.    
//
//              Error flags are automatically reset when certain member functions are called this
//              isolates error occurrences to specific call sequences.
//
//
// Date: 11.01.07
// Name: Walter E. Capers
// Description: In THKernel, changed how events are released.  Events are now released right after
//              They are recieved.
#include "StdAfx.h"
#include "Thread.h"
#include <process.h>

/**
* 
* _THKERNEL
* thread callback function used by CreateThread
*
*
**/
unsigned __stdcall
	_THKERNEL( LPVOID lpvData /* CThread Object */ 
	)
{
	CThread *pThread = (CThread *)lpvData;
	ThreadType_t lastType;
	/*
	*
	* initialization
	*
	*/

	pThread->m_mutex.Lock();
	pThread->m_state = ThreadStateWaiting;
	pThread->m_bRunning = TRUE;
	pThread->m_mutex.Unlock();

	while( TRUE )
	{
		lastType = pThread->m_type;

		if( lastType == ThreadTypeHomogeneous ||
			lastType == ThreadTypeSpecialized ||
			lastType == ThreadTypeNotDefined )
		{
			if( ! pThread->m_event.Wait()  )  // wait for a message
				break;
			pThread->m_event.Reset(); // message recieved
		}

		if( ! pThread->KernelProcess() ) 
			break;

		if( pThread->m_type == ThreadTypeIntervalDriven )
			Sleep(pThread->m_dwIdle);

	}

	pThread->m_mutex.Lock();
	pThread->m_state = ThreadStateDown;
	pThread->m_bRunning = FALSE;
	pThread->m_mutex.Unlock();

	return 0;
}

/**
*
* FromSameThread
* determines if the calling thread is the same
* as the thread assoicated with the object
*
**/
BOOL
	CThread::FromSameThread()
{
	ThreadId_t id = ThreadId();
	if( ThreadIdsEqual(&id,&m_dwId) ) return TRUE;
	return FALSE;
}

/**
*
* OnTask
* called when a thread is tasked using the Event
* member function
*
**/
BOOL 
	CThread::OnTask( LPVOID lpvData /*data passed from thread*/ 
	)
{
	ASSERT(lpvData && m_type == ThreadTypeHomogeneous);

	((CTask *)lpvData)->SetTaskStatus(TaskStatusBeingProcessed);
	BOOL bReturn = ((CTask *)lpvData)->Task();
	((CTask *)lpvData)->SetTaskStatus(TaskStatusCompleted);

	return bReturn; 
} 



/**
*
* OnTask
* overloaded implementation of OnTask that
* takes no arguments
*
**/
BOOL
	CThread::OnTask()
{
	ASSERT(m_type == ThreadTypeIntervalDriven);

	TRACE(_T("\nthread is alive\n"));

	return TRUE;
}

/**
*
* CEvent
* used to place tasks on the threads event queue
* wakes up thread.
*
**/
BOOL
	CThread::Event(CTask *pvTask /* data to be processed by thread */
	)
{
	m_mutex.Lock();

	ASSERT(m_type == ThreadTypeHomogeneous ||
		m_type == ThreadTypeNotDefined );

	try 
	{
		if( FromSameThread() )
		{
			throw "it is illegal for a thread to place an event on its own event stack!\n";
		}


		// make sure that the thread is running 
		if( !m_bRunning && m_dwObjectCondition == NO_ERRORS )
		{
			m_mutex.Unlock();
			PingThread(m_dwIdle*2); // wait two idle cycles for it to start
			m_mutex.Lock();
		}
		if( !m_bRunning ) // if it is not running return FALSE;
		{
			m_mutex.Unlock();
			return FALSE;
		}


		if( m_dwObjectCondition & ILLEGAL_USE_OF_EVENT )
			m_dwObjectCondition = m_dwObjectCondition ^ ILLEGAL_USE_OF_EVENT;
		if( m_dwObjectCondition & EVENT_AND_TYPE_DONT_MATCH)
			m_dwObjectCondition = m_dwObjectCondition ^ EVENT_AND_TYPE_DONT_MATCH;

		m_type = ThreadTypeHomogeneous;
		m_mutex.Unlock();

		pvTask->SetId(&m_dwId);
		if( ! Push((LPVOID)pvTask) )
			return FALSE;

		pvTask->SetTaskStatus(TaskStatusWaitingOnQueue);
		m_event.Set();

	}
	catch (char *psz)
	{
		MessageBoxA(NULL,psz,"Fatal exception CThread::CEvent",MB_ICONHAND);
		exit(-1);
	}
	return TRUE;
}

/**
*
* Event
* used to place tasks on the threads event queue
* wakes up thread.
*
**/
BOOL
	CThread::Event(LPVOID lpvData /* data to be processed by thread */
	)
{

	m_mutex.Lock();
	ASSERT( m_type == ThreadTypeSpecialized ||
		m_type == ThreadTypeNotDefined );
	try 
	{
		if( FromSameThread() )
		{
			throw "it is illegal for a thread to place an event on its own event stack!\n";
		}
	}
	catch (char *psz)
	{
		MessageBoxA(NULL,psz,"Fatal exception CThread::CEvent",MB_ICONHAND);
		exit(-1);
	}

	// make sure that the thread is running 
	if( !m_bRunning && m_dwObjectCondition == NO_ERRORS )
	{
		m_mutex.Unlock();
		PingThread(m_dwIdle*2); // wait two idle cycles for it to start
		m_mutex.Lock();
	}
	if( !m_bRunning ) // if it is not running return FALSE;
	{
		m_mutex.Unlock();
		return FALSE;
	}

	if( m_dwObjectCondition & ILLEGAL_USE_OF_EVENT )
		m_dwObjectCondition = m_dwObjectCondition ^ ILLEGAL_USE_OF_EVENT;
	if( m_dwObjectCondition & EVENT_AND_TYPE_DONT_MATCH)
		m_dwObjectCondition = m_dwObjectCondition ^ EVENT_AND_TYPE_DONT_MATCH;

	m_type = ThreadTypeSpecialized;

	m_mutex.Unlock();
	if( ! Push(lpvData) )
	{
		return FALSE;
	}

	m_event.Set();

	return TRUE;
}


/**
*
* SetPriority
* sets a threads run priority, see SetThreadPriority
* Note: only works for Windows family of operating systems
*
*
**/
void
	CThread::SetPriority(DWORD dwPriority)
{
	SetThreadPriority(m_thread,dwPriority);
}


/**
*
* KernelProcess
* routes thread activity
*
**/
BOOL
	CThread::KernelProcess()
{

	m_mutex.Lock();
	m_state = ThreadStateBusy;
	if( !m_bRunning )
	{
		m_state = ThreadStateShuttingDown;
		m_mutex.Unlock();
		return FALSE;
	}
	m_mutex.Unlock();

	if( !Empty() )
	{
		while( !Empty() )
		{
			Pop();
			if( !OnTask(m_lpvProcessor) )
			{
				m_mutex.Lock();
				m_lpvProcessor = NULL;
				m_state = ThreadStateShuttingDown;
				m_mutex.Unlock();
				return FALSE;
			}
		}
		m_mutex.Lock();
		m_lpvProcessor = NULL;
		m_state = ThreadStateWaiting;
	}
	else {
		if( !OnTask() )
		{
			m_mutex.Lock();
			m_state = ThreadStateShuttingDown;
			m_mutex.Unlock();
			return FALSE;
		}
		m_mutex.Lock();
		m_state = ThreadStateWaiting;
	}

	m_mutex.Unlock();

	return TRUE;
}


/**
* 
* GetEventsPending
* returns the total number of vents waiting
* in the event que
* 
**/
unsigned int
	CThread::GetEventsPending()
{
	unsigned int chEventsWaiting;

	m_mutex.Lock();
	chEventsWaiting = m_queuePos;
	m_mutex.Unlock();

	return chEventsWaiting;
}


/**
*
* CThread
* instanciates thread object and
* starts thread.
*
**/
CThread::CThread(void)
	:m_bRunning(FALSE)
	,m_thread(NULL)
	,m_dwId(0L)
	,m_state(ThreadStateDown)
	,m_dwIdle(100)
	,m_lppvQueue(NULL)
	,m_lpvProcessor(NULL)
	,m_chQueue(QUEUE_SIZE)
	,m_type(ThreadTypeNotDefined)
	,m_stackSize(DEFAULT_STACK_SIZE)
	,m_queuePos(0)
	,m_StopTimeout(30)
{

	m_dwObjectCondition = NO_ERRORS;

	m_lppvQueue = new LPVOID [QUEUE_SIZE];

	if( !m_lppvQueue ) 
	{
		m_dwObjectCondition |= MEMORY_FAULT;
		m_state = ThreadStateFault;
		return;
	}

	if( !m_mutex.m_bCreated )
	{
		TRACE(_T("mutex creation failed"));
		m_dwObjectCondition |= MUTEX_CREATION;
		m_state = ThreadStateFault;
		return;
	}


	if( !m_event.m_bCreated )
	{
		TRACE(_T("event creation failed"));
		m_dwObjectCondition |= EVENT_CREATION;
		m_state = ThreadStateFault;
		return;
	}


	Start();

}


/**
*
* PercentCapacity
* returns a floating point value identifying
* the current workload of the thread
*
**/
float
	CThread::PercentCapacity()
{
	float fValue = 0;
	m_mutex.Lock();
	fValue = (float)m_queuePos/m_chQueue;
	m_mutex.Unlock();
	return fValue;
}

/**
*
* SetQueueSize
* changes the threads queue size
*
**/
BOOL
	CThread::SetQueueSize( unsigned int ch )
{
	LPVOID * newQueue = NULL;

	m_mutex.Lock();
	ASSERT(ch > m_queuePos);

	newQueue = new LPVOID [ch];
	if(  !newQueue )
	{
		TRACE(_T("Warning CThread::SetQueueSize:\n\ta low memory, could not reallocate queue!\n"));
		m_mutex.Unlock();
		return FALSE;
	}

	for( unsigned int i=0;i<m_queuePos; i++ )
	{
		newQueue[i] = m_lppvQueue[i];
	}

	delete [] m_lppvQueue;

	m_chQueue = ch;
	m_lppvQueue = newQueue;

	m_mutex.Unlock();

	return TRUE;
}



/**
*
* Empty
* returns a value of TRUE if there are no items on the threads que
* otherwise a value of FALSE is returned.
*
**/
BOOL
	CThread::Empty()
{
	m_mutex.Lock();
	if( m_queuePos <= 0 )
	{
		m_mutex.Unlock();
		return TRUE;
	}
	m_mutex.Unlock();
	return FALSE;
}



/**
*
* Push
* place a data object in the threads que
*
**/
BOOL
	CThread::Push( LPVOID lpv )
{
	if( !lpv ) return TRUE;

	m_mutex.Lock();

	if( m_queuePos+1 >= m_chQueue ) {
		m_dwObjectCondition |= STACK_OVERFLOW;
		m_mutex.Unlock();
		return FALSE;
	}
	if( m_dwObjectCondition & STACK_EMPTY    )
		m_dwObjectCondition = m_dwObjectCondition ^ STACK_EMPTY;

	if( m_dwObjectCondition & STACK_OVERFLOW ) 
		m_dwObjectCondition = m_dwObjectCondition ^ STACK_OVERFLOW;

	m_lppvQueue[m_queuePos++] = lpv;
	if( m_queuePos+1 >= m_chQueue )
		m_dwObjectCondition |= STACK_FULL;

	m_mutex.Unlock();
	return TRUE;
}


/**
*
* Pop
* move an object from the input que to the processor
*
**/
BOOL
	CThread::Pop()
{

	m_mutex.Lock();
	if( m_queuePos-1 < 0 )
	{
		m_queuePos = 0;
		m_dwObjectCondition |= STACK_EMPTY;
		m_mutex.Unlock();
		return FALSE;
	}
	if( m_dwObjectCondition & STACK_EMPTY )
		m_dwObjectCondition = m_dwObjectCondition ^ STACK_EMPTY;
	if( m_dwObjectCondition & STACK_OVERFLOW )
		m_dwObjectCondition = m_dwObjectCondition ^ STACK_OVERFLOW;
	if( m_dwObjectCondition & STACK_FULL )
		m_dwObjectCondition = m_dwObjectCondition ^ STACK_FULL;

	m_queuePos--;
	m_lpvProcessor = m_lppvQueue[m_queuePos];
	m_mutex.Unlock();
	return TRUE;
}


/**
*
* SetThreadType
* specifies the type of threading that is to be performed.
*
* ThreadTypeEventDriven (default): an event must be physically sent
*									to the thread using the Event member
*									function.
*
* ThreadTypeIntervalDriven       : an event occurs automatically every 
*                                  dwIdle milli seconds.
*
**/
void
	CThread::SetThreadType(ThreadType_t typ,
	DWORD dwIdle)
{

	try 
	{
		if( FromSameThread() )
		{
			throw "it is illegal for a thread to change its own type!\n";
		}


		m_mutex.Lock();
		m_dwIdle = dwIdle;


		if( m_type == typ ) {
			m_mutex.Unlock();
			return;
		}
		if( m_dwObjectCondition & ILLEGAL_USE_OF_EVENT )
			m_dwObjectCondition = m_dwObjectCondition ^ ILLEGAL_USE_OF_EVENT;
		if( m_dwObjectCondition & EVENT_AND_TYPE_DONT_MATCH )
			m_dwObjectCondition = m_dwObjectCondition ^ EVENT_AND_TYPE_DONT_MATCH;

		m_type = typ;


		m_mutex.Unlock();
		m_event.Set();
	}
	catch (char *psz)
	{
		MessageBoxA(NULL,psz,"Fatal exception CThread::SetThreadType",MB_ICONHAND);
		exit(-1);
	}
}


/**
*
* Stop
* stop thread 
*
**/
BOOL
	CThread::Stop()
{
	try 
	{
		if( FromSameThread() )
		{
			throw "it is illegal for a thread to attempt to signal itself to stop!\n";
		}

		m_mutex.Lock();
		m_bRunning = FALSE;
		m_mutex.Unlock();
		m_event.Set();

		int ticks = (m_StopTimeout*1000)/100;

		for( int i=0; i<ticks; i++ )
		{
			Sleep(100);

			m_mutex.Lock();
			if( m_state == ThreadStateDown )
			{
				m_mutex.Unlock();
				return TRUE;
			}
			m_mutex.Unlock();

		} 
	}
	catch (char *psz)
	{
		MessageBoxA(NULL,psz,"Fatal exception CThread::Stop",MB_ICONHAND);
		exit(-1);
	}
	return FALSE;
}


/**
*
* SetIdle
* changes the threads idle interval
*
**/
void
	CThread::SetIdle(DWORD dwIdle)
{
	m_mutex.Lock();
	m_dwIdle = dwIdle;
	m_mutex.Unlock();
}

/**
*
* Start
* start thread
*
**/
BOOL
	CThread::Start()
{
	try 
	{
		if( FromSameThread() )
		{
			throw "it is illegal for a thread to attempt to start itself!\n";
		}


		m_mutex.Lock();
		if( m_bRunning ) 
		{
			m_mutex.Unlock();
			return TRUE;
		}

		m_mutex.Unlock();


		if( m_dwObjectCondition & THREAD_CREATION )
			m_dwObjectCondition = m_dwObjectCondition ^ THREAD_CREATION;

		if( m_thread ) CloseHandle(m_thread);
		m_thread = (HANDLE )_beginthreadex(NULL,(unsigned int)m_stackSize,_THKERNEL,(LPVOID)this,0,&m_dwId);
		if( !m_thread )
		{
			perror("thread creation failed");
			m_dwObjectCondition |= THREAD_CREATION;
			m_state = ThreadStateFault;
			return FALSE;
		}
	}
	catch (char *psz)
	{
		MessageBoxA(NULL,psz,"Fatal exception CThread::Start",MB_ICONHAND);
		exit(-1);
	}
	return TRUE;
}

/**
*
* AtCapacity
* returns TRUE if the threads queue is full, and the thread
* is busy processing an event or the thread is not running
*
**/
BOOL
	CThread::AtCapacity()
{
	m_mutex.Lock();
	if( ((m_dwObjectCondition & STACK_OVERFLOW ||
		m_dwObjectCondition & STACK_FULL ) &&
		m_state == ThreadStateBusy) || !m_bRunning)
	{
		m_mutex.Unlock();
		return TRUE;
	}
	m_mutex.Unlock();
	return FALSE;
}

/**
*
* ThreadState
* return the current state of the thread
*
**/
ThreadState_t 
	CThread::ThreadState()
{
	ThreadState_t currentState;
	m_mutex.Lock();
	currentState = m_state;
	m_mutex.Unlock();
	return currentState;
}

/**
*
* ~CThread
* destructor.  Stop should be called prior to destruction to
* allow for gracefull thread termination.
*
**/
CThread::~CThread(void)
{
	if( m_bRunning ) // gracefull termination
	{
		try 
		{
			if( !Stop() )
			{
				throw "thread failed to stop in a timely manner!\n";
			}
		}
		catch( char *psz )
		{
			MessageBoxA(NULL,psz,"Fatal exception CThread::Stop",MB_ICONHAND);
			exit(-1);
		}
	}
	CloseHandle(m_thread);

	delete [] m_lppvQueue;
}


/**
*
* PingThread
* used to determine if a thread is running
*
**/
BOOL
	CThread::PingThread(DWORD dwTimeout /* timeout in milli-seconds */
	)
{
	DWORD dwTotal = 0;

	while(TRUE)
	{
		if( dwTotal > dwTimeout && dwTimeout > 0 )
			return FALSE;
		m_mutex.Lock();
		if( m_bRunning )
		{
			m_mutex.Unlock();
			return TRUE;
		}
		dwTotal += m_dwIdle;
		m_mutex.Unlock();
		Sleep(m_dwIdle);
	}

	return FALSE;
}

/**
*
* WaitTillExit
* blocks caller until thread exits
*
**/
void
	CThread::WaitTillExit()
{

	/*
	*
	* prevent users from calling this function from within the same thread
	* of execution
	*
	*/
	try 
	{
		if( FromSameThread() )
			throw "this function can not be called from within the same thread!\n";

		if( !m_bRunning ) return;

		WaitForSingleObject(m_thread,INFINITE);
	}
	catch( char *psz )
	{
		MessageBoxA(NULL,psz,"Fatal exception CThread::WaitTillExit",MB_ICONHAND);
		exit(-1);
	}
}