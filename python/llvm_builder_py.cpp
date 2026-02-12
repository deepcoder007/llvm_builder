//
// Python bindings for llvm_builder using nanobind
//

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/shared_ptr.h>

#include "llvm_builder/module.h"
#include "llvm_builder/type.h"
#include "llvm_builder/value.h"
#include "llvm_builder/function.h"
#include "llvm_builder/control_flow.h"
#include "llvm_builder/jit.h"

namespace nb = nanobind;
using namespace nb::literals;
using namespace llvm_builder;

// Forward declarations for binding functions
void bind_types(nb::module_& m);
void bind_values(nb::module_& m);
void bind_functions(nb::module_& m);
void bind_modules(nb::module_& m);
void bind_jit(nb::module_& m);
void bind_control_flow(nb::module_& m);

NB_MODULE(llvm_builder_py, m) {
    m.doc() = "Python bindings for llvm_builder - A high-level LLVM IR builder library";

    // Bind in dependency order
    bind_types(m);
    bind_values(m);
    bind_functions(m);
    bind_modules(m);
    bind_control_flow(m);
    bind_jit(m);
}

//
// Type system bindings
//
void bind_types(nb::module_& m) {
    // runtime::type_t enum
    nb::enum_<runtime::type_t>(m, "RuntimeType")
        .value("unknown", runtime::type_t::unknown)
        .value("boolean", runtime::type_t::boolean)
        .value("int8", runtime::type_t::int8)
        .value("int16", runtime::type_t::int16)
        .value("int32", runtime::type_t::int32)
        .value("int64", runtime::type_t::int64)
        .value("uint8", runtime::type_t::uint8)
        .value("uint16", runtime::type_t::uint16)
        .value("uint32", runtime::type_t::uint32)
        .value("uint64", runtime::type_t::uint64)
        .value("float32", runtime::type_t::float32)
        .value("float64", runtime::type_t::float64)
        .value("pointer_struct", runtime::type_t::pointer_struct)
        .value("pointer_array", runtime::type_t::pointer_array);

    // LinkSymbol::symbol_type enum
    nb::enum_<LinkSymbol::symbol_type>(m, "SymbolType")
        .value("unknown", LinkSymbol::symbol_type::unknown)
        .value("custom_struct", LinkSymbol::symbol_type::custom_struct)
        .value("function", LinkSymbol::symbol_type::function);

    // member_field_entry
    nb::class_<member_field_entry>(m, "MemberFieldEntry")
        .def(nb::init<>())
        .def(nb::init<const std::string&, TypeInfo>(), "name"_a, "type"_a)
        .def(nb::init<const std::string&, TypeInfo, bool>(), "name"_a, "type"_a, "is_readonly"_a)
        .def("name", &member_field_entry::name)
        .def("type", &member_field_entry::type)
        .def("is_readonly", &member_field_entry::is_readonly)
        .def("is_valid", &member_field_entry::is_valid);

    // TypeInfo::field_entry_t
    nb::class_<TypeInfo::field_entry_t>(m, "FieldEntry")
        .def("idx", &TypeInfo::field_entry_t::idx)
        .def("offset", &TypeInfo::field_entry_t::offset)
        .def("name", &TypeInfo::field_entry_t::name)
        .def("type", &TypeInfo::field_entry_t::type)
        .def("is_readonly", &TypeInfo::field_entry_t::is_readonly)
        .def_static("null", &TypeInfo::field_entry_t::null, nb::rv_policy::reference);

    // TypeInfo
    nb::class_<TypeInfo>(m, "TypeInfo")
        .def(nb::init<>())
        // Type queries
        .def("is_valid", &TypeInfo::is_valid)
        .def("is_void", &TypeInfo::is_void)
        .def("is_boolean", &TypeInfo::is_boolean)
        .def("is_integer", &TypeInfo::is_integer)
        .def("is_pointer", &TypeInfo::is_pointer)
        .def("is_unsigned_integer", &TypeInfo::is_unsigned_integer)
        .def("is_signed_integer", &TypeInfo::is_signed_integer)
        .def("is_float", &TypeInfo::is_float)
        .def("is_scalar", &TypeInfo::is_scalar)
        .def("is_array", &TypeInfo::is_array)
        .def("is_vector", &TypeInfo::is_vector)
        .def("is_struct", &TypeInfo::is_struct)
        // Size and properties
        .def("size_in_bytes", &TypeInfo::size_in_bytes)
        .def("num_elements", &TypeInfo::num_elements)
        .def("struct_size_bytes", &TypeInfo::struct_size_bytes)
        .def("struct_name", &TypeInfo::struct_name)
        .def("short_name", &TypeInfo::short_name)
        // Related types
        .def("base_type", &TypeInfo::base_type)
        .def("pointer_type", &TypeInfo::pointer_type)
        // Field access
        .def("__getitem__", nb::overload_cast<uint32_t>(&TypeInfo::operator[], nb::const_), "i"_a)
        .def("__getitem__", nb::overload_cast<const std::string&>(&TypeInfo::operator[], nb::const_), "name"_a)
        // Comparison
        .def("__eq__", &TypeInfo::operator==)
        // Static factory methods
        .def_static("null", &TypeInfo::null, nb::rv_policy::reference)
        .def_static("mk_void", &TypeInfo::mk_void)
        .def_static("mk_bool", &TypeInfo::mk_bool)
        .def_static("mk_int8", &TypeInfo::mk_int8)
        .def_static("mk_int16", &TypeInfo::mk_int16)
        .def_static("mk_int32", &TypeInfo::mk_int32)
        .def_static("mk_int64", &TypeInfo::mk_int64)
        .def_static("mk_uint8", &TypeInfo::mk_uint8)
        .def_static("mk_uint16", &TypeInfo::mk_uint16)
        .def_static("mk_uint32", &TypeInfo::mk_uint32)
        .def_static("mk_uint64", &TypeInfo::mk_uint64)
        .def_static("mk_float32", &TypeInfo::mk_float32)
        .def_static("mk_float64", &TypeInfo::mk_float64)
        .def_static("mk_array", &TypeInfo::mk_array, "element_type"_a, "num_elements"_a)
        .def_static("mk_vector", &TypeInfo::mk_vector, "element_type"_a, "num_elements"_a)
        .def_static("mk_struct", &TypeInfo::mk_struct, "name"_a, "element_list"_a, "is_packed"_a = true)
        .def_static("mk_type_from_name", &TypeInfo::mk_type_from_name, "name"_a);

    // LinkSymbolName
    nb::class_<LinkSymbolName>(m, "LinkSymbolName")
        .def(nb::init<>())
        .def(nb::init<const std::string&>(), "name"_a)
        .def(nb::init<const std::string&, const std::string&>(), "ns"_a, "name"_a)
        .def("short_name", &LinkSymbolName::short_name)
        .def("namespace_name", &LinkSymbolName::namespace_name)
        .def("full_name", &LinkSymbolName::full_name)
        .def("is_global", &LinkSymbolName::is_global)
        .def("is_valid", &LinkSymbolName::is_valid)
        .def("equals_name", &LinkSymbolName::equals_name, "name"_a)
        .def("__eq__", &LinkSymbolName::operator==)
        .def_static("null", &LinkSymbolName::null);

    // LinkSymbol
    nb::class_<LinkSymbol>(m, "LinkSymbol")
        .def(nb::init<>())
        .def(nb::init<const LinkSymbolName&, const TypeInfo&, LinkSymbol::symbol_type>(),
             "name"_a, "ctype"_a, "type"_a)
        .def("symbol_name", &LinkSymbol::symbol_name)
        .def("full_name", &LinkSymbol::full_name)
        .def("is_valid", &LinkSymbol::is_valid)
        .def("is_custom_struct", &LinkSymbol::is_custom_struct)
        .def("is_function", &LinkSymbol::is_function)
        .def("equals_name", &LinkSymbol::equals_name, "name"_a)
        .def("add_arg", &LinkSymbol::add_arg, "type"_a, "name"_a)
        .def("__eq__", &LinkSymbol::operator==)
        .def_static("null", &LinkSymbol::null);
}

