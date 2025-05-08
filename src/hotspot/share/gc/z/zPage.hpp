/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef SHARE_GC_Z_ZPAGE_HPP
#define SHARE_GC_Z_ZPAGE_HPP

#include "gc/z/zForwardingTable.hpp"
#include "gc/z/zList.hpp"
#include "gc/z/zLiveMap.hpp"
#include "gc/z/zPhysicalMemory.hpp"
#include "gc/z/zVirtualMemory.hpp"
#include "memory/allocation.hpp"

class ZPage : public CHeapObj<mtGC> {
  friend class VMStructs;
  friend class ZList<ZPage>;

private:
  // Always hot
  // 页面类型
  const uint8_t        _type;             // Page type
  // 页面状态, 在垃圾回收的时候会涉及对象转移, 会把对象所在页面设置为pinned, 用于在垃圾回收过程中对象的重定位, 如果页面为pinned
  // 则转移后的对象还在该页面中
  volatile uint8_t     _pinned;           // Pinned flag
  // 页面管理的numa节点id
  uint8_t              _numa_id;          // NUMA node affinity
  // 分配序列号,
  uint32_t             _seqnum;           // Allocation sequence number
  const ZVirtualMemory _virtual;          // Virtual start/end address
  volatile uintptr_t   _top;              // Virtual top address
  // 对象活跃信息表
  ZLiveMap             _livemap;          // Live map


  // Hot when relocated and cached
  // 记录对页面中活跃对象的个数,大小等
  volatile uint32_t    _refcount;         // Page reference count
  // 对象转地址信息表, 存储垃圾回收过程中对象转移之后的地址
  ZForwardingTable     _forwarding;       // Forwarding table
  ZPhysicalMemory      _physical;         // Physical memory for page
  ZListNode<ZPage>     _node;             // Page list node

  const char* type_to_string() const;
  uint32_t object_max_count() const;
  uintptr_t relocate_object_inner(uintptr_t from_index, uintptr_t from_offset);

  bool is_object_marked(uintptr_t addr) const;
  bool is_object_strongly_marked(uintptr_t addr) const;

public:
  ZPage(uint8_t type, ZVirtualMemory vmem, ZPhysicalMemory pmem);
  ~ZPage();

  size_t object_alignment_shift() const;
  size_t object_alignment() const;

  uint8_t type() const;
  uintptr_t start() const;
  uintptr_t end() const;
  size_t size() const;
  uintptr_t top() const;
  size_t remaining() const;

  uint8_t numa_id();

  ZPhysicalMemory& physical_memory();
  const ZVirtualMemory& virtual_memory() const;

  void reset();

  bool inc_refcount();
  bool dec_refcount();

  bool is_in(uintptr_t addr) const;

  uintptr_t block_start(uintptr_t addr) const;
  size_t block_size(uintptr_t addr) const;
  bool block_is_obj(uintptr_t addr) const;
  // 页面活跃, 该状态下页面不能被回收
  bool is_active() const;
  // 页面分配
  bool is_allocating() const;
  // 页面可转移
  bool is_relocatable() const;
  // 页面可回收
  bool is_detached() const;

  // 页面已映射
  bool is_mapped() const;
  void set_pre_mapped();
  // 页面被钉住
  bool is_pinned() const;
  void set_pinned();

  bool is_forwarding() const;
  void set_forwarding();
  void reset_forwarding();
  void verify_forwarding() const;

  bool is_marked() const;
  bool is_object_live(uintptr_t addr) const;
  bool is_object_strongly_live(uintptr_t addr) const;
  // 标记活跃对象的状态
  bool mark_object(uintptr_t addr, bool finalizable, bool& inc_live);

  void inc_live_atomic(uint32_t objects, size_t bytes);
  size_t live_bytes() const;

  void object_iterate(ObjectClosure* cl);

  // 从页面中分配对象空间
  uintptr_t alloc_object(size_t size);
  uintptr_t alloc_object_atomic(size_t size);

  bool undo_alloc_object(uintptr_t addr, size_t size);
  bool undo_alloc_object_atomic(uintptr_t addr, size_t size);
  // 将对象从一个页面复制到另一个页面
  uintptr_t relocate_object(uintptr_t from);
  // 对象重定位
  uintptr_t forward_object(uintptr_t from);

  void print_on(outputStream* out) const;
  void print() const;
};

#endif // SHARE_GC_Z_ZPAGE_HPP
