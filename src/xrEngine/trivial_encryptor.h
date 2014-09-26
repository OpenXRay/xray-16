#ifndef TRIVIAL_ENCRYPTOR_H
#define TRIVIAL_ENCRYPTOR_H

// before including this file
// you should define at least one of the following macro:
// #define TRIVIAL_ENCRYPTOR_ENCODER
// #define TRIVIAL_ENCRYPTOR_DECODER

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

//#define trivial_encryptor	temp_stuff

#define RUSSIAN_BUILD

class trivial_encryptor {
private:
	typedef u8					type;
	typedef void*				pvoid;
	typedef const void*			pcvoid;

private:
	enum						{alphabet_size = u32(1 << (8*sizeof(type)))};

private:
	class random32 {
	private:
		u32				m_seed;

	public:
		IC	void		seed	(const u32 &seed)
		{
			m_seed		= seed;
		}

		IC	u32			random	(const u32 &range)
		{
			m_seed		= 0x08088405*m_seed + 1;
			return		(u32(u64(m_seed)*u64(range) >> 32));
		}
	};

public:
	static	u32					m_table_iterations;
	static	u32					m_table_seed;
	static	u32					m_encrypt_seed;

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
#	ifdef TRIVIAL_ENCRYPTOR_DECODER
		private: static bool	m_initialized;
#	endif // TRIVIAL_ENCRYPTOR_DECODER
#endif // TRIVIAL_ENCRYPTOR_ENCODER

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
	private: static type		m_alphabet[alphabet_size];
#endif // TRIVIAL_ENCRYPTOR_ENCODER

#ifdef TRIVIAL_ENCRYPTOR_DECODER
	private: static type		m_alphabet_back[alphabet_size];
#endif // TRIVIAL_ENCRYPTOR_DECODER

private:
	IC	static void	initialize		()
	{
	#ifndef TRIVIAL_ENCRYPTOR_ENCODER
		type					*m_alphabet = (type*)_alloca(sizeof(type)*alphabet_size);
	#endif // TRIVIAL_ENCRYPTOR_ENCODER

		for (u32 i=0; i<alphabet_size; ++i)
			m_alphabet[i]		= (type)i;

		random32				temp;
		temp.seed				(m_table_seed);
		for (u32 i=0; i<m_table_iterations; ++i) {
			u32					j = temp.random(alphabet_size);
			u32					k = temp.random(alphabet_size);
			while (j == k)
				k				= temp.random(alphabet_size);

			std::swap			(m_alphabet[j],m_alphabet[k]);
		}

	#ifdef TRIVIAL_ENCRYPTOR_DECODER
		for (u32 i=0; i<alphabet_size; ++i)
			m_alphabet_back[m_alphabet[i]]	= (type)i;
	#endif // TRIVIAL_ENCRYPTOR_DECODER
	}

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
	public:	IC	static void	encode	(pcvoid source, const u32 &source_size, pvoid destination)
	{
#	ifndef TRIVIAL_ENCRYPTOR_DECODER
		static bool m_initialized	= false;
#	endif // TRIVIAL_ENCRYPTOR_DECODER
		if (!m_initialized) {
			initialize			();
			m_initialized		= true;
		}

		random32				temp;
		temp.seed				(m_encrypt_seed);
		const u8				*I = (const u8*)source;
		const u8				*E = (const u8*)source + source_size;
		u8						*J = (u8*)destination;
		for ( ; I != E; ++I, ++J)
			*J					= m_alphabet[*I] ^ type(temp.random(256) & 0xff);
	}
#endif // TRIVIAL_ENCRYPTOR_ENCODER

#ifdef TRIVIAL_ENCRYPTOR_DECODER
	public:	IC	static void	decode	(pcvoid source, const u32 &source_size, pvoid destination)
	{
#	ifndef TRIVIAL_ENCRYPTOR_ENCODER
		static bool m_initialized	= false;
#	endif // TRIVIAL_ENCRYPTOR_ENCODER
		if (!m_initialized) {
			initialize			();
			m_initialized		= true;
		}

		random32				temp;
		temp.seed				(m_encrypt_seed);
		const u8				*I = (const u8*)source;
		const u8				*E = (const u8*)source + source_size;
		u8						*J = (u8*)destination;
		for ( ; I != E; ++I, ++J)
			*J					= m_alphabet_back[(*I) ^ type(temp.random(256) & 0xff)];
	}
#endif // TRIVIAL_ENCRYPTOR_DECODER
};

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
#	ifdef TRIVIAL_ENCRYPTOR_DECODER
		bool trivial_encryptor::m_initialized	= false;
#	endif // TRIVIAL_ENCRYPTOR_DECODER
#endif // TRIVIAL_ENCRYPTOR_ENCODER

#ifdef RUSSIAN_BUILD
	u32	trivial_encryptor::m_table_iterations	= 4*1024;
	u32	trivial_encryptor::m_table_seed			= 7071984;
	u32	trivial_encryptor::m_encrypt_seed		= 24031979;
#else // RUSSIAN_BUILD
	u32	trivial_encryptor::m_table_iterations	= 3*1024;
	u32	trivial_encryptor::m_table_seed			= 24081978;
	u32	trivial_encryptor::m_encrypt_seed		= 20041983;
#endif // RUSSIAN_BUILD

#ifdef TRIVIAL_ENCRYPTOR_ENCODER
	trivial_encryptor::type	trivial_encryptor::m_alphabet[trivial_encryptor::alphabet_size];
#endif // TRIVIAL_ENCRYPTOR_ENCODER

#ifdef TRIVIAL_ENCRYPTOR_DECODER
	trivial_encryptor::type	trivial_encryptor::m_alphabet_back[trivial_encryptor::alphabet_size];
#endif // TRIVIAL_ENCRYPTOR_DECODER

#endif // TRIVIAL_ENCRYPTOR_H