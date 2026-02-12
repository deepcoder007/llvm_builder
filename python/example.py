#!/usr/bin/env python3
"""
Example usage of llvm_builder Python bindings.

This example demonstrates how to:
1. Create a cursor and module
2. Define a struct type
3. Create a function that operates on the struct
4. JIT compile and execute the function
"""

import llvm_builder_py as llvm


def main():
    # Create a cursor (entry point to LLVM context)
    cursor = llvm.Cursor("example")

    # Get the main module
    module = cursor.main_module()

    # Create a struct type: Point { x: int32, y: int32 }
    point_fields = [
        llvm.MemberFieldEntry("x", llvm.TypeInfo.mk_int32()),
        llvm.MemberFieldEntry("y", llvm.TypeInfo.mk_int32()),
    ]
    point_type = llvm.TypeInfo.mk_struct("Point", point_fields)
    module.add_struct_definition(point_type)

    # Create a function context with pointer to Point as parameter
    ctx_type = point_type.pointer_type()
    fn_context = llvm.FnContext(ctx_type)

    # Build a function: int32 add_coords(Point* p) { return p->x + p->y; }
    builder = llvm.FunctionBuilder()
    fn = (builder
          .set_name("add_coords")
          .set_context(fn_context)
          .set_module(module)
          .compile())

    # Create the function body
    body = fn.mk_section("body")
    body.enter()

    # Get the function parameter
    param = CodeSectionContext.current_context()

    # Access fields: p->x and p->y
    x_ptr = param.field("x")
    y_ptr = param.field("y")

    # Load the values
    x_val = x_ptr.load("x_val")
    y_val = y_ptr.load("y_val")

    # Add them
    result = x_val + y_val

    # Set return value
    llvm.CodeSectionContext.set_return_value(result)

    # Verify and finalize
    fn.verify()
    cursor.bind()

    # Print the generated LLVM IR
    print("Generated LLVM IR:")
    fn.write_to_ostream()

    # JIT compile and execute
    jit = llvm.JustInTimeRunner()
    jit.add_module(cursor)
    jit.bind()

    # Get the runtime namespace and struct info
    ns = jit.get_global_namespace()
    point_struct = ns.struct_info("Point")

    # Create a Point object
    point = point_struct.mk_object()
    point.set_int32("x", 10)
    point.set_int32("y", 20)

    # Get and call the function
    add_fn = ns.event_fn_info("add_coords")
    add_fn.init()
    result = add_fn.on_event(point)

    print(f"\nadd_coords({{x=10, y=20}}) = {result}")
    assert result == 30, f"Expected 30, got {result}"
    print("Test passed!")


def arithmetic_example():
    """Example demonstrating arithmetic operations."""
    cursor = llvm.Cursor("arithmetic")
    module = cursor.main_module()

    # Create input struct
    input_fields = [
        llvm.MemberFieldEntry("a", llvm.TypeInfo.mk_float64()),
        llvm.MemberFieldEntry("b", llvm.TypeInfo.mk_float64()),
        llvm.MemberFieldEntry("result", llvm.TypeInfo.mk_float64()),
    ]
    input_type = llvm.TypeInfo.mk_struct("Input", input_fields)
    module.add_struct_definition(input_type)

    # Build function: void compute(Input* i) { i->result = i->a * i->b + i->a; }
    ctx = llvm.FnContext(input_type.pointer_type())
    fn = (llvm.FunctionBuilder()
          .set_name("compute")
          .set_context(ctx)
          .set_module(module)
          .compile())

    body = fn.mk_section("body")
    body.enter()

    param = CodeSectionContext.current_context()
    a = param.field("a").load()
    b = param.field("b").load()

    # result = a * b + a
    result = a * b + a
    param.field("result").store(result)

    # Return 0 (success)
    llvm.CodeSectionContext.set_return_value(llvm.ValueInfo.from_int32(0))

    fn.verify()
    cursor.bind()

    # JIT and run
    jit = llvm.JustInTimeRunner()
    jit.add_module(cursor)
    jit.bind()

    ns = jit.get_global_namespace()
    input_struct = ns.struct_info("Input")
    obj = input_struct.mk_object()
    obj.set_float64("a", 3.0)
    obj.set_float64("b", 4.0)
    obj.set_float64("result", 0.0)

    compute_fn = ns.event_fn_info("compute")
    compute_fn.init()
    compute_fn.on_event(obj)

    result = obj.get_float64("result")
    expected = 3.0 * 4.0 + 3.0  # 15.0
    print(f"\ncompute(a=3.0, b=4.0) -> result = {result}")
    assert abs(result - expected) < 0.0001, f"Expected {expected}, got {result}"
    print("Arithmetic test passed!")


def conditional_example():
    """Example with conditional branching."""
    cursor = llvm.Cursor("conditional")
    module = cursor.main_module()

    # Create struct with condition and values
    fields = [
        llvm.MemberFieldEntry("cond", llvm.TypeInfo.mk_bool()),
        llvm.MemberFieldEntry("a", llvm.TypeInfo.mk_int32()),
        llvm.MemberFieldEntry("b", llvm.TypeInfo.mk_int32()),
    ]
    data_type = llvm.TypeInfo.mk_struct("Data", fields)
    module.add_struct_definition(data_type)

    # Build function: int32 select(Data* d) { return d->cond ? d->a : d->b; }
    ctx = llvm.FnContext(data_type.pointer_type())
    fn = (llvm.FunctionBuilder()
          .set_name("select")
          .set_context(ctx)
          .set_module(module)
          .compile())

    body = fn.mk_section("body")
    body.enter()

    param = CodeSectionContext.current_context()
    cond = param.field("cond").load()
    a = param.field("a").load()
    b = param.field("b").load()

    # Use conditional select: cond ? a : b
    result = cond.cond(a, b, "select")

    llvm.CodeSectionContext.set_return_value(result)

    fn.verify()
    cursor.bind()

    # JIT and test
    jit = llvm.JustInTimeRunner()
    jit.add_module(cursor)
    jit.bind()

    ns = jit.get_global_namespace()
    data_struct = ns.struct_info("Data")

    # Test with cond=true
    obj1 = data_struct.mk_object()
    obj1.set_bool("cond", True)
    obj1.set_int32("a", 100)
    obj1.set_int32("b", 200)

    select_fn = ns.event_fn_info("select")
    select_fn.init()
    r1 = select_fn.on_event(obj1)
    print(f"\nselect(cond=true, a=100, b=200) = {r1}")
    assert r1 == 100

    # Test with cond=false
    obj2 = data_struct.mk_object()
    obj2.set_bool("cond", False)
    obj2.set_int32("a", 100)
    obj2.set_int32("b", 200)
    r2 = select_fn.on_event(obj2)
    print(f"select(cond=false, a=100, b=200) = {r2}")
    assert r2 == 200

    print("Conditional test passed!")


if __name__ == "__main__":
    main()
    arithmetic_example()
    conditional_example()
    print("\nAll examples completed successfully!")
