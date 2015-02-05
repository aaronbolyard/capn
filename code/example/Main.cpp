// This hook clears all buffers bound to the GL_FRAMEBUFFER_READ target to solid
// red. It shows off the usage of standard hooks and 'far' hooks.
#include <windows.h>
#include <GL/gl.h>

#include "Hook.hpp"

// In order to not drag in an OpenGL extension loader, just define the necessary
// enumerations here.
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9

// glBindFramebuffer is the same method, usage-wise, between the EXT and core
// implementations. Therefore, we only need to provide one method for both.
//
// Since glBindFramebuffer can only be retrieved via wglGetProcAddress, it is
// considered a 'far' hook. Far hooks are simply a bit of syntactic sugar that
// allow nearly the same behavior as standard hooks to the programmer.
HOOK_UTIL_CREATE_FAR(glBindFramebuffer, void, APIENTRY, GLenum target, GLuint framebuffer)
	// Call the original method.
	HOOK_UTIL_CALL_BASE(target, framebuffer);
	
	// Now here's the extra logic: clear the framebuffer to red. In order to not
	// alter OpenGL state, we have to get the current clear color, and set it back
	// at the end, which is slow. Alternatively, we could hook glClearColor and
	// store its arguments, then reuse them here.
	if (target != GL_READ_FRAMEBUFFER)
	{
		GLfloat previousClearColor[4];
		glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
		
		// Set new clear color and apply it.
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Revert.
		glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
	}
	
	// Now when this buffer is read from, the color value will be red.
	// Amazing!
HOOK_UTIL_END()

// Hook wglGetProcAddress with a standard hook.
HOOK_UTIL_CREATE(wglGetProcAddress, "OPENGL32.DLL", PROC, WINAPI, LPCSTR lpszProc)
	// Call the original method.
	// We only want to override glBindFramebuffer; any other 
	PROC proc = HOOK_UTIL_CALL_BASE(lpszProc);
	
	// There may not be a valid OpenGL context, or the method is not supported.
	// In such a case, don't bother checking if it is one of the methods we want
	// to hook.
	if (proc != NULL)
	{
		if (strcmp(lpszProc, "glBindFramebuffer") == 0 || strcmp(lpszProc, "glBindFramebufferEXT") == 0)
		{
			HOOK_UTIL_DEFINE_FAR(glBindFramebuffer, proc);

			return (PROC)HOOK_UTIL_GET_FAR_PROC(glBindFramebuffer);
		}
	}
	
	// Return the original value.
	return proc;
HOOK_UTIL_END()
