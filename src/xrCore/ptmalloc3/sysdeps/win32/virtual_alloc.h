#ifndef VIRTUAL_ALLOC_H_INCLUDED
#define VIRTUAL_ALLOC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus

	typedef void const* out_of_memory_handler_parameter_type;
	typedef void (__stdcall *out_of_memory_handler_type)	(void*, out_of_memory_handler_parameter_type, int);

	struct virtual_alloc_region {
		struct virtual_alloc_region*			previous_free_region;
		struct virtual_alloc_region*			next_free_region;
		size_t									size;
	};

	struct virtual_alloc_arena {
		struct virtual_alloc_region*			first_free_region;
		char const*								arena_id;
		char*									start_pointer;
		out_of_memory_handler_type				out_of_memory_handler;
		out_of_memory_handler_parameter_type	out_of_memory_handler_parameter;
		size_t									total_size;
		size_t									free_size;
		size_t									region_count;
	};

	extern struct virtual_alloc_arena			g_ptmalloc3_arena;

	void*	virtual_alloc						( struct virtual_alloc_arena* arena, size_t size );
	int		virtual_free						( struct virtual_alloc_arena* arena, void* pointer, size_t size );
	void	initialize_virtual_alloc_arena		(
				struct virtual_alloc_arena* result,
				void* buffer,
				size_t buffer_size,
				size_t granularity,
				char const* arena_id,
				out_of_memory_handler_type handler,
				out_of_memory_handler_parameter_type parameter
			);
	void	finalize_virtual_alloc_arena		( struct virtual_alloc_arena* arena );

#ifdef __cplusplus
} // extern "C"
#endif // #ifdef __cplusplus

#endif // #ifndef VIRTUAL_ALLOC_H_INCLUDED