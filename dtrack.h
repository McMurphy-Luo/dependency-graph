#ifndef DTRACK_
#define DTRACK_

#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <array>
#include <unordered_map>
#include <bitset>
#include <map>
#include "signals.h"

namespace dtrack
{
  namespace detail
  {
    template<size_t N>
    struct FirstSetBit {

    };

    template<>
    struct FirstSetBit<sizeof(unsigned long)> {
      size_t operator()(unsigned long value) {
        return 0;
      }
    };

    template<>
    struct FirstSetBit<sizeof(unsigned __int64)> {
      size_t operator()(unsigned __int64) {
        return 0;
      }
    };

    class GlobalBlock;

    class TrackableBase {

    };

    template<typename T>
    class Trackable : public TrackableBase {
    public:
      Trackable(const std::shared_ptr<GlobalBlock>& global_block, const T& default_value)
        : global_block_;
        , value_;
        , positions_; {

      }

      void Invalidate() {

      }

      void Track(const std::tuple<size_t, uintptr_t>& position) {

      }

      void StopTrack(const std::tuple<size_t, uintptr_t>& position) {
        
      }

      void SetValue(const T& value) {
        if (value_ == value) {
          return;
        }
        value_ = value;
        Invalidate();
      }

      virtual T Value() = 0;

      std::shared_ptr<GlobalBlock> GlobalBlock() { return global_block_; }

    private:
      std::shared_ptr<GlobalBlock> global_block_;
      T value_;
      std::list<std::tuple<size_t, uintptr_t>> positions_;
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

      std::tuple<size_t, uintptr_t> AllocateTracker(const std::shared_ptr<TrackerSharedBlockBase>& tracker);

      void FreeTracker(const std::shared_ptr<TrackerSharedBlockBase>& tracker);

      void Apply();

      void CommitValidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position);

      void CommitInvalidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position);

      bool IsTrackerValid(const std::tuple<size_t, uintptr_t>& tracker_position);

    private:
      std::set<std::tuple<size_t, uintptr_t>> trackers_position_;
      std::vector<uintptr_t> trackers_validation_status_;
      std::map<size_t, std::array<std::weak_ptr<TrackerSharedBlockBase>, sizeof(uintptr_t)>> trackers_;
    };

    void ValueSharedBlockBase::Invalidate() {
      for (
        std::list<std::tuple<size_t, uintptr_t>>::iterator it = trackers_position.begin();
        it != trackers_position.end();
        ++it
        ) {
        global_block->CommitInvalidatedTracker(*it);
      }
    }

    template<typename T>
    struct ValueSharedBlock : public Trackable<T> {
      ValueSharedBlock(const std::shared_ptr<GlobalBlock>& global_block, const T& default_value)
        : ValueSharedBlockBase(global_block)
        , value(default_value) {

      }
      
      T value;
    };

    template<typename T>
    std::shared_ptr<ValueSharedBlock<T>> Transform(std::weak_ptr<ValueSharedBlockBase> ptr) {
      if (ptr.expired()) {
        return std::shared_ptr<ValueSharedBlock<T>>();
      }
      std::shared_ptr<ValueSharedBlockBase> ptr_locked = ptr.lock();
      return std::dynamic_pointer_cast<ValueSharedBlock<T>>(ptr_locked);
    }

    template<typename R, typename... T>
    R InvokeImpl(
      const std::function<R(const T&...)>& function,
      const std::shared_ptr<ValueSharedBlock<T>>&... args
    ) {
      return function((args ? args->value : T())...);
    }

    template <typename R, typename... T, std::size_t... I>
    R ExpandInvoke(
      const std::function<R(const T&...)>& function,
      const std::array<std::weak_ptr<ValueSharedBlockBase>, sizeof...(I)>& t,
      std::index_sequence<I...>
    ) {
      return InvokeImpl<R, T...>(
        function,
        Transform<std::tuple_element_t<I, std::tuple<T...>>>(t[I])...
      );
    }

    template<typename R, typename... T>
    R Invoke(
      const std::function<R(const T&...)>& function,
      const std::array<std::weak_ptr<ValueSharedBlockBase>, sizeof...(T)>& arguments
    ) {
      return ExpandInvoke(
        function,
        arguments,
        std::index_sequence_for<T...>{}
      );
    }

    template<typename T, typename... N>
    class TrackerSharedBlock : public Trackable<T> {
      TrackerSharedBlock(const std::shared_ptr<GlobalBlock>& global_block, const T& default_value, const std::function<T(const N&...)>& calculator)
        : TrackerSharedBlockBase(global_block)
        , value(default_value)
        , values()
        , calculator(calculator) {

      }

      template<size_t index>
      void Bind(const std::shared_ptr<ValueSharedBlock<std::tuple_element_t<index, std::tuple<N...>>>> value) {
        values[index] = value;
        value->trackers_position.push_back(position);
        global_block->CommitValidatedTracker(position);
      }

      virtual T Value() {
        if (global_block->IsTrackerValid(position)) {
          return value;
        }
        value = Invoke<T, N...>(calculator, values);
        global_block->CommitValidatedTracker(position);
        return value;
      }

      T value;
      std::array<std::weak_ptr<ValueSharedBlockBase>, sizeof...(N)> values;
      std::function<T(const N&...)> calculator;
    };

    std::tuple<size_t, uintptr_t>
      GlobalBlock::AllocateTracker(const std::shared_ptr<TrackerSharedBlockBase>& tracker) {
      return std::make_tuple(0, 0);
    }

    void GlobalBlock::FreeTracker(const std::shared_ptr<TrackerSharedBlockBase>& tracker) {

    }

    void GlobalBlock::Apply() {

    }

    void GlobalBlock::CommitValidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position) {

    }

    void GlobalBlock::CommitInvalidatedTracker(const std::tuple<size_t, uintptr_t>& tracker_position) {

    }

    bool GlobalBlock::IsTrackerValid(const std::tuple<size_t, uintptr_t>& tracker_position) {
      return false;
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
      : shared_block_(std::make_shared<detail::ValueSharedBlock<T>>(detail::GetGlobalBlock(), default_value)) {

    }

    void SetValue(T value) {
      if (shared_block_->value != value) {
        shared_block_->value = value;
        shared_block_->Invalidate();
      }
    }

    T Value() { return shared_block_->value; }

  public:
    std::shared_ptr<detail::ValueSharedBlock<T>> shared_block_;
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
      : shared_block_(std::make_shared<detail::TrackerSharedBlock<T, N...>>(detail::GetGlobalBlock(), default_value, calculator))
    {

    }

    template<size_t index>
    DTracker& Watch(const DValue<std::tuple_element_t<index, std::tuple<N...>>>& value) {
      shared_block_->Bind<index>(value.shared_block_);
      return *this;
    }

    template<size_t index>
    DTracker& Watch(const DTracker<std::tuple_element_t<index, std::tuple<N...>>>& tracker) {
      shared_block_->Bind<index>(tracker.shared_block_);
      return *this;
    }

    template<size_t index>
    DTracker& Bind(std::function<void(const T& value)>) {
      return *this;
    }

    void Apply() {

    }

    T Value() {
      return shared_block_->Value();
    }

  private:
    std::shared_ptr<detail::TrackerSharedBlock<T, N...>> shared_block_;
  };

  void Apply() {
    detail::GetGlobalBlock()->Apply();
  }
}

#endif // DTRACK_
