//===-- sanitizer_malloc_mac.inc --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains Mac-specific malloc interceptors and a custom zone
// implementation, which together replace the system allocator.
//
//===----------------------------------------------------------------------===//

#include "sanitizer_common/sanitizer_platform.h"
#if !SANITIZER_MAC
#error "This file should only be compiled on Darwin."
#endif

#include <AvailabilityMacros.h>
#include <CoreFoundation/CFBase.h>
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <sys/mman.h>

#include "interception/interception.h"
#include "sanitizer_common/sanitizer_mac.h"

// Similar code is used in Google Perftools,
// https://github.com/gperftools/gperftools.

static malloc_zone_t sanitizer_zone;

INTERCEPTOR(malloc_zone_t *, malloc_create_zone,
                             vm_size_t start_size, unsigned zone_flags) {
  COMMON_MALLOC_ENTER();
  uptr page_size = GetPageSizeCached();
  uptr allocated_size = RoundUpTo(sizeof(sanitizer_zone), page_size);
  COMMON_MALLOC_MEMALIGN(page_size, allocated_size);
  malloc_zone_t *new_zone = (malloc_zone_t *)p;
  internal_memcpy(new_zone, &sanitizer_zone, sizeof(sanitizer_zone));
  new_zone->zone_name = NULL;  // The name will be changed anyway.
  if (GetMacosVersion() >= MACOS_VERSION_LION) {
    // Prevent the client app from overwriting the zone contents.
    // Library functions that need to modify the zone will set PROT_WRITE on it.
    // This matches the behavior of malloc_create_zone() on OSX 10.7 and higher.
    mprotect(new_zone, allocated_size, PROT_READ);
  }
  return new_zone;
}

INTERCEPTOR(malloc_zone_t *, malloc_default_zone, void) {
  COMMON_MALLOC_ENTER();
  return &sanitizer_zone;
}

INTERCEPTOR(malloc_zone_t *, malloc_default_purgeable_zone, void) {
  // FIXME: ASan should support purgeable allocations.
  // https://github.com/google/sanitizers/issues/139
  COMMON_MALLOC_ENTER();
  return &sanitizer_zone;
}

INTERCEPTOR(void, malloc_make_purgeable, void *ptr) {
  // FIXME: ASan should support purgeable allocations. Ignoring them is fine
  // for now.
  COMMON_MALLOC_ENTER();
}

INTERCEPTOR(int, malloc_make_nonpurgeable, void *ptr) {
  // FIXME: ASan should support purgeable allocations. Ignoring them is fine
  // for now.
  COMMON_MALLOC_ENTER();
  // Must return 0 if the contents were not purged since the last call to
  // malloc_make_purgeable().
  return 0;
}

INTERCEPTOR(void, malloc_set_zone_name, malloc_zone_t *zone, const char *name) {
  COMMON_MALLOC_ENTER();
  // Allocate |sizeof(COMMON_MALLOC_ZONE_NAME "-") + internal_strlen(name)|
  // bytes.
  size_t buflen =
      sizeof(COMMON_MALLOC_ZONE_NAME "-") + (name ? internal_strlen(name) : 0);
  InternalScopedString new_name(buflen);
  if (name && zone->introspect == sanitizer_zone.introspect) {
    new_name.append(COMMON_MALLOC_ZONE_NAME "-%s", name);
    name = new_name.data();
  }

  // Call the system malloc's implementation for both external and our zones,
  // since that appropriately changes VM region protections on the zone.
  REAL(malloc_set_zone_name)(zone, name);
}

INTERCEPTOR(void *, malloc, size_t size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_MALLOC(size);
  return p;
}

INTERCEPTOR(void, free, void *ptr) {
  COMMON_MALLOC_ENTER();
  if (!ptr) return;
  COMMON_MALLOC_FREE(ptr);
}

INTERCEPTOR(void *, realloc, void *ptr, size_t size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_REALLOC(ptr, size);
  return p;
}

INTERCEPTOR(void *, calloc, size_t nmemb, size_t size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_CALLOC(nmemb, size);
  return p;
}

INTERCEPTOR(void *, valloc, size_t size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_VALLOC(size);
  return p;
}

INTERCEPTOR(size_t, malloc_good_size, size_t size) {
  COMMON_MALLOC_ENTER();
  return sanitizer_zone.introspect->good_size(&sanitizer_zone, size);
}

INTERCEPTOR(int, posix_memalign, void **memptr, size_t alignment, size_t size) {
  COMMON_MALLOC_ENTER();
  CHECK(memptr);
  COMMON_MALLOC_MEMALIGN(alignment, size);
  if (p) {
    *memptr = p;
    return 0;
  }
  return -1;
}

