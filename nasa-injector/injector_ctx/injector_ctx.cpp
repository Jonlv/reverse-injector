#include "injector_ctx.hpp"

namespace nasa
{
	injector_ctx::injector_ctx(nasa::mem_ctx* map_into, nasa::mem_ctx* map_from)
		: 
		map_into(map_into),
		map_from(map_from)
	{}

	injector_ctx::~injector_ctx()
	{
		const auto pml4 = reinterpret_cast<ppml4e>(
			this->map_into->set_page(
				this->map_into->get_dirbase()));
		
		// zero inserted pml4e's...
		for (const auto [real_idx, inserted_idx] : this->pml4_index_map)
			pml4[inserted_idx] = pml4e{ NULL };

		while (!SwitchToThread());
	}

	bool injector_ctx::init() const
	{
		const auto source_pml4 =
			reinterpret_cast<ppml4e>(
				map_from->set_page(
					map_from->get_dirbase()));

		const auto target_pml4 =
			reinterpret_cast<ppml4e>(
				map_into->set_page(
					map_into->get_dirbase()));

		std::vector<std::pair<std::uint8_t, pml4e>> present_pml4es;
		std::vector<std::uint8_t> empty_pml4es;

		// find present pml4e's in usermode, and also find empty pml4e's in usermode...
		for (auto idx = 0u; idx < 256; ++idx)
		{
			if (source_pml4[idx].present)
				present_pml4es.push_back({ idx, source_pml4[idx] });

			if (!target_pml4[idx].present)
				empty_pml4es.push_back(idx);
		}

		// pretty much never going to happen but just in case :^)
		if (present_pml4es.size() > empty_pml4es.size())
			return false;

		// setup translation table and insert pml4e's...
		for (auto idx = 0u; idx < present_pml4es.size(); ++idx)
		{
			this->pml4_index_map.insert({ present_pml4es[idx].first, empty_pml4es[idx] });
			target_pml4[empty_pml4es[idx]] = present_pml4es[idx].second;
		}
		return true;
	}

	// translate the pml4 index to the correct pml4 index...
	std::uintptr_t injector_ctx::translate(std::uintptr_t translate) const
	{
		virt_addr_t virt_addr{ reinterpret_cast<void*>(translate) };
		virt_addr.pml4_index = pml4_index_map[virt_addr.pml4_index];
		return reinterpret_cast<std::uintptr_t>(virt_addr.value);
	}
}