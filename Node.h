#ifndef DEPENDENCY_GRAPH_NODE_
#define DEPENDENCY_GRAPH_NODE_

#include <functional>

template<typename T>
class Node {
public:
  Node(T default_value)
    : value_(default_value) {
    
  }
  
  void SetValue(T value);

  T Value() const { return value_; }

private:
  T value_;
};

template<typename R, typename... T>
class Node {
public:
  Node(R default_value)
    : value_(default_value) {

  }

  void Bind(std::function<T...> reducer, Node<T>... nodes) {

  }

  R Value() const { return value_; }

public:


private:
  R value_;
};

#endif // DEPENDENCY_GRAPH_NODE_
