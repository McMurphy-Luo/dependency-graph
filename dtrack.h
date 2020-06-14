#ifndef DEPENDENCY_GRAPH_NODE_
#define DEPENDENCY_GRAPH_NODE_

#include <functional>
#include <utility>
#include <list>
#include <memory>
#include <array>

namespace dtrack
{
  namespace detail
  {
    struct NodeSharedBlockBase {
      virtual void Invalidate() = 0;
    };

    struct ValueSharedBlockBase {
      virtual void Invalidate() = 0;
    };

    template<typename T>
    struct NodeSharedBlock : public NodeSharedBlockBase
    {
      NodeSharedBlock(T default_value)
        : value(default_value)
        , values_bind_to_me() {

      }

      virtual void Invalidate() override {
        std::list<std::weak_ptr<detail::ValueSharedBlockBase>>::iterator it = values_bind_to_me.begin();
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
      std::list<std::weak_ptr<ValueSharedBlockBase>> values_bind_to_me;
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
    struct ValueSharedBlock : public ValueSharedBlockBase {
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
  class DNode {
  public:
    DNode(T default_value)
      : shared_block_(std::make_shared<detail::NodeSharedBlock<T>>(default_value)) {

    }

    void SetValue(T value) {
      shared_block_->value = value;
      shared_block_->Invalidate();
    }

    T Value() { return shared_block_->value; }

  public:
    std::shared_ptr<detail::NodeSharedBlock<T>> shared_block_;
  };

  template<typename T, typename... N>
  class DValue {
  public:
    DValue(const T& default_value, const std::function<T(const N&...)>& calculator)
      : shared_block_(std::make_shared<detail::ValueSharedBlock<T, N...>>(default_value, calculator))
    {

    }

    template<size_t index>
    DValue& Bind(const DNode<std::tuple_element_t<index, std::tuple<N...>>>& node) {
      shared_block_->node_shared_block[index] = node.shared_block_;
      node.shared_block_->values_bind_to_me.push_back(shared_block_);
      shared_block_->Invalidate();
      return *this;
    }

    T Value() {
      return shared_block_->Value();
    }

  private:
    std::shared_ptr<detail::ValueSharedBlock<T, N...>> shared_block_;
  };
}

#endif // DEPENDENCY_GRAPH_NODE_