//
// Value bindings
//
void bind_values(nb::module_& m) {
    // TagInfo
    nb::class_<TagInfo>(m, "TagInfo")
        .def(nb::init<>())
        .def(nb::init<const std::string&>(), "value"_a)
        .def("contains", &TagInfo::contains, "v"_a)
        .def("add_entry", &TagInfo::add_entry, "v"_a)
        .def("set_union", &TagInfo::set_union, "other"_a);

    // ValueInfo
    nb::class_<ValueInfo>(m, "ValueInfo")
        .def(nb::init<>())
        // Queries
        .def("type", &ValueInfo::type)
        .def("tag_info", &ValueInfo::tag_info)
        .def("has_tag", &ValueInfo::has_tag, "v"_a)
        .def("add_tag", nb::overload_cast<const std::string&>(&ValueInfo::add_tag), "v"_a)
        .def("equals_type", nb::overload_cast<const ValueInfo&>(&ValueInfo::equals_type, nb::const_), "other"_a)
        .def("equals_type", nb::overload_cast<TypeInfo>(&ValueInfo::equals_type, nb::const_), "type"_a)
        // Type conversion
        .def("cast", &ValueInfo::cast, "target_type"_a)
        // Arithmetic operations
        .def("add", &ValueInfo::add, "v2"_a)
        .def("sub", &ValueInfo::sub, "v2"_a)
        .def("mult", &ValueInfo::mult, "v2"_a)
        .def("div", &ValueInfo::div, "v2"_a)
        .def("remainder", &ValueInfo::remainder, "v2"_a)
        // Comparison operations
        .def("less_than", &ValueInfo::less_than, "v2"_a)
        .def("less_than_equal", &ValueInfo::less_than_equal, "v2"_a)
        .def("greater_than", &ValueInfo::greater_than, "v2"_a)
        .def("greater_than_equal", &ValueInfo::greater_than_equal, "v2"_a)
        .def("equal", &ValueInfo::equal, "v2"_a)
        .def("not_equal", &ValueInfo::not_equal, "v2"_a)
        // Conditional
        .def("cond", &ValueInfo::cond, "then_value"_a, "else_value"_a)
        // Python operators
        .def("__add__", &ValueInfo::operator+)
        .def("__sub__", &ValueInfo::operator-)
        .def("__mul__", &ValueInfo::operator*)
        .def("__truediv__", &ValueInfo::operator/)
        .def("__mod__", &ValueInfo::operator%)
        .def("__lt__", &ValueInfo::operator<)
        .def("__le__", &ValueInfo::operator<=)
        .def("__gt__", &ValueInfo::operator>)
        .def("__ge__", &ValueInfo::operator>=)
        .def("__eq__", &ValueInfo::operator==)
        // Memory operations
        .def("store", &ValueInfo::store, "value"_a)
        .def("load", &ValueInfo::load)
        .def("entry", nb::overload_cast<uint32_t>(&ValueInfo::entry, nb::const_), "i"_a)
        .def("entry", nb::overload_cast<const ValueInfo&>(&ValueInfo::entry, nb::const_), "i"_a)
        .def("field", &ValueInfo::field, "name"_a)
        // Vector operations
        .def("load_vector_entry", [](const ValueInfo& self, uint32_t i) {
            return self.load_vector_entry(i);
        }, "i"_a)
        .def("store_vector_entry", [](ValueInfo& self, uint32_t i, ValueInfo value) {
            return self.store_vector_entry(i, value);
        }, "i"_a, "value"_a)
        // Static factories
        .def_static("null", &ValueInfo::null)
        .def_static("mk_pointer", &ValueInfo::mk_pointer, "type"_a)
        .def_static("mk_array", &ValueInfo::mk_array, "type"_a)
        .def_static("mk_struct", &ValueInfo::mk_struct, "type"_a)
        .def_static("calc_struct_size", &ValueInfo::calc_struct_size, "type"_a)
        .def_static("calc_struct_field_count", &ValueInfo::calc_struct_field_count, "type"_a)
        .def_static("calc_struct_field_offset", &ValueInfo::calc_struct_field_offset, "type"_a, "idx"_a)
        // Template specializations for from_constant
        .def_static("from_bool", &ValueInfo::from_constant<bool>, "v"_a)
        .def_static("from_int8", &ValueInfo::from_constant<int8_t>, "v"_a)
        .def_static("from_int16", &ValueInfo::from_constant<int16_t>, "v"_a)
        .def_static("from_int32", &ValueInfo::from_constant<int32_t>, "v"_a)
        .def_static("from_int64", &ValueInfo::from_constant<int64_t>, "v"_a)
        .def_static("from_uint8", &ValueInfo::from_constant<uint8_t>, "v"_a)
        .def_static("from_uint16", &ValueInfo::from_constant<uint16_t>, "v"_a)
        .def_static("from_uint32", &ValueInfo::from_constant<uint32_t>, "v"_a)
        .def_static("from_uint64", &ValueInfo::from_constant<uint64_t>, "v"_a)
        .def_static("from_float32", &ValueInfo::from_constant<float>, "v"_a)
        .def_static("from_float64", &ValueInfo::from_constant<double>, "v"_a);
}

