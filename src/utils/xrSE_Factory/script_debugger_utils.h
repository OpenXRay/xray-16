#pragma once

enum xrEventWaitRes
{
	wrError,
	wrSignaled,
	wrTimeOut
};

class xr_event
{
public:
	xr_event(bool broadcast = false, bool signalled = false);
	virtual ~xr_event();
	bool			signal();
	bool			reset();
	xrEventWaitRes	wait();
	xrEventWaitRes	wait(unsigned msec);

private:
	HANDLE			m_event;

private: //hidden
	xr_event(const xr_event &);
	void operator = (const xr_event &);
};

class xr_mutex
{
public:
	xr_mutex();
	virtual ~xr_mutex();
	bool trylock();
	bool lock();
	bool unlock();

private:
	CRITICAL_SECTION	m_mutex;
private: // hidden
	xr_mutex(const xr_mutex &);
	void operator = (const xr_mutex &);
};

class xr_sync
{
public:
	xr_sync(xr_mutex &);
	virtual ~xr_sync();

private:
	xr_mutex &m_mutex;

private: // hidden
	xr_sync(const xr_sync &);
	void operator=(const xr_sync &);
};


class xr_thread
{
public:
	xr_thread();
	virtual ~xr_thread();

	virtual void run() = 0;

	bool start();
	bool kill();
	bool join();
	static bool yield();
	static void sleep(unsigned msec);
//	static pid_t getId();
	static unsigned getTickCount(); // msec

private:
	HANDLE		m_thread;

private: // hidden
	xr_thread(const xr_thread &);
	void operator = (const xr_thread &);
};

class xr_waitableThread: private xr_thread
{
public:
	xr_waitableThread();
	virtual ~xr_waitableThread();

	virtual void run_w() = 0;

	bool start();
	bool kill();
	bool join();
	bool join(unsigned msec);
	static bool yield();

private:
	virtual void run();
	xr_event m_event;

private: // hidden
	xr_waitableThread(const xr_waitableThread &);
	void operator = (const xr_waitableThread &);
};

