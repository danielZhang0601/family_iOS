
#include "util/hash.h"


uint32 hash_dx(const char *p_name, int len)
{
	//copy from linux-3.18.3/fs/ext4/hash.c
	uint32 hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const signed char *scp = (const signed char *) p_name;

	while (len--) 
	{
		hash = hash1 + (hash0 ^ (((int) *scp++) * 7152373));

		if (hash & 0x80000000)
		{
				hash -= 0x7fffffff;
		}
		hash1 = hash0;
		hash0 = hash;
	}
	return (hash0 << 1);
}


// RS Hash Function
uint32 hash_rs(const char *p_str, int len)
{
	uint32 b    = 378551;
	uint32 a    = 63689;
	uint32 hash = 0;
	int i    = 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = hash * a + (*p_str);
		a    = a * b;
	}

	return hash;
}

// JS Hash Function
uint32 hash_js(const char *p_str, int len)
{
	uint32 hash = 1315423911;
	int i    = 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash ^= ((hash << 5) + (*p_str) + (hash >> 2));
	}

	return hash;
}

// P. J. Weinberger Hash Function
uint32 hash_pjw(const char *p_str, int len)
{
	const uint32 bitsInUnsignedInt = (uint32)(sizeof(uint32) * 8);
	const uint32 threeQuarters     = (uint32)((bitsInUnsignedInt  * 3) / 4);
	const uint32 oneEighth         = (uint32)(bitsInUnsignedInt / 8);
	const uint32 highBits          = (uint32)(0xFFFFFFFF) << (bitsInUnsignedInt - oneEighth);
	uint32 hash				= 0;
	uint32 test				= 0;
	int i					= 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = (hash << oneEighth) + (*p_str);

		if((test = hash & highBits)  != 0)
		{
			hash = (( hash ^ (test >> threeQuarters)) & (~highBits));
		}
	}

	return hash;
}

// ELF Hash Function
uint32 hash_elf(const char *p_str, int len)
{
	uint32 hash = 0;
	uint32 x	= 0;
	int i		= 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = (hash << 4) + (*p_str);
		if((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
		}
		hash &= ~x;
	}

	return hash;
}

// BKDR Hash Function
uint32 hash_bkdr(const char *p_str, int len)
{
	uint32 seed = 131; /* 31 131 1313 13131 131313 etc.. */
	uint32 hash = 0;
	int i		= 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = (hash * seed) + (*p_str);
	}

	return hash;
}

// SDBM Hash Function
uint32 hash_sdbm(const char *p_str, int len)
{
	uint32 hash = 0;
	int i    = 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = (*p_str) + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

// DJB Hash Function
uint32 hash_djb(const char *p_str, int len)
{
	uint32 hash = 5381;
	int i    = 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = ((hash << 5) + hash) + (*p_str);
	}

	return hash;
}

// DEK Hash Function
uint32 hash_dek(const char *p_str, int len)
{
	uint32 hash = len;
	int i		= 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash = ((hash << 5) ^ (hash >> 27)) ^ (*p_str);
	}
	return hash;
}

// BP Hash Function
uint32 hash_bp(const char *p_str, int len)
{
	uint32 hash = 0;
	int i    = 0;
	for(i = 0; i < len; p_str++, i++)
	{
		hash = hash << 7 ^ (*p_str);
	}

	return hash;
}

// FNV Hash Function
uint32 hash_fnv(const char *p_str, int len)
{
	const uint32 fnv_prime = 0x811C9DC5;
	uint32 hash		= 0;
	int i			= 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash *= fnv_prime;
		hash ^= (*p_str);
	}

	return hash;
}

// AP Hash Function
uint32 hash_ap(const char *p_str, int len)
{
	uint32 hash = 0xAAAAAAAA;
	int i		= 0;

	for(i = 0; i < len; p_str++, i++)
	{
		hash ^= ((i & 1) == 0) ? ((hash <<  7) ^ (*p_str) * (hash >> 3)) :
			(~((hash << 11) + ((*p_str) ^ (hash >> 5))));
	}

	return hash;
}

//end