//
// Function bindings
//
void bind_functions(nb::module_& m) {
    // FnContext
    nb::class_<FnContext>(m, "FnContext")
        .def(nb::init<>())
        .def(nb::init<const TypeInfo&>(), "type"_a)
        .def("type", &FnContext::type)
        .def("value", &FnContext::value)
        .def("is_valid", &FnContext::is_valid)
        .def("is_init", &FnContext::is_init)
        .def("__eq__", &FnContext::operator==)
        .def_static("null", &FnContext::null, nb::rv_policy::reference);

    // Forward declare Function for CodeSection
    nb::class_<Function> function_class(m, "Function");

    // CodeSection
    nb::class_<CodeSection>(m, "CodeSection")
        .def(nb::init<>())
        .def("name", &CodeSection::name)
        .def("is_valid", &CodeSection::is_valid)
        .def("is_open", &CodeSection::is_open)
        .def("is_commit", &CodeSection::is_commit)
        .def("is_sealed", &CodeSection::is_sealed)
        .def("enter", &CodeSection::enter)
        .def("detach", &CodeSection::detach)
        .def("function", &CodeSection::function)
        .def("jump_to_section", &CodeSection::jump_to_section, "dst"_a)
        .def("conditional_jump", &CodeSection::conditional_jump, "value"_a, "then_dst"_a, "else_dst"_a)
        .def("__eq__", &CodeSection::operator==)
        .def_static("null", &CodeSection::null);

    // CodeSectionContext (static methods only)
    nb::class_<CodeSectionContext>(m, "CodeSectionContext")
        .def_static("push_var_context", &CodeSectionContext::push_var_context)
        .def_static("pop_var_context", &CodeSectionContext::pop_var_context)
        .def_static("pop", &CodeSectionContext::pop, "name"_a)
        .def_static("push", &CodeSectionContext::push, "name"_a, "v"_a)
        .def_static("mk_ptr", &CodeSectionContext::mk_ptr, "name"_a, "type"_a, "default_value"_a)
        .def_static("function", &CodeSectionContext::function)
        .def_static("set_return_value", &CodeSectionContext::set_return_value, "value"_a)
        .def_static("mk_section", &CodeSectionContext::mk_section, "name"_a)
        .def_static("jump_to_section", &CodeSectionContext::jump_to_section, "dst"_a)
        .def_static("section_break", &CodeSectionContext::section_break, "new_section_name"_a)
        .def_static("current_section", &CodeSectionContext::current_section)
        .def_static("is_current_section", &CodeSectionContext::is_current_section, "code"_a)
        .def_static("current_function", &CodeSectionContext::current_function)
        .def_static("current_context", &CodeSectionContext::current_context)
        .def_static("clean_sealed_context", &CodeSectionContext::clean_sealed_context)
        .def_static("assert_no_context", &CodeSectionContext::assert_no_context);

    // Function - complete the definition
    function_class
        .def(nb::init<>())
        .def("is_valid", &Function::is_valid)
        .def("parent_module", &Function::parent_module, nb::rv_policy::reference)
        .def("name", &Function::name)
        .def("is_external", &Function::is_external)
        .def("return_type", &Function::return_type)
        .def("call_fn", &Function::call_fn, "context"_a)
        .def("declare_fn", &Function::declare_fn, "src_mod"_a, "dst_mod"_a)
        .def("mk_section", &Function::mk_section, "name"_a)
        .def("verify", &Function::verify)
        .def("remove_from_module", &Function::remove_from_module)
        .def("write_to_ostream", &Function::write_to_ostream)
        .def("__eq__", &Function::operator==)
        .def_static("null", &Function::null);

    // FunctionBuilder
    nb::class_<FunctionBuilder>(m, "FunctionBuilder")
        .def(nb::init<>())
        .def("is_frozen", &FunctionBuilder::is_frozen)
        .def("set_context", &FunctionBuilder::set_context, "context"_a, nb::rv_policy::reference)
        .def("set_name", &FunctionBuilder::set_name, "name"_a, nb::rv_policy::reference)
        .def("set_module", &FunctionBuilder::set_module, "mod"_a, nb::rv_policy::reference)
        .def("mark_external", &FunctionBuilder::mark_external, nb::rv_policy::reference)
        .def("compile", &FunctionBuilder::compile)
        .def("__eq__", &FunctionBuilder::operator==)
        .def_static("null", &FunctionBuilder::null);
}

