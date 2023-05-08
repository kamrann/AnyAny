#pragma once

#include "anyany.hpp"

#include <functional>  // std::invoke
#include <optional>    // for type_switch
#include <variant>     // only for std_variant_poly_traits

namespace aa::noexport {

template <typename PolyPtr, typename ResultT = void, typename Traits = anyany_poly_traits>
struct type_switch_impl {
 private:
  struct non_void {
    // no way to create it for user
    explicit non_void() = default;
  };
  using Result = std::conditional_t<std::is_void_v<ResultT>, non_void, ResultT>;

 public:
  constexpr explicit type_switch_impl(PolyPtr value) noexcept : value(std::move(value)) {
    assert(value != nullptr);
  }

  template <typename T, typename Fn>
  type_switch_impl& case_(Fn&& f) {
    if (result)
      return *this;
    if (auto* v = ::aa::any_cast<T, Traits>(value)) {
      if constexpr (std::is_void_v<ResultT>) {
        std::invoke(std::forward<Fn>(f), *v);
        result.emplace();
      } else {
        result = std::invoke(std::forward<Fn>(f), *v);
      }
    }
    return *this;
  }
  // If value is one of Ts... F invoked (invokes count <= 1)
  template <typename... Ts, typename Fn>
  type_switch_impl& cases(Fn&& f) {
    struct assert_ : noexport::type_identity<Ts>... {
    } assert_unique_types;
    (void)assert_unique_types;
    (case_<Ts>(std::forward<Fn>(f)), ...);
    return *this;
  }
  // As a default, invoke the given callable within the root value.
  template <typename Fn>
  [[nodiscard]] Result default_(Fn&& f) {
    if (result)
      return std::move(*result);
    return std::forward<Fn>(f)(*value);
  }
  // As a default, return the given value.
  [[nodiscard]] Result default_(Result v) {
    if (result)
      return std::move(*result);
    return v;
  }
  // explicitly says there are no default value
  // postcondition: return value setted if some 'case_' succeeded
  std::optional<Result> no_default() {
    return std::move(result);
  }

 private:
  // value for which switch created
  // invariant - initially always not null.
  PolyPtr value;
  // stored result and if it exist
  std::optional<Result> result = std::nullopt;
};

}  // namespace noexport

// ######################## ACTION type_switch for all polymorphic types ########################

// Returns instance of type which
// implements a switch-like dispatch statement for poly_ptr/const_poly_ptr
// using any_cast.
// Each `Case<T>` takes a callable to be invoked
// if the root value is a <T>, the callable is invoked with any_cast<T>(ValueInSwitch)
//
// usage example:
//  any_operation<Methods...> op = ...;
//  ResultType result = type_switch<ResultType>(op)
//    .Case<ConstantOp>([](ConstantOp op) { ... })
//    .Default([](const_poly_ref<Methods...> ref) { ... });
namespace aa {
template <typename Result = void, typename Traits = anyany_poly_traits, typename... Methods>
constexpr auto type_switch(poly_ref<Methods...> p) noexcept {
  return noexport::type_switch_impl<poly_ptr<Methods...>, Result, Traits>{&p};
}
template <typename Result = void, typename Traits = anyany_poly_traits, typename... Methods>
constexpr auto type_switch(const_poly_ref<Methods...> p) noexcept {
  return noexport::type_switch_impl<const_poly_ptr<Methods...>, Result, Traits>{&p};
}

// those traits may be used for visit many variants without O(n*m*...) instantiating.
// Its very usefull for pattern matching, but may be slower then matching with visit.
// All variants are polymorphic types for there traits, all other types considered as non-polymorphic
struct std_variant_poly_traits {
  template <typename... Ts>
  static descriptor_t get_type_descriptor(const std::variant<Ts...>& v) noexcept {
    return std::visit([]<typename T>(T&& v) { return descriptor_v<T>; }, v);
  }
  template <typename... Ts>
  static descriptor_t get_type_descriptor(std::variant<Ts...>& v) noexcept {
    return std::visit([]<typename T>(T&& v) { return descriptor_v<T>; }, v);
  }
  template <typename... Ts>
  static descriptor_t get_type_descriptor(std::variant<Ts...>&& v) noexcept {
    return std::visit([]<typename T>(T&& v) { return descriptor_v<T>; }, v);
  }
  template <typename T>
  static descriptor_t get_type_descriptor(T&&) noexcept {
    return descriptor_v<T>;
  }
  template <typename T>
  static constexpr auto* to_address(T&& v) noexcept {
    return static_cast<std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, const void*, void*>>(
        std::addressof(v));
  }
  // Do not support cases like variant<int, const int> with mixed constness
  template <typename... Ts>
  static constexpr const void* to_address(const std::variant<Ts...>& v) noexcept {
    return std::visit([](const auto& v) { return static_cast<const void*>(std::addressof(v)); }, v);
  }
  template <typename... Ts>
  static constexpr void* to_address(std::variant<Ts...>& v) noexcept {
    return std::visit([](auto& v) { return static_cast<void*>(std::addressof(v)); }, v);
  }
  template <typename... Ts>
  static constexpr void* to_address(std::variant<Ts...>&& v) noexcept {
    return std::visit([](auto&& v) { return static_cast<void*>(std::addressof(v)); }, v);
  }
};

}  // namespace aa