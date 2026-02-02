//
// Created by vibhanshu on 2024-06-07
//

#ifndef CORE_META_NONCOPYABLE_H
#define CORE_META_NONCOPYABLE_H

#include "core/util/defines.h"

namespace CORE_NS()::meta {

class noncopyable {
  protected:
    noncopyable() = default;

  private:
    noncopyable(const noncopyable&) = delete;
    noncopyable(noncopyable&&) = delete;
    void operator=(const noncopyable&) = delete;
    void operator=(noncopyable&&) = delete;
};

class only_moveable {
  protected:
    only_moveable() = default;
    only_moveable(only_moveable&&) = default;
    only_moveable& operator=(only_moveable&&) = default;

  private:
    only_moveable(const only_moveable&) = delete;
    void operator=(const only_moveable&) = delete;
};

class only_move_construct {
  protected:
    only_move_construct() = default;
    only_move_construct(only_move_construct&&) = default;

  private:
    only_move_construct(const only_move_construct&) = delete;
    only_move_construct& operator=(only_move_construct&&) = delete;
    void operator=(const only_move_construct&) = delete;
};

class non_assignable {
  protected:
    non_assignable() = default;
    non_assignable(const non_assignable&) = default;
    non_assignable(non_assignable&&) = default;

  private:
    non_assignable& operator=(non_assignable&&) = delete;
    non_assignable& operator=(const non_assignable&) = delete;
};

}


#endif
