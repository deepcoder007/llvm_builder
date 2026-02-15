"""
llvm_builder - Python bindings for a high-level LLVM IR builder library.

This module provides a Pythonic interface to create, compile, and execute
LLVM IR code using JIT compilation.

Example usage:
    >>> import llvm_builder as llvm
    >>> cursor = llvm.Cursor("my_module")
    >>> module = cursor.main_module()
    >>> # Define a struct
    >>> point_type = llvm.struct("Point", [
    ...     ("x", llvm.int32),
    ...     ("y", llvm.int32),
    ... ])
    >>> module.add_struct_definition(point_type)
"""

from .llvm_builder_py import (
    # Enums
    RuntimeType,
    SymbolType,
    # Type system
    TypeInfo,
    MemberFieldEntry,
    FieldEntry,
    LinkSymbol,
    LinkSymbolName,
    # Values
    ValueInfo,
    TagInfo,
    # Functions
    Function,
    FnContext,
    CodeSection,
    CodeSectionContext,
    # Modules
    Module,
    Cursor,
    # Control flow
    IfElseCond,
    BranchSection,
    # JIT
    JustInTimeRunner,
    RuntimeNamespace,
    RuntimeStruct,
    RuntimeObject,
    RuntimeArray,
    RuntimeField,
    RuntimeEventFn,
)

__version__ = "1.0.0"
__all__ = [
    # Enums
    "RuntimeType",
    "SymbolType",
    # Type system
    "TypeInfo",
    "MemberFieldEntry",
    "FieldEntry",
    "LinkSymbol",
    "LinkSymbolName",
    # Values
    "ValueInfo",
    "TagInfo",
    # Functions
    "Function",
    "FnContext",
    "CodeSection",
    "CodeSectionContext",
    # Modules
    "Module",
    "Cursor",
    # Control flow
    "IfElseCond",
    "BranchSection",
    # JIT
    "JustInTimeRunner",
    "RuntimeNamespace",
    "RuntimeStruct",
    "RuntimeObject",
    "RuntimeArray",
    "RuntimeField",
    "RuntimeEventFn",
    # Convenience functions
    "void",
    "bool_",
    "int8",
    "int16",
    "int32",
    "int64",
    "uint8",
    "uint16",
    "uint32",
    "uint64",
    "float32",
    "float64",
    "array",
    "vector",
    "struct",
    "const",
]

# Convenience type aliases
void = TypeInfo.mk_void
bool_ = TypeInfo.mk_bool
int8 = TypeInfo.mk_int8
int16 = TypeInfo.mk_int16
int32 = TypeInfo.mk_int32
int64 = TypeInfo.mk_int64
uint8 = TypeInfo.mk_uint8
uint16 = TypeInfo.mk_uint16
uint32 = TypeInfo.mk_uint32
uint64 = TypeInfo.mk_uint64
float32 = TypeInfo.mk_float32
float64 = TypeInfo.mk_float64
array = TypeInfo.mk_array
vector = TypeInfo.mk_vector


def struct(name: str, fields: list, packed: bool = True) -> TypeInfo:
    """
    Create a struct type with the given fields.

    Args:
        name: The name of the struct type
        fields: List of (name, type) tuples or MemberFieldEntry objects
        packed: Whether the struct should be packed (default: True)

    Returns:
        TypeInfo representing the struct type

    Example:
        >>> point_type = struct("Point", [
        ...     ("x", int32()),
        ...     ("y", int32()),
        ... ])
    """
    entries = []
    for field in fields:
        if isinstance(field, MemberFieldEntry):
            entries.append(field)
        elif isinstance(field, tuple):
            if len(field) == 2:
                name_, type_ = field
                entries.append(MemberFieldEntry(name_, type_))
            elif len(field) == 3:
                name_, type_, readonly = field
                entries.append(MemberFieldEntry(name_, type_, readonly))
            else:
                raise ValueError(f"Invalid field specification: {field}")
        else:
            raise TypeError(f"Expected tuple or MemberFieldEntry, got {type(field)}")
    return TypeInfo.mk_struct(name, entries, packed)


def const(value) -> ValueInfo:
    """
    Create a constant ValueInfo from a Python value.

    Args:
        value: A Python bool, int, or float

    Returns:
        ValueInfo representing the constant

    Example:
        >>> x = const(42)  # int32 constant
        >>> y = const(3.14)  # float64 constant
        >>> z = const(True)  # bool constant
    """
    if isinstance(value, bool):
        return ValueInfo.from_bool(value)
    elif isinstance(value, int):
        # Choose appropriate int type based on value range
        if -128 <= value <= 127:
            return ValueInfo.from_int8(value)
        elif -32768 <= value <= 32767:
            return ValueInfo.from_int16(value)
        elif -2147483648 <= value <= 2147483647:
            return ValueInfo.from_int32(value)
        else:
            return ValueInfo.from_int64(value)
    elif isinstance(value, float):
        return ValueInfo.from_float64(value)
    else:
        raise TypeError(f"Cannot create constant from {type(value)}")
