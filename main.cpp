
//import anyany;
#include "anyany.ixx"
using namespace anyany;

#include <functional>

template <typename T>
struct execute_method {
  static void do_(void* exe, std::function<void()> foo) {
    reinterpret_cast<T*>(exe)->execute(std::move(foo));
  }
};
struct any_executor : any_base<std::allocator<std::byte>, 54, destroy, move, execute_method> {
  void execute(std::function<void()> f) {
    vtable_invoke<execute_method>(std::move(f));
  }
};

#include <iostream>

// TODO - may be ���� ��������� ���������� ��� Any, ���� ��������� soo ��� ���� ��.
// TODO - ����� �������� �������� ����������� ������(� any �� ���� �� �������, ��� �� ���� �������� ���
// ���������� && ||) ���� � �� � ������ ������ ��� ���� ��� �� ��� ������
// TODO - ����� ���������� any ��� �� ������ ����� ������� ��������� ������ � clang, ������� �� ��� ���������
// �� ������� �������� ����(� ����� � �� ����, �.�. ��� ���� ���������� ����� ��������)

// TODO - �������� �� ��� ��� �������� ��������� Soz� ����������(�� ������ ���) (� ���� �������� ������ construct �� ���������� �����(����� ������ ������� ��� �� ��������))
// TODO - ������� ������� � ����������� � cmake add_library / add_executable ��� ��� ����� � kelcoro
struct SomeExe1 {
  SomeExe1(int) {
  }
  void execute(auto) {
    std::cout << "Hello1\n";
  }
};
struct SomeExe2 {
  SomeExe2(std::string) {
  }
  void execute(std::function<void()>) {
    std::cout << "Hello2\n";
  }
};
int main() {
  any_executor x;
  x.emplace<SomeExe1>(5);
  x.execute([] {});
  x.emplace<SomeExe2>("Hello world");
  x.execute([] {});
}