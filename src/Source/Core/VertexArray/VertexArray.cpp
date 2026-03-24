#include "../../../Headers/Core/VertexArray/VertexArray.h"
#include <iostream>
#include <vector>

VertexArray::VertexArray() { glCall(glGenVertexArrays(1, &ID)); }
VertexArray::~VertexArray() { glCall(glDeleteVertexArrays(1, &ID)); }

void VertexArray::AddBuffer(const VertexBuffer &vbo,
                            const VertexBufferLayout &layout) {
  this->Bind();
  vbo.Bind();
  const auto &elements = layout.GetLayoutElements();
  unsigned int offset = 0;

  for (unsigned int i = 0; i < elements.size(); ++i) {
    const auto &element = elements[i];
    this->Bind();
    glCall(glEnableVertexAttribArray(i));
    glCall(glVertexAttribPointer(i, element.count, element.type,
                                 element.normalized, layout.GetStride(),
                                 reinterpret_cast<const void *>(offset)));
    offset += element.count * VertexBufferLayoutElement::GetSize(element.type);

    std::cout << "Stride: " << layout.GetStride() << "\n";
  }
  std::cout << "VBO ID: " << vbo.GetId() << "\nVAO ID: " << this->GetID()
            << '\n';
  vbo.UnBind();
  this->UnBind();
}

unsigned int VertexArray::GetID() const { return ID; }

void VertexArray::Bind() const { glCall(glBindVertexArray(ID)); }

void VertexArray::UnBind() const { glCall(glBindVertexArray(0)); }
