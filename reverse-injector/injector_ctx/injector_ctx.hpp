#pragma once
#include "../mem_ctx/mem_ctx.hpp"

namespace nasa
{
	class injector_ctx
	{
	public:
		explicit injector_ctx(nasa::mem_ctx* map_into, nasa::mem_ctx* map_from);
		~injector_ctx();

		std::uintptr_t translate(std::uintptr_t) const;
		bool init() const;
	private:
		// std::uint8_t is 2^8 = 256 which is the same amount
		// of possible usermode pml4e's...
		//
		// also this is "real pml4e index" ---> "inserted pml4e index"
		mutable std::map<std::uint8_t, std::uint8_t> pml4_index_map;
		nasa::mem_ctx* map_into, *map_from;
	};
}