//
// Module bindings
//
void bind_modules(nb::module_& m) {
    // Module
    nb::class_<Module>(m, "Module")
        .def(nb::init<>())
        .def("is_init", &Module::is_init)
        .def("name", &Module::name)
        .def("public_symbols", &Module::public_symbols)
        .def("struct_type", &Module::struct_type, "name"_a)
        .def("exported_symbol_names", &Module::exported_symbol_names)
        .def("public_symbol_name", &Module::public_symbol_name, "symbol"_a)
        .def("contains", &Module::contains, "symbol"_a)
        .def("register_symbol", &Module::register_symbol, "link_symbol"_a)
        .def("init_standard", &Module::init_standard)
        .def("get_function", &Module::get_function, "name"_a)
        .def("add_struct_definition", &Module::add_struct_definition, "struct_type"_a)
        .def("write_to_file", &Module::write_to_file)
        .def("write_llvm_output", &Module::write_llvm_output, "suffix"_a = "")
        .def("write_to_ostream", &Module::write_to_ostream)
        .def("__eq__", &Module::operator==)
        .def_static("null", &Module::null);

    // Cursor
    nb::class_<Cursor>(m, "Cursor")
        .def(nb::init<>())
        .def(nb::init<const std::string&>(), "name"_a)
        .def("name", &Cursor::name)
        .def("main_module", &Cursor::main_module)
        .def("gen_module", &Cursor::gen_module)
        .def("is_bind_called", &Cursor::is_bind_called)
        .def("bind", &Cursor::bind)
        .def("cleanup", &Cursor::cleanup)
        .def("__eq__", &Cursor::operator==)
        .def_static("null", &Cursor::null);
}

