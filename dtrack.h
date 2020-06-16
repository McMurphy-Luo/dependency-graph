#ifndef DTRACK_
#define DTRACK_

#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <array>
#include <unordered_map>
#include "signals.h"

namespace dtrack
{
  namespace detail
  {
    class GlobalBlock {
    private:
      static GlobalBlock g_block;

    public:
      GlobalBlock()
        : {

      }

      void FreeTracker() {

      }

      void Apply();

      void CommitValidatedTracker() {

      }

      void CommitInvalidatedTracker() {

      }

      std::unordered_map<std::shared_ptr<>>
      std::unique_ptr<std::uintptr_t[]> buffer;
    };

    std::shared_ptr<GlobalBlock> GetGlobalBlock() {
      static std::weak_ptr<GlobalBlock> instance;
      if (std::shared_ptr<GlobalBlock> result = instance.lock()) {
        return result;
      }
      std::shared_ptr<GlobalBlock> created_instance = std::make_shared<GlobalBlock>();
      instance = created_instance;
      return created_instance;
    }

    struct ValueSharedBlockBase {
      ValueSharedBlockBase()
        : trackers_position() {

      }

      virtual void Invalidate() = 0;
      std::list<std::pair<size_t, uintptr_t>> trackers_position;
    };

    template<typename T>
    struct ValueSharedBlock : public ValueSharedBlockBase
    {
      ValueSharedBlock(T default_value)
        : value(default_value) {

      }

      virtual void Invalidate() override {
        std::list<std::weak_ptr<detail::TrackerSharedBlockBase>>::iterator it = tracker_bind_to_me.begin();
        while (it != values_bind_to_me.end()) {
          if (it->expired()) {
            it = values_bind_to_me.erase(it);
          }
          else {
            it->lock()->Invalidate();
            ++it;
          }
        }
      }

      T value;
    };

    template<typename T>
    std::shared_ptr<NodeSharedBlock<T>> Transform(std::weak_ptr<NodeSharedBlockBase> ptr) {
      if (ptr.expired()) {
        return std::shared_ptr<NodeSharedBlock<T>>();
      }
      std::shared_ptr<NodeSharedBlockBase> ptr_locked = ptr.lock();
      return std::dynamic_pointer_cast<NodeSharedBlock<T>>(ptr_locked);
    }

    template<typename R, typename... T>
    R InvokeImpl(
      const std::function<R(const T&...)>& function,
      const std::shared_ptr<NodeSharedBlock<T>>&... args
    ) {
      return function((args ? args->value : T())...);
    }
    
    template <typename R, typename... T, std::size_t... I>
    R ExpandInvoke(
      const std::function<R(const T&...)>& function,
      const std::array<std::weak_ptr<NodeSharedBlockBase>, sizeof...(I)>& t,
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
      const std::array<std::weak_ptr<NodeSharedBlockBase>, sizeof...(T)>& arguments
    ) {
      return ExpandInvoke(
        function,
        arguments,
        std::index_sequence_for<T...>{}
      );
    }

    template<typename T, typename... N>
    struct TrackerSharedBlock : public TrackerSharedBlockBase {
      ValueSharedBlock(const T& default_value, const std::function<T(const N&...)>& calculator)
        : value(default_value)
        , is_valid(true)
        , node_shared_block()
        , calculator(calculator) {

      }

      virtual void Invalidate() override {
        is_valid = false;
      }

      T Value() {
        if (is_valid) {
          return value;
        }
        value = Invoke<T, N...>(calculator, node_shared_block);
        is_valid = true;
        return value;
      }

      T value;
      bool is_valid;
      std::array<std::weak_ptr<NodeSharedBlockBase>, sizeof...(N)> node_shared_block;
      std::function<T(const N&...)> calculator;
    };
  }

  template<typename T>
  class DValue {
  public:
    DValue(const T& default_value)
      : shared_block_(std::make_shared<detail::ValueSharedBlock<T>>(default_value)) {

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
      : shared_block_(std::make_shared<detail::TrackerSharedBlock<T, N...>>(default_value, calculator))
    {

    }

    template<size_t index>
    DTracker& Assemble(const DValue<std::tuple_element_t<index, std::tuple<N...>>>& node) {
      shared_block_->node_shared_block[index] = node.shared_block_;
      node.shared_block_->values_bind_to_me.push_back(shared_block_);
      shared_block_->Invalidate();
      return *this;
    }

    template<size_t index>
    DTracker& Assemble(const DTracker<std::tuple_element_t<index, std::tuple<N...>>>& tracker) {
      shared_block_->node_shared_block[index] = node.shared_block_;
      node.shared_block_->values_bind_to_me.push_back(shared_block_);
      shared_block_->Invalidate();
      return *this;
    }

    DTracker& Bind(const std::function<void(T)>& bind_function) {
      shared_block_->node_shared_block[index] = node.shared_block_;
      node.shared_block_->values_bind_to_me.push_back(shared_block_);
      shared_block_->Invalidate();
      return *this;
    }

    void Apply();

    T Value() {
      return shared_block_->Value();
    }

  private:
    std::shared_ptr<detail::TrackerSharedBlock<T, N...>> shared_block_;
  };

  void Apply() {

  }
}

#endif // DTRACK_
