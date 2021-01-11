#include "./time.hpp"
#include <Arduino.h>

auto Time::update() -> void {
  this->lastUpdated = millis();
}

auto Time::isElapsed(const uint64_t & span) const -> bool {
  // 0は初回とみなしてtrueを返す
  if (this->lastUpdated == 0) {
    return true;
  }
  const auto current = millis();
  if (current >= this->lastUpdated) {
    return current - this->lastUpdated > span;
  }
  // 過去にupdateした時点よりcurrentの方が小さい場合
  return -1UL - this->lastUpdated + current > span; // 桁溢れまでの量 + 桁溢れした量 > span
}
