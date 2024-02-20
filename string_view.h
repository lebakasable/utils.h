#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t count;
  const char *data;
} StringView;

#define SV(cstr_lit) sv_from_parts(cstr_lit, sizeof(cstr_lit) - 1)
#define SV_STATIC(cstr_lit)                                                    \
  { sizeof(cstr_lit) - 1, (cstr_lit) }

#define SV_NULL sv_from_parts(NULL, 0)

#define SV_Fmt "%.*s"
#define SV_Arg(sv) (int)(sv).count, (sv).data

StringView sv_from_parts(const char *data, size_t count);
StringView sv_from_cstr(const char *cstr);
StringView sv_trim_left(StringView sv);
StringView sv_trim_right(StringView sv);
StringView sv_trim(StringView sv);
StringView sv_take_left_while(StringView sv, bool (*predicate)(char x));
StringView sv_chop_by_delim(StringView *sv, char delim);
StringView sv_chop_by_sv(StringView *sv, StringView thicc_delim);
bool sv_try_chop_by_delim(StringView *sv, char delim, StringView *chunk);
StringView sv_chop_left(StringView *sv, size_t n);
StringView sv_chop_right(StringView *sv, size_t n);
StringView sv_chop_left_while(StringView *sv, bool (*predicate)(char x));
bool sv_index_of(StringView sv, char c, size_t *index);
bool sv_eq(StringView a, StringView b);
bool sv_eq_ignorecase(StringView a, StringView b);
bool sv_starts_with(StringView sv, StringView prefix);
bool sv_ends_with(StringView sv, StringView suffix);
uint64_t sv_to_u64(StringView sv);
uint64_t sv_chop_u64(StringView *sv);

#endif // STRING_VIEW_H

#ifdef STRING_VIEW_IMPLEMENTATION

StringView sv_from_parts(const char *data, size_t count) {
  StringView sv;
  sv.count = count;
  sv.data = data;
  return sv;
}

StringView sv_from_cstr(const char *cstr) {
  return sv_from_parts(cstr, strlen(cstr));
}

StringView sv_trim_left(StringView sv) {
  size_t i = 0;
  while (i < sv.count && isspace(sv.data[i])) {
    i += 1;
  }

  return sv_from_parts(sv.data + i, sv.count - i);
}

StringView sv_trim_right(StringView sv) {
  size_t i = 0;
  while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
    i += 1;
  }

  return sv_from_parts(sv.data, sv.count - i);
}

StringView sv_trim(StringView sv) { return sv_trim_right(sv_trim_left(sv)); }

StringView sv_chop_left(StringView *sv, size_t n) {
  if (n > sv->count) { n = sv->count; }

  StringView result = sv_from_parts(sv->data, n);

  sv->data += n;
  sv->count -= n;

  return result;
}

StringView sv_chop_right(StringView *sv, size_t n) {
  if (n > sv->count) { n = sv->count; }

  StringView result = sv_from_parts(sv->data + sv->count - n, n);

  sv->count -= n;

  return result;
}

bool sv_index_of(StringView sv, char c, size_t *index) {
  size_t i = 0;
  while (i < sv.count && sv.data[i] != c) {
    i += 1;
  }

  if (i < sv.count) {
    if (index) { *index = i; }
    return true;
  } else {
    return false;
  }
}

bool sv_try_chop_by_delim(StringView *sv, char delim, StringView *chunk) {
  size_t i = 0;
  while (i < sv->count && sv->data[i] != delim) {
    i += 1;
  }

  StringView result = sv_from_parts(sv->data, i);

  if (i < sv->count) {
    sv->count -= i + 1;
    sv->data += i + 1;
    if (chunk) { *chunk = result; }
    return true;
  }

  return false;
}

StringView sv_chop_by_delim(StringView *sv, char delim) {
  size_t i = 0;
  while (i < sv->count && sv->data[i] != delim) {
    i += 1;
  }

  StringView result = sv_from_parts(sv->data, i);

  if (i < sv->count) {
    sv->count -= i + 1;
    sv->data += i + 1;
  } else {
    sv->count -= i;
    sv->data += i;
  }

  return result;
}

StringView sv_chop_by_sv(StringView *sv, StringView thicc_delim) {
  StringView window = sv_from_parts(sv->data, thicc_delim.count);
  size_t i = 0;
  while (i + thicc_delim.count < sv->count && !(sv_eq(window, thicc_delim))) {
    i++;
    window.data++;
  }

  StringView result = sv_from_parts(sv->data, i);

  if (i + thicc_delim.count == sv->count) { result.count += thicc_delim.count; }

  sv->data += i + thicc_delim.count;
  sv->count -= i + thicc_delim.count;

  return result;
}

bool sv_starts_with(StringView sv, StringView expected_prefix) {
  if (expected_prefix.count <= sv.count) {
    StringView actual_prefix = sv_from_parts(sv.data, expected_prefix.count);
    return sv_eq(expected_prefix, actual_prefix);
  }

  return false;
}

bool sv_ends_with(StringView sv, StringView expected_suffix) {
  if (expected_suffix.count <= sv.count) {
    StringView actual_suffix = sv_from_parts(
        sv.data + sv.count - expected_suffix.count, expected_suffix.count);
    return sv_eq(expected_suffix, actual_suffix);
  }

  return false;
}

bool sv_eq(StringView a, StringView b) {
  if (a.count != b.count) {
    return false;
  } else {
    return memcmp(a.data, b.data, a.count) == 0;
  }
}

bool sv_eq_ignorecase(StringView a, StringView b) {
  if (a.count != b.count) { return false; }

  char x, y;
  for (size_t i = 0; i < a.count; i++) {
    x = 'A' <= a.data[i] && a.data[i] <= 'Z' ? a.data[i] + 32 : a.data[i];

    y = 'A' <= b.data[i] && b.data[i] <= 'Z' ? b.data[i] + 32 : b.data[i];

    if (x != y) return false;
  }
  return true;
}

uint64_t sv_to_u64(StringView sv) {
  uint64_t result = 0;

  for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); ++i) {
    result = result * 10 + (uint64_t)sv.data[i] - '0';
  }

  return result;
}

uint64_t sv_chop_u64(StringView *sv) {
  uint64_t result = 0;
  while (sv->count > 0 && isdigit(*sv->data)) {
    result = result * 10 + *sv->data - '0';
    sv->count -= 1;
    sv->data += 1;
  }
  return result;
}

StringView sv_chop_left_while(StringView *sv, bool (*predicate)(char x)) {
  size_t i = 0;
  while (i < sv->count && predicate(sv->data[i])) {
    i += 1;
  }
  return sv_chop_left(sv, i);
}

StringView sv_take_left_while(StringView sv, bool (*predicate)(char x)) {
  size_t i = 0;
  while (i < sv.count && predicate(sv.data[i])) {
    i += 1;
  }
  return sv_from_parts(sv.data, i);
}

#endif // STRING_VIEW_IMPLEMENTATION
