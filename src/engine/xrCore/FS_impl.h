#ifndef FS_IMPL_H_INCLUDED
#define FS_IMPL_H_INCLUDED

// 1: default
// 1.5: check next chunk first heuristics
// 2: vector population heuristics
// 3: dynamic map population heuristics

#define FIND_CHUNK_HEU
//#define FIND_CHUNK_STD
//#define FIND_CHUNK_VEC
//#define FIND_CHUNK_MAP

// Uncomment to log time of find_chunk (search 
//#define FIND_CHUNK_BENCHMARK_ENABLE

#ifdef FIND_CHUNK_BENCHMARK_ENABLE

struct find_chunk_counter
{
	CTimer timer;

	u64 ticks;
	u32 calls;

	find_chunk_counter()
	{
		ticks = 0;
		calls = 0;
	}

	void flush ()
	{
		float secs = (float)ticks / CPU::qpc_freq;
		Msg("find_chunk sec: %f", secs);
	}
};

#ifdef INCLUDE_FROM_ENGINE
extern __declspec(dllimport) find_chunk_counter g_find_chunk_counter;
#else //INCLUDE_FROM_ENGINE
extern __declspec(dllexport) find_chunk_counter g_find_chunk_counter;
#endif //INCLUDE_FROM_ENGINE

extern bool g_initialize_cpu_called;

struct find_chunk_auto_timer
{
	find_chunk_auto_timer() 
	{
		if ( g_initialize_cpu_called )
		{
			g_find_chunk_counter.timer.Start();
		}
	}

	~find_chunk_auto_timer() 
	{
		if ( g_initialize_cpu_called )
		{
			g_find_chunk_counter.ticks += g_find_chunk_counter.timer.GetElapsed_ticks();
		}
	}
};

#endif // FIND_CHUNK_BENCHMARK_ENABLE

#ifdef FIND_CHUNK_STD

struct IReaderBase_Test {

};

template <typename T>
IC	u32 IReaderBase<T>::find_chunk	(u32 ID, BOOL* bCompressed)	
{
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
	find_chunk_auto_timer timer;
#endif // FIND_CHUNK_BENCHMARK_ENABLE

	u32	dwSize,dwType;

	rewind();
	while (!eof()) {
		dwType = r_u32();
		dwSize = r_u32();
		if ((dwType&(~CFS_CompressMark)) == ID) {

			VERIFY	((u32)impl().tell() + dwSize <= (u32)impl().length());
			if (bCompressed) *bCompressed = dwType&CFS_CompressMark;
			return dwSize;
		}
		else	impl().advance(dwSize);
	}

	return 0;
}

#endif // #ifdef FIND_CHUNK_STD

#ifdef FIND_CHUNK_HEU

struct IReaderBase_Test {};
#pragma warning (disable:4701)

template <typename T>
IC	u32 IReaderBase<T>::find_chunk	(u32 ID, BOOL* bCompressed)	
{
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
	find_chunk_auto_timer timer;
#endif // FIND_CHUNK_BENCHMARK_ENABLE

	u32	dwSize, dwType;

	bool success = false;

	if ( m_last_pos != 0 )
	{
		impl().seek(m_last_pos);
		dwType = r_u32();
		dwSize = r_u32();

		if ( (dwType & (~CFS_CompressMark)) == ID ) 
		{
			success = true;
		}
	}

	if ( !success )
	{
		rewind();
		while ( !eof() ) 
		{
			dwType = r_u32();
			dwSize = r_u32();
			if ( (dwType & (~CFS_CompressMark)) == ID )
			{
				success = true;
				break;
			}
			else 
			{
				impl().advance(dwSize);
			}
		}

		if ( !success )
		{
			m_last_pos = 0;
			return 0;
		}
	}

	VERIFY ((u32)impl().tell() + dwSize <= (u32)impl().length());
	if (bCompressed) *bCompressed = dwType & CFS_CompressMark;

	const int dwPos = impl().tell();
	if ( dwPos + dwSize < (u32)impl().length() )
	{
		m_last_pos = dwPos + dwSize;
	}
	else
	{
		m_last_pos = 0;
	}

	return dwSize;
}