//
// Control flow bindings
//
void bind_control_flow(nb::module_& m) {
    // IfElseCond::BranchSection
    nb::class_<IfElseCond::BranchSection>(m, "BranchSection")
        .def("is_open", &IfElseCond::BranchSection::is_open)
        .def("is_sealed", &IfElseCond::BranchSection::is_sealed)
        .def("enter", &IfElseCond::BranchSection::enter)
        .def("exit", &IfElseCond::BranchSection::exit)
        .def("define_branch", &IfElseCond::BranchSection::define_branch, "fn"_a)
        .def("code_section", &IfElseCond::BranchSection::code_section);

    // IfElseCond
    nb::class_<IfElseCond>(m, "IfElseCond")
        .def(nb::init<const std::string&, const ValueInfo&>(), "name"_a, "value"_a)
        .def("is_sealed", &IfElseCond::is_sealed)
        .def("then_branch", nb::overload_cast<IfElseCond::br_fn_t&&>(&IfElseCond::then_branch), "fn"_a)
        .def("else_branch", nb::overload_cast<IfElseCond::br_fn_t&&>(&IfElseCond::else_branch), "fn"_a)
        .def("get_then_branch", nb::overload_cast<>(&IfElseCond::then_branch), nb::rv_policy::reference)
        .def("get_else_branch", nb::overload_cast<>(&IfElseCond::else_branch), nb::rv_policy::reference)
        .def("bind", &IfElseCond::bind);
}

