#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_ASSERT
#include <assert.h>
#define ARENA_ASSERT assert
#endif

#define ARENA_BACKEND_LIBC_MALLOC 0
#define ARENA_BACKEND_WIN32_VIRTUALALLOC 1

#ifndef ARENA_BACKEND
#define ARENA_BACKEND ARENA_BACKEND_LIBC_MALLOC
#endif // ARENA_BACKEND

typedef struct Region Region;

struct Region {
  Region *next;
  size_t count;
  size_t capacity;
  uintptr_t data[];
};

typedef struct {
  Region *begin, *end;
} Arena;

#define REGION_DEFAULT_CAPACITY (8 * 1024)

Region *new_region(size_t capacity);
void free_region(Region *r);

void *arena_alloc(Arena *a, size_t size_bytes);
void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz);

void arena_reset(Arena *a);
void arena_free(Arena *a);

#endif // ARENA_H

#ifdef ARENA_IMPLEMENTATION

#if ARENA_BACKEND == ARENA_BACKEND_LIBC_MALLOC
#include <stdlib.h>

Region *new_region(size_t capacity) {
  size_t size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
  Region *r = malloc(size_bytes);
  ARENA_ASSERT(r);
  r->next = NULL;
  r->count = 0;
  r->capacity = capacity;
  return r;
}

void free_region(Region *r) { free(r); }
#elif ARENA_BACKEND == ARENA_BACKEND_WIN32_VIRTUALALLOC

#if !defined(_WIN32)
#error "Current platform is not Windows"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define INV_HANDLE(x) (((x) == NULL) || ((x) == INVALID_HANDLE_VALUE))

Region *new_region(size_t capacity) {
  SIZE_T size_bytes = sizeof(Region) + sizeof(uintptr_t) * capacity;
  Region *r = VirtualAllocEx(GetCurrentProcess(), NULL, size_bytes,
                             MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (INV_HANDLE(r)) ARENA_ASSERT(0 && "VirtualAllocEx() failed.");

  r->next = NULL;
  r->count = 0;
  r->capacity = capacity;
  return r;
}

void free_region(Region *r) {
  if (INV_HANDLE(r)) return;

  BOOL free_result =
      VirtualFreeEx(GetCurrentProcess(), (LPVOID)r, 0, MEM_RELEASE);

  if (FALSE == free_result) ARENA_ASSERT(0 && "VirtualFreeEx() failed.");
}
#else
#error "Unknown Arena backend"
#endif

void *arena_alloc(Arena *a, size_t size_bytes) {
  size_t size = (size_bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

  if (a->end == NULL) {
    ARENA_ASSERT(a->begin == NULL);
    size_t capacity = REGION_DEFAULT_CAPACITY;
    if (capacity < size) capacity = size;
    a->end = new_region(capacity);
    a->begin = a->end;
  }

  while (a->end->count + size > a->end->capacity && a->end->next != NULL) {
    a->end = a->end->next;
  }

  if (a->end->count + size > a->end->capacity) {
    ARENA_ASSERT(a->end->next == NULL);
    size_t capacity = REGION_DEFAULT_CAPACITY;
    if (capacity < size) capacity = size;
    a->end->next = new_region(capacity);
    a->end = a->end->next;
  }

  void *result = &a->end->data[a->end->count];
  a->end->count += size;
  return result;
}

void *arena_realloc(Arena *a, void *oldptr, size_t oldsz, size_t newsz) {
  if (newsz <= oldsz) return oldptr;
  void *newptr = arena_alloc(a, newsz);
  char *newptr_char = newptr;
  char *oldptr_char = oldptr;
  for (size_t i = 0; i < oldsz; ++i) {
    newptr_char[i] = oldptr_char[i];
  }
  return newptr;
}

void arena_reset(Arena *a) {
  for (Region *r = a->begin; r != NULL; r = r->next) {
    r->count = 0;
  }

  a->end = a->begin;
}

void arena_free(Arena *a) {
  Region *r = a->begin;
  while (r) {
    Region *r0 = r;
    r = r->next;
    free_region(r0);
  }
  a->begin = NULL;
  a->end = NULL;
}

#endif // ARENA_IMPLEMENTATION