#pragma warning (default:4701)

#endif // #ifdef FIND_CHUNK_HEU

#ifdef FIND_CHUNK_VEC

#include "../xrServerEntities/associative_vector.h"

struct IReaderBase_Test {

	typedef associative_vector<u32, u32> id2pos_container;
	id2pos_container id2pos;
};

template <typename T>
IC	u32 IReaderBase<T>::find_chunk	(u32 ID, BOOL* bCompressed)	
{
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
	find_chunk_auto_timer timer;
#endif // FIND_CHUNK_BENCHMARK_ENABLE

	u32	dwSize, dwType;
	
	if ( !m_test )
	{
		m_test	= xr_new<IReaderBase_Test>();

		rewind();
		int num_chunks = 0;
		while (!eof())
		{
			r_u32();
			impl().advance(r_u32());
			++num_chunks;
		}

		((std::vector< std::pair<u32, u32> >*)&m_test->id2pos)->reserve(num_chunks);

		rewind();
		while ( !eof() )
		{
			u32 dwPos = impl().tell();

			dwType	  = r_u32();
			dwSize	  = r_u32();
	
			u32 dwId  = dwType & (~CFS_CompressMark);
			VERIFY	((u32)impl().tell() + dwSize <= (u32)impl().length());

			m_test->id2pos.insert( IReaderBase_Test::id2pos_container::value_type(dwId, dwPos) );

			impl().advance(dwSize);
		}
	}

	IReaderBase_Test::id2pos_container::iterator it = m_test->id2pos.find(ID);
	if ( it != m_test->id2pos.end() )
	{
		impl().seek(it->second);
		dwType = r_u32();
		dwSize = r_u32();

		VERIFY ( (dwType&(~CFS_CompressMark)) == ID );

		if ( bCompressed ) *bCompressed = dwType & CFS_CompressMark;
		return dwSize;
	}

	return 0;
}

#endif // #ifdef FIND_CHUNK_VEC

#ifdef FIND_CHUNK_MAP

#include "../xrServerEntities/associative_vector.h"

struct IReaderBase_Test {

	typedef xr_hash_map<u32, u32> id2pos_container;

	id2pos_container			  id2pos;
	u32							  last_pos;
};

template <typename T>
IC	u32 IReaderBase<T>::find_chunk	(u32 ID, BOOL* bCompressed)	
{
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
	find_chunk_auto_timer timer;
#endif // FIND_CHUNK_BENCHMARK_ENABLE

	u32	dwSize, dwType;

	if ( !m_test )
	{
		m_test			 = xr_new<IReaderBase_Test>();
		m_test->last_pos = 0;
	}

	IReaderBase_Test::id2pos_container::iterator it = m_test->id2pos.find(ID);
	if ( it != m_test->id2pos.end() )
	{
		impl().seek(it->second);
		dwType = r_u32();
		dwSize = r_u32();

		VERIFY ( (dwType&(~CFS_CompressMark)) == ID );

		if ( bCompressed ) *bCompressed = dwType & CFS_CompressMark;
		return dwSize;
	}

	impl().seek(m_test->last_pos);
	while ( !eof() )
	{
		u32 dwPos = impl().tell();

		dwType = r_u32();
		dwSize = r_u32();

		VERIFY((u32)impl().tell() + dwSize <= (u32)impl().length());

		u32 dwId = dwType & (~CFS_CompressMark);

		m_test->id2pos.insert( IReaderBase_Test::id2pos_container::value_type(dwId, dwPos) );

		if ( dwId == ID ) 
		{
			if (bCompressed) *bCompressed = dwType&CFS_CompressMark;

			m_test->last_pos = impl().tell() + dwSize;
			return dwSize;
		}
		else
		{
			impl().advance(dwSize);
		}
	}

	m_test->last_pos = impl().tell();
	return 0;
}

#endif // #ifdef FIND_CHUNK_MAP

#endif // #ifndef FS_IMPL_H_INCLUDED