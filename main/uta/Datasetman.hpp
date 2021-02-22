#pragma once

#include "HAL.hpp"
#include "dataset.hpp"

using namespace Kien;

class Picman;

class Datasetman : public Task {
   public:
    ~Datasetman(){};

    void start();

   private:
    void task() override;
};
