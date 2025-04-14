#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#include "../include/daemon.h"

/**
 * Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef intptr_t ssize_t;

typedef struct {
  const WCHAR *const wide;
  const WCHAR *const wide_eq;
  const size_t len;
} daemon_env_var_t;

#define V(str) {L##str, L##str L"=", sizeof(str)}

static const daemon_env_var_t daemon__required_env_vars[] = {
  V("HOMEDRIVE"),
  V("HOMEPATH"),
  V("LOGONSERVER"),
  V("PATH"),
  V("SYSTEMDRIVE"),
  V("SYSTEMROOT"),
  V("TEMP"),
  V("USERDOMAIN"),
  V("USERNAME"),
  V("USERPROFILE"),
  V("WINDIR"),
};

#undef V

static inline int32_t
daemon__wtf8_decode1(const char **input) {
  uint32_t code_point;
  uint8_t b1;
  uint8_t b2;
  uint8_t b3;
  uint8_t b4;

  b1 = **input;
  if (b1 <= 0x7F) return b1;
  if (b1 < 0xC2) return -1;
  code_point = b1;

  b2 = *++*input;
  if ((b2 & 0xC0) != 0x80) return -1;
  code_point = (code_point << 6) | (b2 & 0x3F);
  if (b1 <= 0xDF) return 0x7FF & code_point;

  b3 = *++*input;
  if ((b3 & 0xC0) != 0x80) return -1;
  code_point = (code_point << 6) | (b3 & 0x3F);
  if (b1 <= 0xEF) return 0xFFFF & code_point;

  b4 = *++*input;
  if ((b4 & 0xC0) != 0x80) return -1;
  code_point = (code_point << 6) | (b4 & 0x3F);
  if (b1 <= 0xF4) {
    code_point &= 0x1FFFFF;
    if (code_point <= 0x10FFFF) return code_point;
  }

  return -1;
}

static inline ssize_t
daemon__utf16_length_from_wtf8(const char *source) {
  size_t target_len = 0;
  int32_t code_point;

  do {
    code_point = daemon__wtf8_decode1(&source);

    if (code_point < 0) return -1;
    if (code_point > 0xFFFF) target_len++;

    target_len++;
  } while (*source++);

  return target_len;
}

static inline void
daemon__wtf8_to_utf16(const char *source, uint16_t *target) {
  int32_t code_point;

  do {
    code_point = daemon__wtf8_decode1(&source);

    if (code_point > 0xFFFF) {
      *target++ = (((code_point - 0x10000) >> 10) + 0xD800);
      *target++ = ((code_point - 0x10000) & 0x3FF) + 0xDC00;
    } else {
      *target++ = code_point;
    }
  } while (*source++);
}

static inline int
daemon__utf8_to_utf16(const char *utf8, WCHAR **result) {
  int len;
  len = daemon__utf16_length_from_wtf8(utf8);
  if (len < 0) return -1;

  WCHAR *utf16 = malloc(len * sizeof(WCHAR));
  assert(utf16);

  daemon__wtf8_to_utf16(utf8, utf16);

  *result = utf16;

  return 0;
}

static inline WCHAR *
daemon__quote_argument(const WCHAR *source, WCHAR *target) {
  size_t len = wcslen(source);
  size_t i;
  int quote_hit;
  WCHAR *start;

  if (len == 0) {
    *(target++) = L'"';
    *(target++) = L'"';

    return target;
  }

  if (NULL == wcspbrk(source, L" \t\"")) {
    wcsncpy(target, source, len);
    target += len;

    return target;
  }

  if (NULL == wcspbrk(source, L"\"\\")) {
    *(target++) = L'"';
    wcsncpy(target, source, len);
    target += len;
    *(target++) = L'"';

    return target;
  }

  *(target++) = L'"';
  start = target;
  quote_hit = 1;

  for (i = len; i > 0; --i) {
    *(target++) = source[i - 1];

    if (quote_hit && source[i - 1] == L'\\') {
      *(target++) = L'\\';
    } else if (source[i - 1] == L'"') {
      quote_hit = 1;
      *(target++) = L'\\';
    } else {
      quote_hit = 0;
    }
  }

  target[0] = L'\0';
  _wcsrev(start);
  *(target++) = L'"';

  return target;
}

static inline int
daemon__argv_to_command_line(const char *const *args, WCHAR **result) {
  const char *const *arg;
  WCHAR *dst = NULL;
  WCHAR *tmp = NULL;
  size_t dst_len = 0;
  size_t tmp_len = 0;
  WCHAR *pos;
  int arg_count = 0;

  for (arg = args; *arg; arg++) {
    ssize_t arg_len = daemon__utf16_length_from_wtf8(*arg);

    if (arg_len < 0) return arg_len;

    dst_len += arg_len;

    if ((size_t) arg_len > tmp_len) tmp_len = arg_len;

    arg_count++;
  }

  dst_len = dst_len * 2 + arg_count * 2;

  dst = malloc(dst_len * sizeof(WCHAR));
  assert(dst);

  tmp = malloc(tmp_len * sizeof(WCHAR));
  assert(tmp);

  pos = dst;

  for (arg = args; *arg; arg++) {
    ssize_t arg_len = daemon__utf16_length_from_wtf8(*arg);

    daemon__wtf8_to_utf16(*arg, tmp);

    pos = daemon__quote_argument(tmp, pos);

    *pos++ = *(arg + 1) ? L' ' : L'\0';
  }

  free(tmp);

  *result = dst;

  return 0;
}

static int
daemon__env_strncmp(const wchar_t *a, int na, const wchar_t *b) {
  wchar_t *a_eq;
  wchar_t *b_eq;
  int nb;
  int r;

  if (na < 0) {
    a_eq = wcschr(a, L'=');
    assert(a_eq);

    na = (int) (long) (a_eq - a);
  } else {
    na--;
  }

  b_eq = wcschr(b, L'=');
  assert(b_eq);

  nb = b_eq - b;

  r = CompareStringOrdinal(a, na, b, nb, TRUE);

  return r - CSTR_EQUAL;
}

static int
daemon__env_wcscmp(const void *a, const void *b) {
  wchar_t *astr = *(wchar_t *const *) a;
  wchar_t *bstr = *(wchar_t *const *) b;

  return daemon__env_strncmp(astr, -1, bstr);
}

static inline int
daemon__env_list_to_block(const char *const *env_list, WCHAR **result) {
  WCHAR *dst;
  WCHAR *ptr;
  const char *const *env;
  size_t env_len = 0;
  size_t len;
  size_t i;
  size_t var_size;
  size_t env_list_count = 1;
  WCHAR *dst_copy;
  WCHAR **ptr_copy;
  WCHAR **env_copy;
  char *p;
  size_t required_vars_value_len[ARRAY_SIZE(daemon__required_env_vars)];

  for (env = env_list; *env; env++) {
    if (strchr(*env, '=')) {
      ssize_t len = daemon__utf16_length_from_wtf8(*env);
      if (len < 0) return len;

      env_len += len;
      env_list_count++;
    }
  }

  len = env_list_count * sizeof(WCHAR *);

  p = malloc(len + env_len * sizeof(WCHAR));
  assert(p);

  env_copy = (void *) &p[0];
  dst_copy = (void *) &p[len];

  ptr = dst_copy;
  ptr_copy = env_copy;

  for (env = env_list; *env; env++) {
    if (strchr(*env, '=')) {
      ssize_t len = daemon__utf16_length_from_wtf8(*env);

      daemon__wtf8_to_utf16(*env, ptr);

      *ptr_copy++ = ptr;
      ptr += len;
    }
  }

  *ptr_copy = NULL;

  qsort(env_copy, env_list_count - 1, sizeof(wchar_t *), daemon__env_wcscmp);

  for (ptr_copy = env_copy, i = 0; i < ARRAY_SIZE(daemon__required_env_vars);) {
    int cmp;

    if (!*ptr_copy) {
      cmp = -1;
    } else {
      cmp = daemon__env_strncmp(daemon__required_env_vars[i].wide_eq, daemon__required_env_vars[i].len, *ptr_copy);
    }

    if (cmp < 0) {
      var_size = GetEnvironmentVariableW(daemon__required_env_vars[i].wide, NULL, 0);
      required_vars_value_len[i] = var_size;
      if (var_size != 0) {
        env_len += daemon__required_env_vars[i].len;
        env_len += var_size;
      }
      i++;
    } else {
      ptr_copy++;
      if (cmp == 0) i++;
    }
  }

  dst = malloc((1 + env_len) * sizeof(WCHAR));
  assert(dst);

  for (ptr = dst, ptr_copy = env_copy, i = 0; *ptr_copy || i < ARRAY_SIZE(daemon__required_env_vars); ptr += len) {
    int cmp;

    if (i >= ARRAY_SIZE(daemon__required_env_vars)) {
      cmp = 1;
    } else if (!*ptr_copy) {
      cmp = -1;
    } else {
      cmp = daemon__env_strncmp(daemon__required_env_vars[i].wide_eq, daemon__required_env_vars[i].len, *ptr_copy);
    }

    if (cmp < 0) {
      len = required_vars_value_len[i];
      if (len) {
        wcscpy(ptr, daemon__required_env_vars[i].wide_eq);
        ptr += daemon__required_env_vars[i].len;
        var_size = GetEnvironmentVariableW(daemon__required_env_vars[i].wide, ptr, (int) (env_len - (ptr - dst)));
        if (var_size != (DWORD) (len - 1)) {
          abort();
        }
      }
      i++;
    } else {
      len = wcslen(*ptr_copy) + 1;
      wmemcpy(ptr, *ptr_copy, len);
      ptr_copy++;
      if (cmp == 0) i++;
    }
  }

  *ptr = L'\0';

  free(p);

  *result = dst;

  return 0;
}

int
daemon_spawn(daemon_t *daemon, const char *file, const char *const argv[], const char *const env[], const char *cwd) {
  int err;

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));

  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  WCHAR *application_name;
  err = daemon__utf8_to_utf16(file, &application_name);
  if (err < 0) return err;

  WCHAR *command_line = NULL;
  if (argv) {
    err = daemon__argv_to_command_line(argv, &command_line);
    if (err < 0) {
      free(application_name);

      return err;
    }
  }

  WCHAR *environment = NULL;
  if (env) {
    err = daemon__env_list_to_block(env, &environment);
    if (err < 0) {
      free(application_name);
      free(command_line);

      return err;
    }
  }

  WCHAR *current_directory = NULL;
  if (cwd) {
    err = daemon__utf8_to_utf16(cwd, &current_directory);
    if (err < 0) {
      free(application_name);
      free(command_line);
      free(environment);

      return err;
    }
  }

  BOOL success = CreateProcessW(
    application_name,
    command_line,
    NULL,
    NULL,
    FALSE,
    CREATE_NO_WINDOW | DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
    environment,
    current_directory,
    &si,
    &pi
  );

  free(application_name);
  free(command_line);
  free(environment);

  if (!success) return -1;

  CloseHandle(pi.hThread);

  daemon->pid = (int) pi.dwProcessId;

  return 0;
}
