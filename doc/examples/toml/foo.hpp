// foo.hpp

namespace foo {
  void f1();
  void f2();
  void g1();
  void g2();

  class A {
    int x_;

    public:
    int x() const { return x_; }
    void set_x(int x) { x_ = x; }
  };
} // namespace foo