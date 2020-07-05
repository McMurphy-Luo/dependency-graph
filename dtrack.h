#ifndef DTRACK_
#define DTRACK_

#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <array>
#include <unordered_map>
#include <map>

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
    class TrackerPosition;

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

      ~GlobalBlock() {

      }

      GlobalBlock(const GlobalBlock&) = delete;

      GlobalBlock& operator=(const GlobalBlock& another) = delete;

      std::shared_ptr<TrackerPosition> AllocatePosition(const std::function<void()>& invalidate_handler);

      void FreePosition(const std::shared_ptr<TrackerPosition>& position);

      void Apply();

      void CommitValidatedPosition(const std::tuple<size_t, uintptr_t>& tracker_position);

      void CommitInvalidatedPosition(const std::tuple<size_t, uintptr_t>& tracker_position);

      bool IsPositionValid(const std::tuple<size_t, uintptr_t>& tracker_position) const;

    private:
      std::set<std::tuple<size_t, uintptr_t>, PositionComparator> trackers_position_;
      std::vector<uintptr_t> trackers_validation_status_;
      std::unordered_map<size_t, std::array<std::shared_ptr<TrackerPosition>, sizeof(uintptr_t) * CHAR_BIT>> trackers_;
    };

    template<typename T>
    class Trackable {
    public:
      Trackable(const std::shared_ptr<GlobalBlock>& global_block)
        : value_()
        , global_block_(global_block)
        , tracked_positions_() {

      }

      Trackable(const std::shared_ptr<GlobalBlock>& global_block, const T& default_value)
        : value_(default_value)
        , global_block_(global_block)
        , tracked_positions_() {

      }

      void Invalidate() {
        std::unordered_map<size_t, uintptr_t>::iterator it = tracked_positions_.begin();
        for (; it != tracked_positions_.end(); ++it) {
          global_block_->CommitInvalidatedPosition(*it);
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

    class TrackerPosition {
    public:
      TrackerPosition(const std::tuple<size_t, uintptr_t>& position, std::function<void()> invalidate_handler)
        : invalidate_handler_(invalidate_handler)
        , position_(position) {

      }

      void NotifyInvalidated() {
        invalidate_handler_();
      }

      std::tuple<size_t, uintptr_t> Position() const { return position_; }

    private:
      std::function<void()> invalidate_handler_;
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
      const std::tuple<std::shared_ptr<Trackable<T>>...>& arguments,
      std::index_sequence<I...>
    ) {
      return InvokeImpl<R, T...>(
        function,
        std::get<I>(arguments)...
      );
    }

    template<typename R, typename... T>
    R Invoke(
      const std::function<R(const T&...)>& function,
      const std::tuple<std::shared_ptr<Trackable<T>>...>& arguments
    ) {
      return ExpandInvoke(
        function,
        arguments,
        std::index_sequence_for<T...>{}
      );
    }

    template<typename T>
    void Noop(const T&) {

    }

    template<typename T, typename... N>
    class Tracker {
    public:
      Tracker(
        const std::shared_ptr<GlobalBlock>& global_block,
        const std::function<T(const N&...)>& calculator
      )
        : tracked_value_(std::make_shared<Trackable<T>>(global_block))
        , global_block_(global_block)
        , position_(global_block->AllocatePosition(std::bind(std::mem_fn(&Tracker::NotifyInvalidated), this)))
        , tracking_values_()
        , calculator_(calculator)
        , bind_function_(&Noop<T>) {

      }

      Tracker(
        const std::shared_ptr<GlobalBlock>& global_block,
        const T& default_value,
        const std::function<T(const N&...)>& calculator
      )
        : tracked_value_(std::make_shared<Trackable<T>>(global_block, default_value))
        , global_block_(global_block)
        , position_(global_block->AllocatePosition(std::bind(std::mem_fn(&Tracker::NotifyInvalidated), this)))
        , tracking_values_()
        , calculator_(calculator)
        , bind_function_(&Noop<T>){

      }

      ~Tracker() {
        global_block_->FreePosition(position_);
      }

      bool IsValid() const {
        return global_block_->IsPositionValid(position_->Position());
      }

      void NotifyInvalidated() {
        Apply();
      }

      template<size_t index>
      void Watch(const std::shared_ptr<Trackable<std::tuple_element_t<index, std::tuple<N...>>>>& value) {
        if (std::get<index>(tracking_values_)) {
          std::get<index>(tracking_values_)->StopTrack(position_->Position());
        }
        std::get<index>(tracking_values_) = value;
        value->Track(position_->Position());
      }

      void Update() {
        tracked_value_->SetValue(Invoke<T, N...>(calculator_, tracking_values_));
        global_block_->CommitValidatedPosition(position_->Position());
      }

      void Bind(const std::function<void (const T&)>& bind_function) {
        bind_function_ = bind_function;
      }

      void Apply() {
        bind_function_(tracked_value_->ValueRef());
      }

      T Value() {
        if (!IsValid()) {
          Update();
        }
        return tracked_value_->Value();
      }

      const T& ValueRef() const {
        if (!IsValid()) {
          Update();
        }
        return tracked_value_->ValueRef();
      }

    private:
      std::shared_ptr<Trackable<T>> tracked_value_;
      std::shared_ptr<GlobalBlock> global_block_;
      std::shared_ptr<TrackerPosition> position_;
      std::tuple<std::shared_ptr<Trackable<N>>...> tracking_values_;
      std::function<T(const N&...)> calculator_;
      std::function<void(const T&)> bind_function_;
    };

    std::shared_ptr<TrackerPosition> GlobalBlock::AllocatePosition(
      const std::function<void()>& invalidate_handler
    ) {
      if (trackers_position_.empty()) {
        assert(trackers_.empty());
        std::tuple<size_t, uintptr_t> position_allocated;
        std::get<0>(position_allocated) = 0;
        std::get<1>(position_allocated) = 1;
        std::shared_ptr<TrackerPosition> new_tracker_position =
          std::make_shared<TrackerPosition>(position_allocated, invalidate_handler);
        trackers_position_.emplace(0, 1);
        std::array<std::shared_ptr<TrackerPosition>, sizeof(uintptr_t) * CHAR_BIT> tracker_container;
        tracker_container[0] = new_tracker_position;
        trackers_.emplace(0, tracker_container);
        return new_tracker_position;
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
        std::tuple<size_t, uintptr_t> position_allocated;
        std::get<0>(position_allocated) = available;
        std::get<1>(position_allocated) = 1;
        std::shared_ptr<TrackerPosition> new_tracker_position =
          std::make_shared<TrackerPosition>(position_allocated, invalidate_handler);
        trackers_position_.insert(position_allocated);
        std::array<std::shared_ptr<TrackerPosition>, sizeof(uintptr_t)* CHAR_BIT> new_trackers;
        new_trackers[0] = new_tracker_position;
        trackers_.emplace(std::get<0>(position_allocated), new_trackers);
        return new_tracker_position;
      }
      uintptr_t already_occupied_position = std::get<1>(first_tracker_position);
      trackers_position_.erase(trackers_position_.begin());
      trackers_position_.emplace(std::get<0>(first_tracker_position), (already_occupied_position << 1) | 1);
      std::tuple<size_t, uintptr_t> position_allocated;
      std::get<0>(position_allocated) = std::get<0>(first_tracker_position);
      std::get<1>(position_allocated) = already_occupied_position + 1;
      std::shared_ptr<TrackerPosition> new_tracker_position =
        std::make_shared<TrackerPosition>(position_allocated, invalidate_handler);
      FirstSetBitReverse<sizeof(uintptr_t)> scaner;
      std::tuple<bool, size_t> bit_position = scaner(std::get<1>(position_allocated));
      assert(std::get<0>(bit_position));
      trackers_[std::get<0>(position_allocated)][std::get<1>(bit_position)] = new_tracker_position;
      return new_tracker_position;
    }

    void GlobalBlock::FreePosition(const std::shared_ptr<TrackerPosition>& tracker) {
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
      std::unordered_map<size_t, std::array<std::shared_ptr<TrackerPosition>, sizeof(uintptr_t)* CHAR_BIT>>::iterator
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

    void GlobalBlock::CommitValidatedPosition(const std::tuple<size_t, uintptr_t>& tracker_position) {
      assert(CheckBit(std::get<1>(tracker_position)));
      if (std::get<0>(tracker_position) >= trackers_validation_status_.size()) {
        return;
      }
      trackers_validation_status_[std::get<0>(tracker_position)] =
        trackers_validation_status_[std::get<0>(tracker_position)] & (~std::get<1>(tracker_position));
    }

    void GlobalBlock::CommitInvalidatedPosition(const std::tuple<size_t, uintptr_t>& tracker_position) {
      if (trackers_validation_status_.size() <= std::get<0>(tracker_position)) {
        trackers_validation_status_.resize(std::get<0>(tracker_position) + 1, 0);
      }
      trackers_validation_status_[std::get<0>(tracker_position)] =
        trackers_validation_status_[std::get<0>(tracker_position)] | std::get<1>(tracker_position);
    }

    bool GlobalBlock::IsPositionValid(const std::tuple<size_t, uintptr_t>& tracker_position) const {
      if (std::get<0>(tracker_position) >= trackers_validation_status_.size()) {
        return true;
      }
      assert(CheckBit(std::get<1>(tracker_position)));
      return trackers_validation_status_.at(std::get<0>(tracker_position)) ^ std::get<1>(tracker_position);
    }

    template<typename T>
    T Argument(const T& value) {
      return value;
    }
  }

  class DTrack {
  public:
    template<typename T>
    friend class DValue;
    template<typename T, typename... N>
    friend class DTracker;

  public:
    DTrack()
      : global_block_(std::make_shared<detail::GlobalBlock>()) {

    }

    void Apply() {
      global_block_->Apply();
    }

  private:
    std::shared_ptr<detail::GlobalBlock> global_block_;
  };

  template<typename T>
  class DValue {
  public:
    DValue(const DTrack& global_block)
      : tracked_value_(std::make_shared<detail::Trackable<T>>(global_block.global_block_)) {

    }

    DValue(const DTrack& global_block, const T& default_value)
      : tracked_value_(std::make_shared<detail::Trackable<T>>(global_block.global_block_, default_value)) {

    }

    void SetValue(T value) {
      tracked_value_->SetValue(value);
    }

    T Value() { return tracked_value_->Value(); }

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
    DTracker(const DTrack& global_block, const std::function<T(const N&...)>& calculator)
      : shared_block_(std::make_shared<detail::Tracker<T, N...>>(global_block.global_block_, calculator))
    {

    }

    DTracker(const DTrack& global_block, const T& default_value, const std::function<T(const N&...)>& calculator)
      : shared_block_(std::make_shared<detail::Tracker<T, N...>>(global_block.global_block_, default_value, calculator))
    {

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

    void Update() {
      shared_block_->Update();
    }

    void Apply() {
      shared_block_->Apply();
    }

    T Value() {
      return shared_block_->Value();
    }

    const T& ValueRef() const {
      return shared_block_->ValueRef();
    }

  private:
    std::shared_ptr<detail::Tracker<T, N...>> shared_block_;
  };
}

#endif // DTRACK_
