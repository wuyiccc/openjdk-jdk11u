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

#ifndef SHARE_GC_Z_ZPAGEALLOCATOR_HPP
#define SHARE_GC_Z_ZPAGEALLOCATOR_HPP

#include "gc/z/zAllocationFlags.hpp"
#include "gc/z/zList.hpp"
#include "gc/z/zLock.hpp"
#include "gc/z/zPageCache.hpp"
#include "gc/z/zPhysicalMemory.hpp"
#include "gc/z/zPreMappedMemory.hpp"
#include "gc/z/zVirtualMemory.hpp"
#include "memory/allocation.hpp"

class ZPageAllocRequest;

class ZPageAllocator {
  friend class VMStructs;

private:
  // 锁变量, 在进行页面分配, 释放等处理时需要加锁, 保证只有一个线程能够操作
  ZLock                    _lock;
  // 虚拟机内存管理器, 在进行页面分配, 释放时申请和释放虚拟内存
  ZVirtualMemoryManager    _virtual;
  // 物理内存管理器, 在进行页面分配, 释放的时候申请和释放物理内存
  ZPhysicalMemoryManager   _physical;
  // 页面缓存器, 在进行页面分配的时候优先从页面缓存中申请, 在页面释放的时候加入页面缓存
  ZPageCache               _cache;
  const size_t             _max_reserve;
  // 预分配内存管理器, 在进行页面分配的时候, 不能从缓存中申请页面时候, 从预分配内存中分配
  ZPreMappedMemory         _pre_mapped;
  size_t                   _used_high;
  size_t                   _used_low;
  size_t                   _used;
  size_t                   _allocated;
  ssize_t                  _reclaimed;
  // 同步阻塞页面分配的请求队列
  ZList<ZPageAllocRequest> _queue;
  // 可回收的页面, 在进行页面分配的时候, 发现内存不足时把可以回收的缓存页面或者预分配内存页面加入可回收页面供后续页面回收
  ZList<ZPage>             _detached;

  static ZPage* const      gc_marker;

  void increase_used(size_t size, bool relocation);
  void decrease_used(size_t size, bool reclaimed);

  size_t max_available(bool no_reserve) const;
  size_t try_ensure_unused(size_t size, bool no_reserve);
  size_t try_ensure_unused_for_pre_mapped(size_t size);

  ZPage* create_page(uint8_t type, size_t size);
  // 把新分配的内存映射到操作系统的物理内存上
  void map_page(ZPage* page);
  // 在页面分配过程中发现内存不足, 把可以回收的页面进行回收
  void detach_page(ZPage* page);
  void flush_pre_mapped();
  void flush_cache(size_t size);

  void check_out_of_memory_during_initialization();

  ZPage* alloc_page_common_inner(uint8_t type, size_t size, ZAllocationFlags flags);
  ZPage* alloc_page_common(uint8_t type, size_t size, ZAllocationFlags flags);
  ZPage* alloc_page_blocking(uint8_t type, size_t size, ZAllocationFlags flags);
  ZPage* alloc_page_nonblocking(uint8_t type, size_t size, ZAllocationFlags flags);

  void satisfy_alloc_queue();

  void detach_memory(const ZVirtualMemory& vmem, ZPhysicalMemory& pmem);

public:
  ZPageAllocator(size_t min_capacity, size_t max_capacity, size_t max_reserve);

  bool is_initialized() const;

  size_t max_capacity() const;
  size_t current_max_capacity() const;
  size_t capacity() const;
  size_t max_reserve() const;
  size_t used_high() const;
  size_t used_low() const;
  size_t used() const;
  size_t allocated() const;
  size_t reclaimed() const;

  void reset_statistics();
  // 分配页面
  ZPage* alloc_page(uint8_t type, size_t size, ZAllocationFlags flags);
  void flip_page(ZPage* page);
  // 释放页面
  void free_page(ZPage* page, bool reclaimed);
  void destroy_page(ZPage* page);

  void flush_detached_pages(ZList<ZPage>* list);

  void flip_pre_mapped();

  bool is_alloc_stalled() const;
  void check_out_of_memory();
};

#endif // SHARE_GC_Z_ZPAGEALLOCATOR_HPP
