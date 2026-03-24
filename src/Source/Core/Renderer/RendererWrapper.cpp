#include "../../../Headers/Core/Renderer/RendererWrapper.h"

void RendererWrapper::clear() const {
  glCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void RendererWrapper::draw(const VertexArray &va, const IndexBuffer &ib,
                           const ShaderProgram &shader) const {
  shader.Bind();
  va.Bind();
  glCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}

void RendererWrapper::draw(const VertexArray &va, const ShaderProgram &shader,
                           unsigned int count) {
  shader.Bind();
  va.Bind();
  glCall(glDrawArrays(GL_TRIANGLES, 0, count));
}
