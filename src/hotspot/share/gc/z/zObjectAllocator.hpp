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

#ifndef SHARE_GC_Z_ZOBJECTALLOCATOR_HPP
#define SHARE_GC_Z_ZOBJECTALLOCATOR_HPP

#include "gc/z/zAllocationFlags.hpp"
#include "gc/z/zPage.hpp"
#include "gc/z/zValue.hpp"
#include "memory/allocation.hpp"

class ZObjectAllocator {
private:
  const uint         _nworkers;
  // 记录一下每个cpu已经分配的字节数
  ZPerCPU<size_t>    _used;
  // 页面指针/页面缓存
  ZContended<ZPage*> _shared_medium_page;
  // 每个数组的元素为页面指针, 用于应用程序请求分配对象空间的时候使用,
  // 这里要注意的是所有的应用程序都从这个缓存中分配对象, 但是为了加速对象的分配,
  // 按照cpu进行缓存
  ZPerCPU<ZPage*>    _shared_small_page;
  // 数组, 用于并行工作线程分配对象空间的时候使用,
  // 实现每个工作线程对应一个页面缓存, 这也是为了最大限度的减少并发/并行时的竞争
  ZPerWorker<ZPage*> _worker_small_page;

  ZPage* alloc_page(uint8_t type, size_t size, ZAllocationFlags flags);

  // Allocate an object in a shared page. Allocate and
  // atomically install a new page if necessary.
  uintptr_t alloc_object_in_shared_page(ZPage** shared_page,
                                        uint8_t page_type,
                                        size_t page_size,
                                        size_t size,
                                        ZAllocationFlags flags);

  uintptr_t alloc_large_object(size_t size, ZAllocationFlags flags);
  uintptr_t alloc_medium_object(size_t size, ZAllocationFlags flags);
  uintptr_t alloc_small_object_from_nonworker(size_t size, ZAllocationFlags flags);
  uintptr_t alloc_small_object_from_worker(size_t size, ZAllocationFlags flags);
  uintptr_t alloc_small_object(size_t size, ZAllocationFlags flags);
  uintptr_t alloc_object(size_t size, ZAllocationFlags flags);

  bool undo_alloc_large_object(ZPage* page);
  bool undo_alloc_medium_object(ZPage* page, uintptr_t addr, size_t size);
  bool undo_alloc_small_object_from_nonworker(ZPage* page, uintptr_t addr, size_t size);
  bool undo_alloc_small_object_from_worker(ZPage* page, uintptr_t addr, size_t size);
  bool undo_alloc_small_object(ZPage* page, uintptr_t addr, size_t size);
  bool undo_alloc_object(ZPage* page, uintptr_t addr, size_t size);

public:
  ZObjectAllocator(uint nworkers);

  uintptr_t alloc_object(size_t size);

  uintptr_t alloc_object_for_relocation(size_t size);
  void undo_alloc_object_for_relocation(ZPage* page, uintptr_t addr, size_t size);

  size_t used() const;
  size_t remaining() const;

  void retire_tlabs();
  void remap_tlabs();
};

#endif // SHARE_GC_Z_ZOBJECTALLOCATOR_HPP
