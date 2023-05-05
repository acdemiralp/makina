#pragma once

#include <chrono>

namespace mak
{
namespace detail
{
template <typename representation_, typename period_>
class frame_timer
{
public:
  using clock      = std::chrono::high_resolution_clock;

  using rep        = representation_;
  using period     = period_;
  using duration   = std::chrono::duration  <rep  , period  >;
  using time_point = std::chrono::time_point<clock, duration>;

  frame_timer           ()
  : time_      (clock::now())
  , delta_time_(0)
  {
    
  }
  frame_timer           (const frame_timer&  that) = default;
  frame_timer           (      frame_timer&& temp) = default;
 ~frame_timer           ()                         = default;
  frame_timer& operator=(const frame_timer&  that) = default;
  frame_timer& operator=(      frame_timer&& temp) = default;
  
  void              tick      ()
  {
    time_point time = clock::now();
    delta_time_     = time - time_;
    time_           = time;
  }

  [[nodiscard]]
  const time_point& time      () const
  {
    return time_;
  }
  [[nodiscard]]
  const duration&   delta_time() const
  {
    return delta_time_;
  }

protected:
  time_point time_;
  duration   delta_time_;
};
}

using frame_timer = detail::frame_timer<float, std::milli>;
}