#pragma once

#include "annotation.hpp"
#include "node_schema.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#include <meta>
#endif

namespace gkdviz::core {

template <typename Algo>
struct AlgorithmReflectionTraits {
  static constexpr std::string_view display_name = "";
  static constexpr std::string_view category = "General";
};

template <typename T>
inline constexpr bool kSupportedReflectionValueType =
    std::is_same_v<T, bool> || std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> ||
    std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, std::string>;

template <typename T>
[[nodiscard]] constexpr ValueType value_type_of() {
  if constexpr (std::is_same_v<T, bool>) {
    return ValueType::kBool;
  } else if constexpr (std::is_same_v<T, int32_t>) {
    return ValueType::kInt32;
  } else if constexpr (std::is_same_v<T, int64_t>) {
    return ValueType::kInt64;
  } else if constexpr (std::is_same_v<T, float>) {
    return ValueType::kFloat32;
  } else if constexpr (std::is_same_v<T, double>) {
    return ValueType::kFloat64;
  } else if constexpr (std::is_same_v<T, std::string>) {
    return ValueType::kString;
  } else {
    return ValueType::kUnknown;
  }
}

template <typename T>
[[nodiscard]] inline FieldDefaultValue default_value_of(const T& value) {
  if constexpr (std::is_same_v<T, bool>) {
    return FieldDefaultValue{value};
  } else if constexpr (std::is_same_v<T, int32_t>) {
    return FieldDefaultValue{value};
  } else if constexpr (std::is_same_v<T, int64_t>) {
    return FieldDefaultValue{value};
  } else if constexpr (std::is_same_v<T, float>) {
    return FieldDefaultValue{value};
  } else if constexpr (std::is_same_v<T, double>) {
    return FieldDefaultValue{value};
  } else if constexpr (std::is_same_v<T, std::string>) {
    return FieldDefaultValue{value};
  } else {
    return FieldDefaultValue{};
  }
}

#if defined(__cpp_impl_reflection) && __has_include(<meta>)

inline constexpr bool kNativeReflectionAvailable = true;

template <typename Struct>
[[nodiscard]] consteval std::size_t reflected_member_count() {
  return std::meta::nonstatic_data_members_of(^^Struct, std::meta::access_context::unchecked()).size();
}

template <typename Struct, std::size_t Index>
[[nodiscard]] consteval std::meta::info reflected_member() {
  return std::meta::nonstatic_data_members_of(^^Struct, std::meta::access_context::unchecked())[Index];
}

template <std::meta::info Annotation, typename Tag>
[[nodiscard]] consteval bool annotation_matches() {
  return std::meta::is_same_type(std::meta::remove_cv(std::meta::type_of(Annotation)), ^^Tag);
}

template <std::meta::info Member, typename Tag, std::size_t... I>
[[nodiscard]] consteval std::optional<Tag> find_annotation_impl(std::index_sequence<I...>) {
  std::optional<Tag> result;
  (([&] {
      constexpr auto annotation = std::meta::annotations_of(Member)[I];
      if constexpr (annotation_matches<annotation, Tag>()) {
        result = std::meta::extract<Tag>(annotation);
      }
    }()),
   ...);
  return result;
}

template <std::meta::info Member, typename Tag>
[[nodiscard]] consteval std::optional<Tag> find_annotation() {
  constexpr auto annotation_count = std::meta::annotations_of(Member).size();
  return find_annotation_impl<Member, Tag>(std::make_index_sequence<annotation_count>{});
}

template <std::meta::info Member>
[[nodiscard]] inline FieldAnnotation reflected_annotation() {
  FieldAnnotation out{};

  constexpr auto semantic = find_annotation<Member, SemanticAnnotation>();
  if (semantic.has_value()) {
    out.semantic = semantic->value;
  }
  constexpr auto unit = find_annotation<Member, UnitAnnotation>();
  if (unit.has_value()) {
    out.unit = unit->value;
  }
  constexpr auto range = find_annotation<Member, RangeAnnotation>();
  if (range.has_value()) {
    out.range = range->value;
  }
  constexpr auto plottable = find_annotation<Member, PlottableAnnotation>();
  if (plottable.has_value()) {
    out.plottable = plottable->value;
  }
  constexpr auto loggable = find_annotation<Member, LoggableAnnotation>();
  if (loggable.has_value()) {
    out.loggable = loggable->value;
  }

  return out;
}

template <typename Struct, std::size_t Index>
[[nodiscard]] FieldSchema reflected_field_schema() {
  constexpr auto member = reflected_member<Struct, Index>();
  using MemberType = std::remove_cvref_t<typename [: std::meta::type_of(member) :]>;
  static_assert(kSupportedReflectionValueType<MemberType>, "unsupported reflected field type");

  const Struct defaults{};
  auto annotation = reflected_annotation<member>();

  return FieldSchema{
      .name = std::string(std::meta::identifier_of(member)),
      .type =
          PortType{
              .value_type = value_type_of<MemberType>(),
              .semantic = annotation.semantic,
              .unit = annotation.unit,
          },
      .default_value = default_value_of(defaults.[: member :]),
      .annotation = std::move(annotation),
  };
}

template <typename Struct, std::size_t Index>
[[nodiscard]] PortSchema reflected_port_schema() {
  constexpr auto member = reflected_member<Struct, Index>();
  using MemberType = std::remove_cvref_t<typename [: std::meta::type_of(member) :]>;
  static_assert(kSupportedReflectionValueType<MemberType>, "unsupported reflected port type");

  auto annotation = reflected_annotation<member>();
  return PortSchema{
      .name = std::string(std::meta::identifier_of(member)),
      .type =
          PortType{
              .value_type = value_type_of<MemberType>(),
              .semantic = annotation.semantic,
              .unit = annotation.unit,
          },
      .plottable = annotation.plottable,
      .loggable = annotation.loggable,
  };
}

template <typename Struct, std::size_t... I>
[[nodiscard]] std::vector<FieldSchema> reflected_field_schemas_impl(std::index_sequence<I...>) {
  return {reflected_field_schema<Struct, I>()...};
}

template <typename Struct>
[[nodiscard]] std::vector<FieldSchema> reflected_field_schemas() {
  return reflected_field_schemas_impl<Struct>(
      std::make_index_sequence<reflected_member_count<Struct>()>{});
}

template <typename Struct, std::size_t... I>
[[nodiscard]] std::vector<PortSchema> reflected_port_schemas_impl(std::index_sequence<I...>) {
  return {reflected_port_schema<Struct, I>()...};
}

template <typename Struct>
[[nodiscard]] std::vector<PortSchema> reflected_port_schemas() {
  return reflected_port_schemas_impl<Struct>(
      std::make_index_sequence<reflected_member_count<Struct>()>{});
}

template <typename T>
[[nodiscard]] std::string reflected_type_name() {
  return std::string(std::meta::identifier_of(^^T));
}

#else

inline constexpr bool kNativeReflectionAvailable = false;

#endif

} // namespace gkdviz::core
