#pragma once
#include "StepNode.h"

template <typename T>
class AccelNode : public T {
   public:
    AccelNode(StepNode& node) : node(node) {}

    StepNode& node;

   private:
};