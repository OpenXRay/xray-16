#ifndef SoundRender_CacheH
#define SoundRender_CacheH
#pragma once

// --- just thoughts ---
// 1. LRU scheme
// 2. O(1) constant time access
// 3. O(1) constant time LRU-find (deque-like?)
// 4. fixed-count of blocks will allow efficient(cyclic) implementation of deque
// 5. allow FAT-like formatting for sources
// 7. bi-directional cache availability tracking
// 9. "touch" protocol
// 10. in case of cache-hit we have to move line to mark it used -> list

struct	cache_line;
struct	cache_ptr;

//////////////////////////////////////////////////////////////////////////
struct	cache_line						// internal, LRU queue
{
	cache_line*				prev;
	cache_line*				next;
	void*					data;		// pre-formatted
	u16*					loopback;	// dual-connectivity
	u16						id;			// need this for dual-connectivity
};
//////////////////////////////////////////////////////////////////////////
struct	cache_cat						// cache allocation table
{
	u16*					table;		// page-table
	u32						size;		// in pages
};
#define CAT_FREE			0xffff
//////////////////////////////////////////////////////////////////////////
class	CSoundRender_Cache
{
	u8*						data;		// just memory
	cache_line*				c_storage;	// just memory
	cache_line*				c_begin;	// >>> 
	cache_line*				c_end;		// <<<
	u32						_total;		// bytes total (heap)
	u32						_line;		// line size (bytes)
	u32						_count;		// number of lines
public:
	u32						_stat_hit;
	u32						_stat_miss;
private:
	void					move2top	(cache_line* line);					// move one line to TOP-priority
	void					disconnect	();									// disconnect from CATs
	void					format		();									// format structure (like filesystem)
public:
	BOOL					request		(cache_cat& cat, u32 id);			// TRUE=need to fill, FALSE=cached info avail
	void					purge		();									// discard all contents of cache

	void*					get_dataptr	(cache_cat& cat, u32 id)			{ id%=cat.size; return c_storage[cat.table[id]].data;			} //.
	u32						get_linesize()									{ return _line;													}

	void					cat_create	(cache_cat& cat, u32 bytes);
	void					cat_destroy	(cache_cat& cat);

	void					initialize	(u32 _total_kb_approx, u32 bytes_per_line);
	void					destroy		();

	void					stats_clear	()
	{
		_stat_hit			= 0;
		_stat_miss			= 0;
	}

	CSoundRender_Cache		();
	~CSoundRender_Cache		();
};
#endif