namespace {

// TODO(glider): the __sanitizer_mz_* functions should be united with the Linux
// wrappers, as they are basically copied from there.
extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
size_t __sanitizer_mz_size(malloc_zone_t* zone, const void* ptr) {
  COMMON_MALLOC_SIZE(ptr);
  return size;
}

extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void *__sanitizer_mz_malloc(malloc_zone_t *zone, uptr size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_MALLOC(size);
  return p;
}

extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void *__sanitizer_mz_calloc(malloc_zone_t *zone, size_t nmemb, size_t size) {
  if (UNLIKELY(!COMMON_MALLOC_SANITIZER_INITIALIZED)) {
    // Hack: dlsym calls calloc before REAL(calloc) is retrieved from dlsym.
    const size_t kCallocPoolSize = 1024;
    static uptr calloc_memory_for_dlsym[kCallocPoolSize];
    static size_t allocated;
    size_t size_in_words = ((nmemb * size) + kWordSize - 1) / kWordSize;
    void *mem = (void*)&calloc_memory_for_dlsym[allocated];
    allocated += size_in_words;
    CHECK(allocated < kCallocPoolSize);
    return mem;
  }
  COMMON_MALLOC_CALLOC(nmemb, size);
  return p;
}

extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void *__sanitizer_mz_valloc(malloc_zone_t *zone, size_t size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_VALLOC(size);
  return p;
}

// TODO(glider): the allocation callbacks need to be refactored.
extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_mz_free(malloc_zone_t *zone, void *ptr) {
  if (!ptr) return;
  COMMON_MALLOC_FREE(ptr);
}

#define GET_ZONE_FOR_PTR(ptr) \
  malloc_zone_t *zone_ptr = malloc_zone_from_ptr(ptr); \
  const char *zone_name = (zone_ptr == 0) ? 0 : zone_ptr->zone_name

extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void *__sanitizer_mz_realloc(malloc_zone_t *zone, void *ptr, size_t new_size) {
  if (!ptr) {
    COMMON_MALLOC_MALLOC(new_size);
    return p;
  } else {
    COMMON_MALLOC_SIZE(ptr);
    if (size) {
      COMMON_MALLOC_REALLOC(ptr, new_size);
      return p;
    } else {
      // We can't recover from reallocating an unknown address, because
      // this would require reading at most |new_size| bytes from
      // potentially unaccessible memory.
      GET_ZONE_FOR_PTR(ptr);
      COMMON_MALLOC_REPORT_UNKNOWN_REALLOC(ptr, zone_ptr, zone_name);
      return nullptr;
    }
  }
}

extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void __sanitizer_mz_destroy(malloc_zone_t* zone) {
  // A no-op -- we will not be destroyed!
  Report("__sanitizer_mz_destroy() called -- ignoring\n");
}

extern "C"
SANITIZER_INTERFACE_ATTRIBUTE
void *__sanitizer_mz_memalign(malloc_zone_t *zone, size_t align, size_t size) {
  COMMON_MALLOC_ENTER();
  COMMON_MALLOC_MEMALIGN(align, size);
  return p;
}

// This function is currently unused, and we build with .
#if 0
void __sanitizer_mz_free_definite_size(
    malloc_zone_t* zone, void *ptr, size_t size) {
  // TODO(glider): check that |size| is valid.
  UNIMPLEMENTED();
}
#endif

kern_return_t mi_enumerator(task_t task, void *,
                            unsigned type_mask, vm_address_t zone_address,
                            memory_reader_t reader,
                            vm_range_recorder_t recorder) {
  // Should enumerate all the pointers we have.  Seems like a lot of work.
  return KERN_FAILURE;
}

size_t mi_good_size(malloc_zone_t *zone, size_t size) {
  // I think it's always safe to return size, but we maybe could do better.
  return size;
}

boolean_t mi_check(malloc_zone_t *zone) {
  UNIMPLEMENTED();
}

void mi_print(malloc_zone_t *zone, boolean_t verbose) {
  UNIMPLEMENTED();
}

void mi_log(malloc_zone_t *zone, void *address) {
  // I don't think we support anything like this
}

void mi_force_lock(malloc_zone_t *zone) {
  COMMON_MALLOC_FORCE_LOCK();
}

void mi_force_unlock(malloc_zone_t *zone) {
  COMMON_MALLOC_FORCE_UNLOCK();
}

void mi_statistics(malloc_zone_t *zone, malloc_statistics_t *stats) {
  COMMON_MALLOC_FILL_STATS(zone, stats);
}

boolean_t mi_zone_locked(malloc_zone_t *zone) {
  // UNIMPLEMENTED();
  return false;
}

}  // unnamed namespace

namespace COMMON_MALLOC_NAMESPACE {

void ReplaceSystemMalloc() {
  static malloc_introspection_t sanitizer_zone_introspection;
  // Ok to use internal_memset, these places are not performance-critical.
  internal_memset(&sanitizer_zone_introspection, 0,
                  sizeof(sanitizer_zone_introspection));

  sanitizer_zone_introspection.enumerator = &mi_enumerator;
  sanitizer_zone_introspection.good_size = &mi_good_size;
  sanitizer_zone_introspection.check = &mi_check;
  sanitizer_zone_introspection.print = &mi_print;
  sanitizer_zone_introspection.log = &mi_log;
  sanitizer_zone_introspection.force_lock = &mi_force_lock;
  sanitizer_zone_introspection.force_unlock = &mi_force_unlock;
  sanitizer_zone_introspection.statistics = &mi_statistics;
  sanitizer_zone_introspection.zone_locked = &mi_zone_locked;

  internal_memset(&sanitizer_zone, 0, sizeof(malloc_zone_t));

  // Use version 6 for OSX >= 10.6.
  sanitizer_zone.version = 6;
  sanitizer_zone.zone_name = COMMON_MALLOC_ZONE_NAME;
  sanitizer_zone.size = &__sanitizer_mz_size;
  sanitizer_zone.malloc = &__sanitizer_mz_malloc;
  sanitizer_zone.calloc = &__sanitizer_mz_calloc;
  sanitizer_zone.valloc = &__sanitizer_mz_valloc;
  sanitizer_zone.free = &__sanitizer_mz_free;
  sanitizer_zone.realloc = &__sanitizer_mz_realloc;
  sanitizer_zone.destroy = &__sanitizer_mz_destroy;
  sanitizer_zone.batch_malloc = 0;
  sanitizer_zone.batch_free = 0;
  sanitizer_zone.free_definite_size = 0;
  sanitizer_zone.memalign = &__sanitizer_mz_memalign;
  sanitizer_zone.introspect = &sanitizer_zone_introspection;

  // Register the zone.
  malloc_zone_register(&sanitizer_zone);
}

}  // namespace COMMON_MALLOC_NAMESPACE
