/// This file is a part of Capn.
///
/// Capn is a useful and multipurpose hooking library for the Windows platform.
///
/// Copyright 2015 Aaron Bolyard.
///
/// For licensing information, review the LICENSE file located at the root
/// directory of the source package.
#include <algorithm>
#include <cctype>
#include <cstring>
#include <windows.h>

#include "Hook.hpp"

// Checks if both modules are equal. This method is case insensitive.
bool IsModule(const char* a, const char* b)
{
	std::size_t l = std::strlen(a);

	// Early check.
	if (l != std::strlen(b))
		return false;

	for (std::size_t i = 0; i < l; ++i)
	{
		if (std::tolower(a[i]) != std::tolower(b[i]))
			return false;
	}

	return true;
}

Symbol::Symbol()
	: function(NULL), address(NULL), moduleAddress(NULL)
{
	// Nothing.
}

// Creates a hook.
Hook::Hook(const char* dll, const char* func, void* newFunc, bool alwaysLoad, HOOK_TYPE_FLAGS flags)
{
	// Try and hook the export address table.
	if (flags & HOOK_TYPE_FLAG_EXPORT)
	{
		HMODULE handle = NULL;
	
		// If the library must be loaded, load it here preemptively.
		// Doing this in DllMain is horrible...
		if (alwaysLoad)
			handle = LoadLibrary(dll);
		else
			handle = GetModuleHandle(dll);

		// Only proceed if the handle is valid.
		if (handle)
		{
			// Get the export descriptors.
			DWORD header = (DWORD)handle;
			IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)header;
			IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)(header + dosHeader->e_lfanew);

			// Get the export descriptor.
			IMAGE_EXPORT_DIRECTORY* directory = (IMAGE_EXPORT_DIRECTORY*)(header + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

			// The function names are stored in an array of RVAs.
			DWORD* names = (DWORD*)(header + directory->AddressOfNames);
			WORD* ordinals = (WORD*)(header + directory->AddressOfNameOrdinals);
			DWORD* exports = (DWORD*)(header + directory->AddressOfFunctions);

			// Search for the function provided.
			// Could do a binary search for efficiency...
			for (DWORD i = 0; i < directory->NumberOfNames; ++i)
			{
				const char* name = (const char*)(header + names[i]);

				// There's a match.
				if (std::strcmp(name, func) == 0)
				{
					// Get the value from the ordinal table.
					WORD ordinal = ordinals[i];

					// Get where the RVA is stored.
					exportSymbol.address = (void**)&exports[ordinal];

					// Store the original method's actual value.
					exportSymbol.function = (void*)(header + (DWORD)(*exportSymbol.address));

					break;
				}
			}

			exportSymbol.moduleAddress = handle;
			SetExportHook(newFunc);
		}
	}

	// Try the import table, as well.
	if (flags & HOOK_TYPE_FLAG_IMPORT)
	{
		// Get the import descriptors from the running executable.
		DWORD header = (DWORD)GetModuleHandle(NULL);
		IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)header;
		IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*)(header + dosHeader->e_lfanew);
		IMAGE_IMPORT_DESCRIPTOR* directory = (IMAGE_IMPORT_DESCRIPTOR*)(header + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		int index = -1;

		// Search for the function.
		for (DWORD i = 0; directory[i].Characteristics; ++i)
		{
			const char* module = (const char*)(header + directory[i].Name);
				
			// Check if this the requested module.
			if (IsModule(module, dll))
			{
				index = i;

				break;
			}
		}

		// Only continue if the module was found.
		if (index >= 0)
		{
			IMAGE_THUNK_DATA* names = (IMAGE_THUNK_DATA*)(header + directory[index].OriginalFirstThunk);
			IMAGE_THUNK_DATA* imports = (IMAGE_THUNK_DATA*)(header + directory[index].FirstThunk);
			DWORD index = 0;

			for (DWORD i = 0; names[i].u1.Function; ++i)
			{
				// Check to see if this is the function to be hooked.
				if (!(names[i].u1.Function & 0x80000000))
				{
					// Compare name.
					IMAGE_IMPORT_BY_NAME* name = (IMAGE_IMPORT_BY_NAME*)(header + names[i].u1.AddressOfData);

					if (std::strcmp((const char*)name->Name, func) == 0)
					{
						// Get where the RVA is stored.
						importSymbol.address = (void**)&imports[i].u1.Function;

						// Store the original method as well.
						importSymbol.function = (void*)(*importSymbol.address);
					}
				}

				index++;
			}
		}

		importSymbol.moduleAddress = GetModuleHandle(NULL);
		SetImportHook(newFunc);
	}
}

// Utility function to set a hook.
bool SetHook(void** address, void* hook, void* base)
{
	// Early sanity check.
	if (address == NULL || hook == NULL)
		return false;

	DWORD protection = 0;

	// Limit protections around the export table.
	if (!VirtualProtect(address, sizeof(void*), PAGE_READWRITE, &protection))
		return false;

	// Write the new value.
	// The value must be made relative to the hooked module.
	*address = (void*)((char*)hook - (char*)base);

	// Restore protections.
	if (!VirtualProtect(address, sizeof(void*), protection, &protection))
		return false;

	return true;
}

bool Hook::SetExportHook(void* newFunc)
{
	return SetHook(exportSymbol.address, newFunc, exportSymbol.moduleAddress);
}

bool Hook::SetImportHook(void* newFunc)
{
	// An import hook does not need a base address.
	return SetHook(importSymbol.address, newFunc, NULL);
}
