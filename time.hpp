#include <cstdint>

class Time {
  /**
   * 最終更新時刻
   */
  uint64_t lastUpdated = 0;

public:
  /**
   * lastUpdatedを更新する
   */
  auto update() -> void;

  /**
   * 指定した時間が過ぎているか
   */
  auto isElapsed(const uint64_t & span) const -> bool;
};