//
// JIT bindings
//
void bind_jit(nb::module_& m) {
    // runtime::Field
    nb::class_<runtime::Field>(m, "RuntimeField")
        .def(nb::init<>())
        .def("struct_def", &runtime::Field::struct_def, nb::rv_policy::reference)
        .def("idx", &runtime::Field::idx)
        .def("offset", &runtime::Field::offset)
        .def("name", &runtime::Field::name)
        .def("is_bool", &runtime::Field::is_bool)
        .def("is_struct_pointer", &runtime::Field::is_struct_pointer)
        .def("is_array_pointer", &runtime::Field::is_array_pointer)
        .def("is_int8", &runtime::Field::is_int8)
        .def("is_int16", &runtime::Field::is_int16)
        .def("is_int32", &runtime::Field::is_int32)
        .def("is_int64", &runtime::Field::is_int64)
        .def("is_uint8", &runtime::Field::is_uint8)
        .def("is_uint16", &runtime::Field::is_uint16)
        .def("is_uint32", &runtime::Field::is_uint32)
        .def("is_uint64", &runtime::Field::is_uint64)
        .def("is_float32", &runtime::Field::is_float32)
        .def("is_float64", &runtime::Field::is_float64)
        .def("__eq__", &runtime::Field::operator==)
        .def_static("null", &runtime::Field::null, nb::rv_policy::reference)
        .def_static("get_type", &runtime::Field::get_type, "type"_a)
        .def_static("get_raw_size", &runtime::Field::get_raw_size, "type"_a);

    // runtime::Struct
    nb::class_<runtime::Struct>(m, "RuntimeStruct")
        .def(nb::init<>())
        .def("name", &runtime::Struct::name)
        .def("size_in_bytes", &runtime::Struct::size_in_bytes)
        .def("num_fields", &runtime::Struct::num_fields)
        .def("field_names", &runtime::Struct::field_names)
        .def("mk_object", &runtime::Struct::mk_object)
        .def("__getitem__", &runtime::Struct::operator[], "name"_a)
        .def("__eq__", &runtime::Struct::operator==)
        .def_static("null", &runtime::Struct::null, nb::rv_policy::reference);

    // runtime::Array
    // Note: bool and float types are not instantiated in the library for arrays
    nb::class_<runtime::Array>(m, "RuntimeArray")
        .def(nb::init<>())
        .def("is_scalar", &runtime::Array::is_scalar)
        .def("is_pointer", &runtime::Array::is_pointer)
        .def("is_frozen", &runtime::Array::is_frozen)
        .def("try_freeze", &runtime::Array::try_freeze)
        .def("num_elements", &runtime::Array::num_elements)
        .def("element_type", &runtime::Array::element_type)
        .def("element_size", &runtime::Array::element_size)
        // Template specializations for get/set (only types instantiated in library)
        .def("get_int8", &runtime::Array::get<int8_t>, "i"_a)
        .def("get_int16", &runtime::Array::get<int16_t>, "i"_a)
        .def("get_int32", &runtime::Array::get<int32_t>, "i"_a)
        .def("get_int64", &runtime::Array::get<int64_t>, "i"_a)
        .def("get_uint8", &runtime::Array::get<uint8_t>, "i"_a)
        .def("get_uint16", &runtime::Array::get<uint16_t>, "i"_a)
        .def("get_uint32", &runtime::Array::get<uint32_t>, "i"_a)
        .def("get_uint64", &runtime::Array::get<uint64_t>, "i"_a)
        .def("set_int8", &runtime::Array::set<int8_t>, "i"_a, "v"_a)
        .def("set_int16", &runtime::Array::set<int16_t>, "i"_a, "v"_a)
        .def("set_int32", &runtime::Array::set<int32_t>, "i"_a, "v"_a)
        .def("set_int64", &runtime::Array::set<int64_t>, "i"_a, "v"_a)
        .def("set_uint8", &runtime::Array::set<uint8_t>, "i"_a, "v"_a)
        .def("set_uint16", &runtime::Array::set<uint16_t>, "i"_a, "v"_a)
        .def("set_uint32", &runtime::Array::set<uint32_t>, "i"_a, "v"_a)
        .def("set_uint64", &runtime::Array::set<uint64_t>, "i"_a, "v"_a)
        .def("get_object", &runtime::Array::get_object, "i"_a)
        .def("set_object", &runtime::Array::set_object, "i"_a, "v"_a)
        .def("get_array", &runtime::Array::get_array, "i"_a)
        .def("set_array", &runtime::Array::set_array, "i"_a, "v"_a)
        .def("__eq__", &runtime::Array::operator==)
        .def_static("null", &runtime::Array::null, nb::rv_policy::reference)
        .def_static("from", &runtime::Array::from, "type"_a, "size"_a);

    // runtime::Object
    nb::class_<runtime::Object>(m, "RuntimeObject")
        .def(nb::init<>())
        .def("is_frozen", &runtime::Object::is_frozen)
        .def("try_freeze", &runtime::Object::try_freeze)
        .def("null_fields", &runtime::Object::null_fields)
        .def("is_instance_of", &runtime::Object::is_instance_of, "o"_a)
        .def("struct_def", &runtime::Object::struct_def, nb::rv_policy::reference)
        // Template specializations for get/set
        .def("get_bool", &runtime::Object::get<bool>, "name"_a)
        .def("get_int8", &runtime::Object::get<int8_t>, "name"_a)
        .def("get_int16", &runtime::Object::get<int16_t>, "name"_a)
        .def("get_int32", &runtime::Object::get<int32_t>, "name"_a)
        .def("get_int64", &runtime::Object::get<int64_t>, "name"_a)
        .def("get_uint8", &runtime::Object::get<uint8_t>, "name"_a)
        .def("get_uint16", &runtime::Object::get<uint16_t>, "name"_a)
        .def("get_uint32", &runtime::Object::get<uint32_t>, "name"_a)
        .def("get_uint64", &runtime::Object::get<uint64_t>, "name"_a)
        .def("get_float32", &runtime::Object::get<float>, "name"_a)
        .def("get_float64", &runtime::Object::get<double>, "name"_a)
        .def("set_bool", &runtime::Object::set<bool>, "name"_a, "v"_a)
        .def("set_int8", &runtime::Object::set<int8_t>, "name"_a, "v"_a)
        .def("set_int16", &runtime::Object::set<int16_t>, "name"_a, "v"_a)
        .def("set_int32", &runtime::Object::set<int32_t>, "name"_a, "v"_a)
        .def("set_int64", &runtime::Object::set<int64_t>, "name"_a, "v"_a)
        .def("set_uint8", &runtime::Object::set<uint8_t>, "name"_a, "v"_a)
        .def("set_uint16", &runtime::Object::set<uint16_t>, "name"_a, "v"_a)
        .def("set_uint32", &runtime::Object::set<uint32_t>, "name"_a, "v"_a)
        .def("set_uint64", &runtime::Object::set<uint64_t>, "name"_a, "v"_a)
        .def("set_float32", &runtime::Object::set<float>, "name"_a, "v"_a)
        .def("set_float64", &runtime::Object::set<double>, "name"_a, "v"_a)
        .def("get_object", &runtime::Object::get_object, "name"_a)
        .def("set_object", &runtime::Object::set_object, "name"_a, "v"_a)
        .def("get_array", &runtime::Object::get_array, "name"_a)
        .def("set_array", &runtime::Object::set_array, "name"_a, "v"_a)
        .def("__eq__", &runtime::Object::operator==)
        .def_static("null", &runtime::Object::null, nb::rv_policy::reference);

    // runtime::EventFn
    nb::class_<runtime::EventFn>(m, "RuntimeEventFn")
        .def(nb::init<>())
        .def("is_init", &runtime::EventFn::is_init)
        .def("init", &runtime::EventFn::init)
        .def("on_event", &runtime::EventFn::on_event, "o"_a)
        .def("__eq__", &runtime::EventFn::operator==)
        .def_static("null", &runtime::EventFn::null, nb::rv_policy::reference);

    // runtime::Namespace
    nb::class_<runtime::Namespace>(m, "RuntimeNamespace")
        .def(nb::init<>())
        .def("name", &runtime::Namespace::name)
        .def("is_bind", &runtime::Namespace::is_bind)
        .def("is_global", &runtime::Namespace::is_global)
        .def("bind", &runtime::Namespace::bind)
        .def("struct_info", &runtime::Namespace::struct_info, "name"_a)
        .def("event_fn_info", &runtime::Namespace::event_fn_info, "name"_a)
        .def("__eq__", &runtime::Namespace::operator==)
        .def_static("null", &runtime::Namespace::null, nb::rv_policy::reference);

    // JustInTimeRunner
    nb::class_<JustInTimeRunner>(m, "JustInTimeRunner")
        .def(nb::init<>())
        .def("bind", &JustInTimeRunner::bind)
        .def("is_bind", &JustInTimeRunner::is_bind)
        .def("contains_symbol_definition", &JustInTimeRunner::contains_symbol_definition, "name"_a)
        .def("process_module_fn", &JustInTimeRunner::process_module_fn, "fn"_a)
        .def("add_module", &JustInTimeRunner::add_module, "cursor"_a)
        .def("get_namespace", &JustInTimeRunner::get_namespace, "name"_a)
        .def("get_global_namespace", &JustInTimeRunner::get_global_namespace)
        .def("__eq__", &JustInTimeRunner::operator==)
        .def_static("null", &JustInTimeRunner::null, nb::rv_policy::reference);
}
