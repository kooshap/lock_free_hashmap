#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include "lock_free_hashtable.h"

inline static uint32_t int_hash(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

lock_free_hashtable::lock_free_hashtable(uint32_t size)
{
	// Initialize cells
	assert((size & (size - 1)) == 0);   // Must be a power of 2
	table_size = size;
	items = new Item[size];
	clear();
}

lock_free_hashtable::~lock_free_hashtable() 
{
	delete[] items;
}


void lock_free_hashtable::set_item(uint32_t key, uint32_t value)
{
	assert(key != 0);
	assert(value != 0);

	for (uint32_t idx = int_hash(key);; ++idx)
	{
		idx &= table_size - 1;
		uint32_t probed_key = items[idx].key.load(std::memory_order_relaxed);
		if (probed_key != key)
		{
			if (probed_key != 0)
				continue;           // Usually, it contains another key. Keep probing.
			uint32_t prev_key;
			bool exchanged=items[idx].key.compare_exchange_strong(probed_key,key);
			if (!exchanged)	
				continue;       // Another thread just stole it from underneath us.
		}
		items[idx].value.store(value);
		return;
	}
}

uint32_t lock_free_hashtable::get_item(uint32_t key)
{
	assert(key != 0);

	for (uint32_t idx = int_hash(key);; idx++)
	{
		idx &= table_size - 1;

		uint32_t probed_key = items[idx].key.load(std::memory_order_relaxed);
		if (probed_key == key)
			return items[idx].value.load(std::memory_order_relaxed);
		if (probed_key == 0)
			return 0;          
	}
}

void lock_free_hashtable::delete_item(uint32_t key)
{
	assert(key != 0);

	uint32_t i=int_hash(key);
	i &= table_size - 1;
	uint32_t loaded_key=items[i].key.load(std::memory_order_relaxed);
	if (!loaded_key)
		return;	// Key is not in the table.

	uint32_t j=i+1;

	while (1) {
		if (!items[i].key.compare_exchange_strong(loaded_key,0,std::memory_order_relaxed,std::memory_order_relaxed))
			return; // Abort if another thread deleted it before us.
		for (;;++j) {
			j &= table_size - 1;
			loaded_key=items[j].key.load(std::memory_order_relaxed);
			if (!loaded_key)
				return; // No more occupied slots to move.	
			uint32_t k=int_hash(loaded_key)&(table_size-1); // Re-hash the key stored in slot j
			if ( (i<=j) ? ((i<k)&&(k<=j)) : ((i<k)||(k<=j)) ) // If k is between i and j (cyclically)
				continue;
			break;
		}
		// k is before i, move item j to slot i
		uint32_t zero=0;
		if (!items[i].key.compare_exchange_strong(zero,loaded_key,std::memory_order_relaxed,std::memory_order_relaxed))
			return; // Abort if another thread just wrote in the deleted cell. 
		items[i].value.store(items[j].value.load(std::memory_order_relaxed)); // Store the value if moving the key succeeded
		i=j; // Do the same procedure for the emptied slot.
		loaded_key=items[i].key.load(std::memory_order_relaxed);
	}
}

void lock_free_hashtable::clear()
{
	memset(items, 0, sizeof(Item) * table_size);
}

void lock_free_hashtable::print_table()
{
	for (int i=0;i<table_size;i++)
		printf("%u|",items[i].key.load(std::memory_order_relaxed));
	printf("\n");
}
