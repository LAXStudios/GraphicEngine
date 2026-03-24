#pragma once

#include <GL/glew.h>
#include <iostream>
#include <string>

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

#define ASSERT(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      DEBUG_BREAK();                                                           \
    }                                                                          \
  } while (0)

#define glCall(x)                                                              \
  do {                                                                         \
    GLClearError();                                                            \
    (x);                                                                       \
    ASSERT(!GLPrintErrors(#x, __FILE__, __LINE__));                            \
  } while (0)

inline void GLClearError() {
  while (glGetError() != GL_NO_ERROR) { /* clear all errors */
  }
}

inline bool GLPrintErrors(const std::string &functionName,
                          const std::string &file, int line) {
  bool hasErrors = false;
  while (GLenum err = glGetError()) {
    hasErrors = true;
    std::cout << "OpenGL ERROR: " << err << " in function " << functionName
              << " at " << file << ':' << line << '\n';
  }
  std::cerr.flush();
  return hasErrors;
}
