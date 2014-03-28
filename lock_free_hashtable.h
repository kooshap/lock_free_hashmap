#include <atomic>

class lock_free_hashtable
{
	public:
		struct Item
		{
			std::atomic<uint32_t> key;
			std::atomic<uint32_t> value;
		};

	private:    
		Item* items;
		uint32_t table_size;

	public:
		lock_free_hashtable(uint32_t size);
		~lock_free_hashtable();

		void set_item(uint32_t key, uint32_t value);
		uint32_t get_item(uint32_t key);
		void delete_item(uint32_t key);
		void clear();
		void print_table();
};
