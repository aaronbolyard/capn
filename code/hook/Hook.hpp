/// This file is a part of Capn.
///
/// Capn is a useful and multipurpose hooking library for the Windows platform.
///
/// Copyright 2015 Aaron Bolyard.
///
/// For licensing information, review the LICENSE file located at the root
/// directory of the source package.
#ifndef CAPN_HOOK_HPP_
#define CAPN_HOOK_HPP_

enum HOOK_TYPE_FLAGS
{
	// The hook should install itself into the export table.
	HOOK_TYPE_FLAG_EXPORT = 1,

	// The hook should install itself into the import table.
	HOOK_TYPE_FLAG_IMPORT = 2,

	// The hook should try all methods to install itself.
	HOOK_TYPE_FLAG_ALL = HOOK_TYPE_FLAG_EXPORT | HOOK_TYPE_FLAG_IMPORT
};

// A symbol can be from any DLL or executable.
// This structure handles all the necessary data.
struct Symbol
{
	void* function;
	void** address;
	void* moduleAddress;
	
	// Constructor.
	Symbol();
};

struct Hook
{
	Symbol exportSymbol;
	Symbol importSymbol;

	// Constructor. Replaces the provided function with the new function.
	Hook(const char* dll, const char* func, void* newFunc, bool alwaysLoad = false, HOOK_TYPE_FLAGS flags = HOOK_TYPE_FLAG_ALL);

	// Sets the hook to the provided value.
	// Useful for returning to the original functionality, or changing the hook later.
	bool SetExportHook(void* newFunc);

	// Sets the hook to the provided value.
	bool SetImportHook(void* newFunc);
};

// Declares, but does not yet, define a hook.
#define HOOK_DECLARE(funcName, funcModule, returnType, callingConvention, ...) \
	returnType callingConvention funcName##Func (__VA_ARGS__); \
	typedef returnType (callingConvention * funcName##Proc)(__VA_ARGS__); \
	Hook funcName##Hook(funcModule, #funcName, (void*)funcName##Func, false);

// Defines a previously declared hook.
#define HOOK_DEFINE(funcName, returnType, callingConvention, ...) \
	returnType callingConvention funcName##Func(__VA_ARGS__) \

// Calls the original procedure of a previously declared hook.
#define HOOK_CALL_PROC(funcName, ...) ((funcName##Proc)funcName##Hook.exportSymbol.function)(__VA_ARGS__)

// Utility method to declare and define a hook in one place
#define HOOK_UTIL_CREATE(funcName, funcModule, returnType, callingConvention, ...) \
	HOOK_DECLARE(funcName, funcModule, returnType, callingConvention, __VA_ARGS__) \
	HOOK_DEFINE(funcName, returnType, callingConvention, __VA_ARGS__) \
	{ \
		funcName##Proc _hook_internal_base_proc = (funcName##Proc)funcName##Hook.exportSymbol.function;

// Utility for so-called 'far' hooks.
// Think of a procedure returned by wglGetProcAddress
// A far hook allows storing the actual procedure, returning a proxy, and having the proxy
// invoke the original if necessary.
#define HOOK_UTIL_DECLARE_FAR(funcName, returnType, callingConvention, ...) \
	typedef returnType (callingConvention * funcName##Proc)(__VA_ARGS__); \
	funcName##Proc funcName##FarHook = 0;

// Defines a previously declared far hook.
// The provided value will be cast to the underlying far hook type, therefore...
//   HOOK_UTIL_SET_FAR(glBindFramebuffer, wglGetProcAddress("glBindFramebuffer"));
// ...is valid.
// Note that unlike HOOK_DEFINE, this must be followed by a semicolon.
#define HOOK_UTIL_DEFINE_FAR(funcName, value) \
	(funcName##FarHook = (funcName##Proc)value)
	

#define HOOK_UTIL_IS_FAR_DEFINED(funcName) \
	(funcName##FarHook != 0)

// Creates a far hook. Similar in usage to HOOK_UTIL_CREATE.
#define HOOK_UTIL_CREATE_FAR(funcName, returnType, callingConvention, ...) \
	HOOK_UTIL_DECLARE_FAR(funcName, returnType, callingConvention, __VA_ARGS__); \
	returnType callingConvention funcName##Hook(__VA_ARGS__) \
	{ \
		funcName##Proc _hook_internal_base_proc = funcName##FarHook;

// Gets the far hook.
#define HOOK_UTIL_GET_FAR_PROC(funcName) \
	funcName##Hook

// Calls a previously defined far hook.
#define HOOK_UTIL_CALL_FAR(funcName, ...) \
	funcName##FarHook(__VA_ARGS__);

// Utility method to call the original method of a hook created with HOOK_UTIL_CREATE or HOOK_UTIL_CREATE_FAR
#define HOOK_UTIL_CALL_BASE(...) \
	_hook_internal_base_proc(__VA_ARGS__)

// Utility method to end a hook created with HOOK_UTIL_CREATE or HOOK_UTIL_CREATE_FAR
#define HOOK_UTIL_END() }

#endif
