#pragma once
#include <windows.h>
#include <string_view>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include "../vdm/vdm.hpp"

namespace vdm
{
	// change this to whatever you want :^)
	constexpr std::pair<const char*, const char*> syscall_hook = { "NtShutdownSystem", "ntdll.dll" };

	inline std::atomic<bool> is_page_found = false;
	inline std::atomic<void*> syscall_address = nullptr;

	inline std::uint16_t nt_page_offset;
	inline std::uint32_t nt_rva;
	inline std::uint8_t* ntoskrnl;

	using read_phys_t = std::function<decltype(vdm::read_phys)>;
	using write_phys_t = std::function<decltype(vdm::write_phys)>;

	class vdm_ctx
	{
	public:
		vdm_ctx
		(	
			std::function<decltype(vdm::read_phys)>& read_func,
			std::function<decltype(vdm::write_phys)>& write_func
		);

		void set_read(std::function<decltype(vdm::read_phys)>& read_func);
		void set_write(std::function<decltype(vdm::write_phys)>& write_func);

		template <class T, class ... Ts>
		__forceinline std::invoke_result_t<T, Ts...> syscall(void* addr, Ts ... args) const
		{
			static const auto proc = 
				GetProcAddress(
					LoadLibraryA(syscall_hook.second),
					syscall_hook.first
				);

			static std::mutex syscall_mutex;
			syscall_mutex.lock();

			// jmp [rip+0x0]
			std::uint8_t jmp_code[] =
			{
				0xff, 0x25, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00
			};

			std::uint8_t orig_bytes[sizeof jmp_code];
			*reinterpret_cast<void**>(jmp_code + 6) = addr;
			read_phys(vdm::syscall_address.load(), orig_bytes, sizeof orig_bytes);

			// execute hook...
			write_phys(vdm::syscall_address.load(), jmp_code, sizeof jmp_code);
			auto result = reinterpret_cast<T>(proc)(args ...);
			write_phys(vdm::syscall_address.load(), orig_bytes, sizeof orig_bytes);

			syscall_mutex.unlock();
			return result;
		}

		template <class T>
		__forceinline auto rkm(std::uintptr_t addr) -> T
		{
			static const auto ntoskrnl_memcpy =
				util::get_kmodule_export("ntoskrnl.exe", "memcpy");

			T buffer;
			this->syscall<decltype(&memcpy)>(
				ntoskrnl_memcpy, &buffer, (void*)addr, sizeof T);

			return buffer;
		}

		template <class T>
		__forceinline void wkm(std::uintptr_t addr, const T& value)
		{
			static const auto ntoskrnl_memcpy =
				util::get_kmodule_export("ntoskrnl.exe", "memcpy");

			this->syscall<decltype(&memcpy)>(
				ntoskrnl_memcpy, (void*)addr, &value, sizeof T);
		}

		__forceinline auto get_virtual(std::uintptr_t addr) -> std::uintptr_t
		{
			static const auto ntoskrnl_get_virtual =
				util::get_kmodule_export(
					"ntoskrnl.exe",
					"MmGetVirtualForPhysical");

			return this->syscall<MmGetVirtualForPhysical>(
				ntoskrnl_get_virtual, addr);
		}

		__forceinline auto get_peprocess(std::uint32_t pid) -> PEPROCESS
		{
			static const auto ps_lookup_peproc =
				util::get_kmodule_export(
					"ntoskrnl.exe",
					"PsLookupProcessByProcessId");

			PEPROCESS peproc = nullptr;
			this->syscall<PsLookupProcessByProcessId>(
				ps_lookup_peproc,
				(HANDLE)pid,
				&peproc
			);
			return peproc;
		}
	private:
		void locate_syscall(std::uintptr_t begin, std::uintptr_t end) const;
		bool valid_syscall(void* syscall_addr) const;

		std::function<decltype(vdm::read_phys)> read_phys;
		std::function<decltype(vdm::write_phys)> write_phys;
	};
}