#ifndef DTRACK_
#define DTRACK_

#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <array>
#include <unordered_map>
#include <map>
#include "signals.h"

namespace dtrack
{
  namespace detail
  {
    bool CheckBit(uintptr_t bits) {
      return bits && !(bits & (bits - 1));
    }

    template<size_t N>
    struct FirstSetBitForward {

    };

    template<>
    struct FirstSetBitForward<sizeof(unsigned long)> {
      std::tuple<bool, size_t> operator()(unsigned long value) {
        unsigned long index;
        if (_BitScanForward(
          &index,
          value
        )) {
          return std::make_tuple(true, index);
        }
        return std::make_tuple(false, 0);
      }
    };

    template<>
    struct FirstSetBitForward<sizeof(unsigned __int64)> {
      std::tuple<bool, size_t> operator()(unsigned __int64 value) {
        unsigned long index;
        if (_BitScanForward64(
          &index,
          value
        )) {
          return std::make_tuple(true, index);
        }
        return std::make_tuple(false, 0);
      }
    };

    template<size_t N>
    struct FirstSetBitReverse {

    };

    template<>
    struct FirstSetBitReverse<sizeof(unsigned long)> {
      std::tuple<bool, size_t> operator()(unsigned long value) {
        unsigned long index;
        if (_BitScanReverse(
          &index,
          value
        )) {
          return std::make_tuple(true, index);
        }
        return std::make_tuple(false, 0);
      }
    };

    template<>
    struct FirstSetBitReverse<sizeof(unsigned __int64)> {
      std::tuple<bool, size_t> operator()(unsigned __int64 value) {
        unsigned long index;
        if (_BitScanReverse64(
          &index,
          value
        )) {
          return std::make_tuple(true, index);
        }
        return std::make_tuple(false, 0);
      }
    };

    class GlobalBlock;
    class TrackerBase;

    struct PositionComparator {
      bool operator() (
        const std::tuple<size_t, uintptr_t>& lhs,
        const std::tuple<size_t, uintptr_t>& rhs
      ) const {
        if (
          (
            std::get<1>(lhs)
            ==
            std::numeric_limits<std::tuple_element_t<1, std::tuple<size_t, uintptr_t>>>::max()
          )
          &&
          (
            std::get<1>(rhs)
            ==
            std::numeric_limits<std::tuple_element_t<1, std::tuple<size_t, uintptr_t>>>::max()
          )
        ) {
          return std::get<0>(lhs) < std::get<1>(rhs);
        }

        if (
          (
            std::get<1>(lhs)
            ==
            std::numeric_limits<std::tuple_element_t<1, std::tuple<size_t, uintptr_t>>>::max()
          )
          &&
          (
            std::get<1>(rhs)
            !=
            std::numeric_limits<std::tuple_element_t<1, std::tuple<size_t, uintptr_t>>>::max()
          )
        ) {
          return false;
        }
        if (
          (
            std::get<1>(lhs)
            !=
            std::numeric_limits<std::tuple_element_t<1, std::tuple<size_t, uintptr_t>>>::max()
          )
          &&
          (
            std::get<1>(rhs)
            ==
            std::numeric_limits<std::tuple_element_t<1, std::tuple<size_t, uintptr_t>>>::max()
          )
        ) {
          return true;
        }
        return std::get<0>(lhs) < std::get<1>(rhs);
      }
    };

    class GlobalBlock {
    public:
      GlobalBlock()
        : trackers_position_()
        , trackers_validation_status_()
        , trackers_() {

      }

      GlobalBlock(const GlobalBlock&) = delete;

      GlobalBlock& operator=(const GlobalBlock& another) = delete;

      void AllocateTracker(const std::shared_ptr<TrackerBase>& tracker);

      void FreeTracker(const std::shared_ptr<TrackerBase>& tracker);

      void Apply();

      void CommitValidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position);

      void CommitInvalidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position);

      bool IsTrackerValid(const std::tuple<size_t, uintptr_t>& tracker_position) const;

