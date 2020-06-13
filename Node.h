#ifndef DEPENDENCY_GRAPH_NODE_
#define DEPENDENCY_GRAPH_NODE_

#include <functional>
#include <utility>
#include <list>
#include <memory>

namespace dependency
{
  namespace detail
  {
    struct NodeSharedBlockBase {
      virtual void Invalidate() = 0;
    };

    template <class F, class Tuple, std::size_t... I>
    constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
    {
      return std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
    }

    template <class F, class Tuple>
    constexpr decltype(auto) apply(F&& f, Tuple&& t)
    {
      return apply_impl(
        std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    struct ValueSharedBlockBase {
      virtual void Invalidate() = 0;
    };
    
    template<typename T>
    struct NodeSharedBlock;

    template<typename T>
    struct NodeSharedBlock : public NodeSharedBlockBase
    {
      NodeSharedBlock(T default_value)
        : value(default_value)
        , values_bind_to_me() {

      }

      virtual void Invalidate() override {
        for (
          std::list<std::weak_ptr<detail::ValueSharedBlockBase>>::iterator it = values_bind_to_me.begin();
          it != values_bind_to_me.end();
          ++it
          ) {
          it->lock()->Invalidate();
        }
      }

      T value;
      std::list<std::weak_ptr<ValueSharedBlockBase>> values_bind_to_me;
    };

    template<typename... T>
    void SetTupleElement(size_t i, std::tuple<T...>& tuple, std::shared_ptr<NodeSharedBlockBase> node_base) {
      switch (i) {
      case 0:
        std::shared_ptr<NodeSharedBlock<
          std::tuple_element_t<0, std::tuple<T...>>
        >> node_shared_block = std::dynamic_pointer_cast<NodeSharedBlock<
          std::tuple_element_t<0, std::tuple<T...>>
        >>(node_base);
        if (node_shared_block) {
          std::get<0>(tuple) = node_shared_block->value;
        }
      }
    }

    template<typename T, typename... N>
    struct ValueSharedBlock : public ValueSharedBlockBase {
      ValueSharedBlock(T default_value, std::function<T(const N&...)> calculator)
        : value(default_value)
        , is_valid(true)
        , node_shared_block(sizeof...(N))
        , calculator(calculator) {

      }

      virtual void Invalidate() override {
        is_valid = false;
      }

      T Value() {
        if (is_valid) {
          return value;
        }
        std::tuple<N...> arguments;
        for (
          int index = 0;
          index < node_shared_block.size();
          ++index
        ) {
          SetTupleElement(index, arguments, node_shared_block[index].lock());
        }
        apply(
          [this](const N&... args) {
            value = this->calculator(args...);
          },
          arguments
        );
        return value;
      }

      T value;
      bool is_valid;
      std::vector<std::weak_ptr<NodeSharedBlockBase>> node_shared_block;
      std::function<T(const N&...)> calculator;
    };
  }

  template<typename T>
  class Node {
  public:
    Node(T default_value)
      : shared_block_(std::make_shared<detail::NodeSharedBlock<T>>(default_value)) {

    }

    void SetValue(T value) {
      shared_block_->value = value;
      shared_block_->Invalidate();
    }

    T Value() { return shared_block_->value; }

    const T& Value() const { return shared_block_->value; }

  public:
    std::shared_ptr<detail::NodeSharedBlock<T>> shared_block_;
  };

  template<typename T, typename... N>
  class Value {
  public:
    Value(T default_value, std::function<T(const N&...)> calculator)
      : shared_block_(std::make_shared<detail::ValueSharedBlock<T, N...>>(default_value, calculator))
    {

    }

    template<size_t index>
    void Bind(const Node<std::tuple_element_t<index, std::tuple<N...>>>& node) {
      shared_block_->node_shared_block[index] = node.shared_block_;
      node.shared_block_->values_bind_to_me.push_back(shared_block_);
    }

    T GetValue() const {
      return shared_block_->Value();
    }

  private:
    std::shared_ptr<detail::ValueSharedBlock<T, N...>> shared_block_;
  };
}

#endif // DEPENDENCY_GRAPH_NODE_
