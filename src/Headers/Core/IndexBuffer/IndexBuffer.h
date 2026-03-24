#pragma once

#include "../Common/Common.h"

class IndexBuffer {
private:
  unsigned int ID;
  unsigned int count;

public:
  IndexBuffer(const unsigned int *data, int count);
  ~IndexBuffer();

  void Bind();
  void UnBind();

  unsigned int GetCount() const;
  unsigned int GetID() const;
};