    private:
      std::set<std::tuple<size_t, uintptr_t>, PositionComparator> trackers_position_;
      std::vector<uintptr_t> trackers_validation_status_;
      std::unordered_map<size_t, std::array<std::weak_ptr<TrackerBase>, sizeof(uintptr_t) * CHAR_BIT>> trackers_;
    };

    template<typename T>
    class Trackable {
    public:
      Trackable(const std::shared_ptr<GlobalBlock>& global_block, const T& default_value)
        : value_(default_value)
        , global_block_(global_block)
        , tracked_positions_() {

      }

      void Invalidate() {
        for (
          std::unordered_map<size_t, uintptr_t>::iterator it = tracked_positions_.begin();
          it != tracked_positions_.end();
          ++it
          ) {
          global_block_->CommitInvalidatedTracker(*it);
        }
      }

      void Track(const std::tuple<size_t, uintptr_t>& position) {
        CheckBit(std::get<1>(position));
        std::unordered_map<size_t, uintptr_t>::iterator it = tracked_positions_.find(std::get<0>(position));
        if (it == tracked_positions_.end()) {
          tracked_positions_.emplace(std::get<0>(position), std::get<1>(position)).first;
          return;
        }
        it->second = it->second | std::get<1>(position);
      }

      void StopTrack(const std::tuple<size_t, uintptr_t>& position) {
        CheckBit(std::get<1>(position));
        std::unordered_map<size_t, uintptr_t>::iterator it = tracked_positions_.find(std::get<0>(position));
        assert(it != tracked_positions_.end());
        it->second = it->second & (~std::get<1>(position));
      }

      T Value() const { return value_; }

      T& ValueRef() { return value_; }

      const T& ValueRef() const { return value_; }

      void SetValue(const T& new_value) {
        if (value_ != new_value) {
          value_ = new_value;
          Invalidate();
        }
      }

    private:
      T value_;
      std::shared_ptr<GlobalBlock> global_block_;
      std::unordered_map<size_t, uintptr_t> tracked_positions_;
    };

    class TrackerBase {
    public:
      TrackerBase(const std::shared_ptr<GlobalBlock>& global_block)
        : global_block_(global_block)
        , position_() {

      }

      bool IsValid() const {
        return GetGlobalBlock()->IsTrackerValid(position_);
      }

      virtual void NotifyInvalidated() = 0;

      std::tuple<size_t, uintptr_t> Position() const { return position_; }

      void SetPosition(const std::tuple<size_t, uintptr_t>& new_position) { position_ = new_position; }

      std::shared_ptr<GlobalBlock> GetGlobalBlock() const { return global_block_; }

    private:
      std::shared_ptr<GlobalBlock> global_block_;
      std::tuple<size_t, uintptr_t> position_;
    };

    template<typename R, typename... T>
    R InvokeImpl(
      const std::function<R(const T&...)>& function,
      const std::shared_ptr<Trackable<T>>&... args
    ) {
      return function((args ? args->ValueRef() : T())...);
    }

    template <typename R, typename... T, std::size_t... I>
    R ExpandInvoke(
      const std::function<R(const T&...)>& function,
      const std::tuple<std::weak_ptr<Trackable<T>>...>& arguments,
      std::index_sequence<I...>
    ) {
      return InvokeImpl<R, T...>(
        function,
        std::get<I>(arguments).lock()...
      );
    }

    template<typename R, typename... T>
    R Invoke(
      const std::function<R(const T&...)>& function,
      const std::tuple<std::weak_ptr<Trackable<T>>...>& arguments
    ) {
      return ExpandInvoke(
        function,
        arguments,
        std::index_sequence_for<T...>{}
      );
    }

    template<typename T, typename... N>
    class Tracker : public TrackerBase {
    public:
      Tracker(
        const std::shared_ptr<GlobalBlock>& global_block,
        const T& default_value,
        const std::function<T(const N&...)>& calculator
      )
        : TrackerBase(global_block)
        , tracked_value_(std::make_shared<Trackable<T>>(global_block, default_value))
        , tracking_values_()
        , calculator_(calculator) {

      }

      virtual void NotifyInvalidated() override {
        Apply();
      }

      template<size_t index>
      void Watch(const std::shared_ptr<Trackable<std::tuple_element_t<index, std::tuple<N...>>>>& value) {
        std::shared_ptr<Trackable<std::tuple_element_t<index, std::tuple<N...>>>> current_tracking_value_locked
          = std::get<index>(tracking_values_).lock();
        if (current_tracking_value_locked) {
          current_tracking_value_locked->StopTrack(Position());
        }
        std::get<index>(tracking_values_) = value;
        value->Track(Position());
      }

      T Value() {
        if (!IsValid()) {
          Apply();
        }
        return tracked_value_->Value();
      }

      void Apply() {
        tracked_value_->SetValue(Invoke<T, N...>(calculator_, tracking_values_));
        GetGlobalBlock()->CommitValidatedTracker(Position());
      }

      T& ValueRef() {
        if (!IsValid()) {
          Apply();
        }
        return tracked_value_->ValueRef();
      }

      const T& ValueRef() const {
        if (!IsValid()) {
          Apply();
        }
        return tracked_value_->ValueRef();
      }

    private:
      std::shared_ptr<Trackable<T>> tracked_value_;
      std::tuple<std::weak_ptr<Trackable<N>>...> tracking_values_;
      std::function<T(const N&...)> calculator_;
    };

    void GlobalBlock::AllocateTracker(const std::shared_ptr<TrackerBase>& tracker) {
      if (trackers_position_.empty()) {
        assert(trackers_.empty());
        std::tuple<size_t, uintptr_t> new_position;
        std::get<0>(new_position) = 0;
        std::get<1>(new_position) = 1;
        tracker->SetPosition(new_position);
        trackers_position_.emplace(0, 1);
        std::array<std::weak_ptr<TrackerBase>, sizeof(uintptr_t) * CHAR_BIT> tracker_container;
        tracker_container[0] = tracker;
        trackers_.emplace(0, tracker_container);
        return;
      }
      std::tuple<size_t, uintptr_t> first_tracker_position = *(trackers_position_.begin());
      if (
        std::get<1>(first_tracker_position)
        ==
        std::numeric_limits<
          std::tuple_element_t<
            1,
            std::tuple<size_t, uintptr_t>
          >
        >::max()
      ) {
        size_t available = 0;
        if (std::get<0>(first_tracker_position) > 0) {
          available = 0;
        } else {
          std::set<std::tuple<size_t, uintptr_t>, PositionComparator>::iterator
            left = trackers_position_.begin();
          std::set<std::tuple<size_t, uintptr_t>, PositionComparator>::iterator
            right = left;
          std::advance(right, 1);
          while (right != trackers_position_.end()) {
            if (std::get<0>(*right) - std::get<0>(*left) != 1) {
              break;
            }
            std::advance(left, 1);
            std::advance(right, 1);
          }
          available = std::get<0>(*left) + 1;
        }
        std::tuple<size_t, uintptr_t> new_tracker_position;
        std::get<0>(new_tracker_position) = available;
        std::get<1>(new_tracker_position) = 1;
        tracker->SetPosition(new_tracker_position);
        trackers_position_.insert(new_tracker_position);
        std::array<std::weak_ptr<TrackerBase>, sizeof(uintptr_t)* CHAR_BIT> new_trackers;
        new_trackers[0] = tracker;
        trackers_.emplace(std::get<0>(new_tracker_position), new_trackers);
        return;
      }
      uintptr_t already_occupied_position = std::get<1>(first_tracker_position);
      trackers_position_.erase(trackers_position_.begin());
      trackers_position_.emplace(std::get<0>(first_tracker_position), (already_occupied_position << 1) | 1);
      std::tuple<size_t, uintptr_t> new_trackers_position;
      std::get<0>(new_trackers_position) = std::get<0>(first_tracker_position);
      std::get<1>(new_trackers_position) = already_occupied_position + 1;
      tracker->SetPosition(new_trackers_position);
      FirstSetBitReverse<sizeof(uintptr_t)> scaner;
      std::tuple<bool, size_t> bit_position = scaner(std::get<1>(new_trackers_position));
      assert(std::get<0>(bit_position));
      trackers_[std::get<0>(new_trackers_position)][std::get<1>(bit_position)] = tracker;
    }

    void GlobalBlock::FreeTracker(const std::shared_ptr<TrackerBase>& tracker) {
      std::tuple<size_t, uintptr_t> position = tracker->Position();
      assert(CheckBit(std::get<1>(position)));
      std::tuple<size_t, uintptr_t> lower_bound = position;
      std::get<1>(lower_bound) = std::numeric_limits<uintptr_t>::min();
      std::set<std::tuple<size_t, uintptr_t>, PositionComparator>::iterator it_position =
        trackers_position_.upper_bound(lower_bound);
      assert(it_position != trackers_position_.end());
      if (std::get<0>(*it_position) != std::get<0>(position)) {
        std::tuple<size_t, uintptr_t> exact_tracker_position = position;
        std::get<1>(exact_tracker_position) = std::numeric_limits<uintptr_t>::max();
        it_position = trackers_position_.find(exact_tracker_position);
      }
      assert(it_position != trackers_position_.end());
      std::tuple<size_t, uintptr_t> current_trackers_position = *it_position;
      trackers_position_.erase(it_position);
      std::get<1>(current_trackers_position) =
        std::get<1>(current_trackers_position) & (~std::get<1>(position));
      bool is_all_tracker_removed = std::get<1>(current_trackers_position) == 0;
      if (!is_all_tracker_removed) {
        trackers_position_.insert(current_trackers_position);
      }
      if (std::get<0>(position) < trackers_validation_status_.size()) {
        trackers_validation_status_[std::get<0>(position)] =
          trackers_validation_status_[std::get<0>(position)]
          &
          (~std::get<1>(position));
      }
      std::unordered_map<size_t, std::array<std::weak_ptr<TrackerBase>, sizeof(uintptr_t)* CHAR_BIT>>::iterator
        it_tracker = trackers_.find(std::get<0>(position));
      assert(it_tracker != trackers_.end());
      FirstSetBitForward<sizeof(uintptr_t)> scaner;
      std::tuple<bool, size_t> bit_position = scaner(std::get<1>(position));
      assert(std::get<0>(bit_position));
      trackers_[std::get<0>(position)][std::get<1>(bit_position)].reset();
      if (is_all_tracker_removed) {
        trackers_.erase(it_tracker);
      }
    }

    void GlobalBlock::Apply() {

    }

    void GlobalBlock::CommitValidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position) {
      assert(CheckBit(std::get<1>(tracker_position)));
      if (std::get<0>(tracker_position) >= trackers_validation_status_.size()) {
        return;
      }
      trackers_validation_status_[std::get<0>(tracker_position)] =
        trackers_validation_status_[std::get<0>(tracker_position)] & (~std::get<1>(tracker_position));
    }

    void GlobalBlock::CommitInvalidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position) {
      assert(CheckBit(std::get<1>(tracker_position)));
      if (trackers_validation_status_.size() <= std::get<0>(tracker_position)) {
        trackers_validation_status_.resize(std::get<0>(tracker_position) + 1, 0);
      }
      trackers_validation_status_[std::get<0>(tracker_position)] =
        trackers_validation_status_[std::get<0>(tracker_position)] | std::get<1>(tracker_position);
    }

    bool GlobalBlock::IsTrackerValid(const std::tuple<size_t, uintptr_t>& tracker_position) const {
      if (std::get<0>(tracker_position) >= trackers_validation_status_.size()) {
        return true;
      }
      assert(CheckBit(std::get<1>(tracker_position)));
      return trackers_validation_status_.at(std::get<0>(tracker_position)) ^ std::get<1>(tracker_position);
    }

    std::shared_ptr<GlobalBlock> GetGlobalBlock() {
      static std::weak_ptr<GlobalBlock> instance;
      if (std::shared_ptr<GlobalBlock> result = instance.lock()) {
        return result;
      }
      std::shared_ptr<GlobalBlock> created_instance = std::make_shared<GlobalBlock>();
      instance = created_instance;
      return created_instance;
    }
  }

  template<typename T>
  class DValue {
  public:
    DValue(const T& default_value)
      : tracked_value_(std::make_shared<detail::Trackable<T>>(detail::GetGlobalBlock(), default_value)) {

    }

    void SetValue(T value) {
      tracked_value_->SetValue(value);
    }

    T Value() { return tracked_value_->Value(); }

    T& ValueRef() { return tracked_value_->ValueRef(); }

    const T& ValueRef() const { return tracked_value_->ValueRef(); }

  public:
    std::shared_ptr<detail::Trackable<T>> tracked_value_;
  };

  template<typename T>
  class DVector {
  public:


  private:

  };

  template<typename T, typename... N>
  class DTracker {
  public:
    DTracker(const T& default_value, const std::function<T(const N&...)>& calculator)
      : shared_block_(std::make_shared<detail::Tracker<T, N...>>(detail::GetGlobalBlock(), default_value, calculator))
    {
      detail::GetGlobalBlock()->AllocateTracker(shared_block_);
    }

    ~DTracker() {
      detail::GetGlobalBlock()->FreeTracker(shared_block_);
    }

    template<size_t index>
    DTracker& Watch(const DValue<std::tuple_element_t<index, std::tuple<N...>>>& value) {
      shared_block_->Watch<index>(value.tracked_value_);
      return *this;
    }

    template<size_t index>
    DTracker& Watch(const DTracker<std::tuple_element_t<index, std::tuple<N...>>>& tracker) {
      shared_block_->Watch<index>(tracker.tracked_value_);
      return *this;
    }

    template<size_t index>
    DTracker& Bind(std::function<void(const T& value)>) {
      return *this;
    }

    void Apply() {
      shared_block_->Apply();
    }

    T Value() {
      return shared_block_->Value();
    }

  private:
    std::shared_ptr<detail::Tracker<T, N...>> shared_block_;
  };

  void Apply() {
    detail::GetGlobalBlock()->Apply();
  }
}

#endif // DTRACK_
