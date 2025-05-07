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

#ifndef SHARE_GC_Z_ZCOLLECTEDHEAP_HPP
#define SHARE_GC_Z_ZCOLLECTEDHEAP_HPP

#include "gc/shared/collectedHeap.hpp"
#include "gc/shared/softRefPolicy.hpp"
#include "gc/z/zBarrierSet.hpp"
#include "gc/z/zCollectorPolicy.hpp"
#include "gc/z/zDirector.hpp"
#include "gc/z/zDriver.hpp"
#include "gc/z/zInitialize.hpp"
#include "gc/z/zHeap.hpp"
#include "gc/z/zRuntimeWorkers.hpp"
#include "gc/z/zStat.hpp"

class ZCollectedHeap : public CollectedHeap {
  friend class VMStructs;

private:
  ZCollectorPolicy* _collector_policy;
  // 软引用清除策略
  SoftRefPolicy     _soft_ref_policy;
  // 内存屏障, 用于解释 C1/C2 在并发下对象访问处理
  ZBarrierSet       _barrier_set;
  // 初始化地址映射视图初始信息, NUMA, CPU亲缘性, 内存屏障等
  ZInitialize       _initialize;
  // ZGC的堆对象, 对象的分配, 标记, 转移等都在该类中实现
  ZHeap             _heap;

  // 下面三个垃圾回收控制线程
  // 垃圾回收控制线程, 用于触发垃圾回收
  ZDirector*        _director;
  // 垃圾回收控制线程, 用于执行垃圾回收
  ZDriver*          _driver;
  // 垃圾回收控制线程, 用于输出ZGC的统计信息
  ZStat*            _stat;
  // 并发,并行工作线程
  ZRuntimeWorkers   _runtime_workers;
  // 对象快速分配
  virtual HeapWord* allocate_new_tlab(size_t min_size,
                                      size_t requested_size,
                                      size_t* actual_size);

public:
  static ZCollectedHeap* heap();

  using CollectedHeap::ensure_parsability;
  using CollectedHeap::accumulate_statistics_all_tlabs;
  using CollectedHeap::resize_all_tlabs;

  ZCollectedHeap(ZCollectorPolicy* policy);
  virtual Name kind() const;
  virtual const char* name() const;
  // 初始化zgc内存空间
  virtual jint initialize();
  virtual void initialize_serviceability();
  virtual void stop();

  virtual CollectorPolicy* collector_policy() const;
  virtual SoftRefPolicy* soft_ref_policy();

  virtual size_t max_capacity() const;
  virtual size_t capacity() const;
  virtual size_t used() const;

  virtual bool is_maximal_no_gc() const;
  virtual bool is_scavengable(oop obj);
  virtual bool is_in(const void* p) const;
  virtual bool is_in_closed_subset(const void* p) const;
  // 对象慢速分配
  virtual HeapWord* mem_allocate(size_t size, bool* gc_overhead_limit_was_exceeded);
  // 元数据分配失败的时候调用该函数, 该函数首先尝试异步垃圾回收之后是否可以分配元数据对象空间,
  // 如果不能, 尝试进行同步垃圾回收之后再次尝试分配内存空间,
  // 如果还是不能, 那么尝试扩展元数据空间, 再次分配成功之后则返回内存空间, 如果不成功则范围null
  virtual MetaWord* satisfy_failed_metadata_allocation(ClassLoaderData* loader_data,
                                                       size_t size,
                                                       Metaspace::MetadataType mdtype);
  // 垃圾回收
  virtual void collect(GCCause::Cause cause);
  virtual void collect_as_vm_thread(GCCause::Cause cause);
  virtual void do_full_collection(bool clear_all_soft_refs);

  virtual bool supports_tlab_allocation() const;
  virtual size_t tlab_capacity(Thread* thr) const;
  virtual size_t tlab_used(Thread* thr) const;
  virtual size_t max_tlab_size() const;
  virtual size_t unsafe_max_tlab_alloc(Thread* thr) const;

  virtual bool can_elide_tlab_store_barriers() const;
  virtual bool can_elide_initializing_store_barrier(oop new_obj);
  virtual bool card_mark_must_follow_store() const;

  virtual GrowableArray<GCMemoryManager*> memory_managers();
  virtual GrowableArray<MemoryPool*> memory_pools();

  // 对象遍历
  virtual void object_iterate(ObjectClosure* cl);
  virtual void safe_object_iterate(ObjectClosure* cl);

  virtual HeapWord* block_start(const void* addr) const;
  virtual size_t block_size(const HeapWord* addr) const;
  virtual bool block_is_obj(const HeapWord* addr) const;

  virtual void keep_alive(oop obj);
  // 注册c2后的编译方法
  virtual void register_nmethod(nmethod* nm);
  virtual void unregister_nmethod(nmethod* nm);
  virtual void verify_nmethod(nmethod* nmethod);

  virtual WorkGang* get_safepoint_workers();

  virtual jlong millis_since_last_gc();

  virtual void gc_threads_do(ThreadClosure* tc) const;

  virtual VirtualSpaceSummary create_heap_space_summary();

  virtual void print_on(outputStream* st) const;
  virtual void print_on_error(outputStream* st) const;
  virtual void print_extended_on(outputStream* st) const;
  virtual void print_gc_threads_on(outputStream* st) const;
  virtual void print_tracing_info() const;

  virtual void prepare_for_verify();
  virtual void verify(VerifyOption option /* ignored */);
  virtual bool is_oop(oop object) const;
};

#endif // SHARE_GC_Z_ZCOLLECTEDHEAP_HPP
