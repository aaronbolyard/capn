# Capn, a simple hook library for Windows
Capn provides a simple mechanism for overriding functions exposed in a DLL, but
most importantly, it provides the framework to declare and define these hooks
with a pretty nifty API.

This project was created a couple years back for personal purposes, but has
since been greatly refactored. I can't find the original documentation on the
method of hooking I used (it was on MSDN); in any case, the code is pretty clear
and self-documenting (and it also has comments where necessary!).

# Using Capn

For the most part, Capn hooks are built from macros. There are two types of
hooks that can be defined with the macros: standard and 'far' hooks.

Standard hooks simply override functions exported from a DLL and functions
imported into an executable. They can be defined like so:

```cpp
// Create a hook matching the signature of glClear located in OPENGL32.DLL
HOOK_UTIL_CREATE(glClear, "OPENGL32.DLL", void, APIENTRY, GLbitfield mask)
	// Simply call the original method and do nothing different!
	HOOK_UTIL_CALL_BASE(mask);
HOOK_UTIL_END()
```

'Far' hooks are a bit more specialized. Say you want to override a function
that is returned by wglGetProcAddress. Normally, you'd want to call the original
at some point. Far hooks provide the framework to easily store the original
function, define and declare a replacement, and eventually call the original:

```cpp
// Very similar to the above!
HOOK_UTIL_CREATE_FAR(glBindFramebuffer, void, APIENTRY, GLenum target, GLuint framebuffer)
	// Simply call the original method! Do nothing different again!
	HOOK_UTIL_CALL_BASE(target, framebuffer);
HOOK_UTIL_END()

// ... later, in a standard hook to wglGetProcAddress ...
// When you get the location of glBindFramebuffer ('proc')...
HOOK_UTIL_DEFINE_FAR(glBindFramebuffer, proc);

// Return the replacement.
return (PROC)HOOK_UTIL_GET_FAR_PROC(glBindFramebuffer);
```

There's a complete example that ships with this source package; see code/example
for the details.

Also, Capn has some extra macros for other purposes; see code/hook/Hook.hpp for
more information on all of these macros.

# Injection

Capn also comes a simple utility to inject hooks, located at code/inject. It
allows injecting hooks into running processes or creating a process and injecting
the hook DLL at startup.

Even though I don't see what more you need from DLL injection utilities, if the
provided utility is not enough, don't worry! Capn should work with basically any
standard form of DLL injection.

# Troubleshooting

Q. The provided injection utility failed to inject my DLL! Why?

A. When you provide the filename of the hook DLL, it will follow LoadLibrary's
search path. Thus, unless the DLL you are injecting is in the application's
directory, you must generally provide the full path to the DLL.

Q. My hook is never called! Why?

A. In your hook DLL, you must link to the various libraries that contain the
hooks you are overriding. It is not enough to simply include the relevant
headers.

The reason is that LoadLibrary cannot be called from DllMain. Instead, you must
depend on the Windows DLL loader. As it stands, calling GetModuleHandle inside
DllMain may not be safe (this is done to find the base location of the DLL in
memory), but I can't find any solid evidence either way.

# Conclusion

While Capn may not be super fancy, it provides a quick-and-easy way of defining
hooks. Please use it responsibly.