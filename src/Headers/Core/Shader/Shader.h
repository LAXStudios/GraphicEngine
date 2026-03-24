#include <GL/glew.h>
#include <string>

class Shader {
private:
  unsigned int ID;

public:
  Shader(const char *shaderPath);

  unsigned int GetProgram() const;

  void Use();
  void SetBool(const std::string &name, bool value) const;
  void SetInt(const std::string &name, int value) const;
  void SetFloat(const std::string &name, float value) const;
};
