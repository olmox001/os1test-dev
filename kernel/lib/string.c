/*
 * kernel/lib/string.c
 * String manipulation functions
 */
#include <kernel/types.h>

/*
 * strlen - calculate string length
 */
size_t strlen(const char *s) {
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

/*
 * strnlen - calculate string length with limit
 */
size_t strnlen(const char *s, size_t maxlen) {
  const char *p = s;
  while (maxlen-- && *p)
    p++;
  return p - s;
}

/*
 * strcmp - compare two strings
 */
int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

/*
 * strncmp - compare two strings with limit
 */
int strncmp(const char *s1, const char *s2, size_t n) {
  while (n && *s1 && *s1 == *s2) {
    s1++;
    s2++;
    n--;
  }
  if (n == 0)
    return 0;
  return (unsigned char)*s1 - (unsigned char)*s2;
}

/*
 * strcpy - copy string
 */
char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++) != '\0')
    ;
  return dest;
}

/*
 * strncpy - copy string with limit
 */
char *strncpy(char *dest, const char *src, size_t n) {
  char *d = dest;
  while (n && (*d++ = *src++) != '\0')
    n--;
  while (n--)
    *d++ = '\0';
  return dest;
}

/*
 * strcat - concatenate strings
 */
char *strcat(char *dest, const char *src) {
  char *d = dest;
  while (*d)
    d++;
  while ((*d++ = *src++) != '\0')
    ;
  return dest;
}

/*
 * strchr - find character in string
 */
char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c)
      return (char *)s;
    s++;
  }
  return (c == '\0') ? (char *)s : NULL;
}

/*
 * strrchr - find last occurrence of character
 */
char *strrchr(const char *s, int c) {
  const char *last = NULL;
  while (*s) {
    if (*s == (char)c)
      last = s;
    s++;
  }
  return (c == '\0') ? (char *)s : (char *)last;
}

/*
 * memset - fill memory with value
 */
void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n--)
    *p++ = (unsigned char)c;
  return s;
}

/*
 * memcpy - copy memory
 */
void *memcpy(void *dest, const void *src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;
  while (n--)
    *d++ = *s++;
  return dest;
}

/*
 * memmove - copy memory (overlapping safe)
 */
void *memmove(void *dest, const void *src, size_t n) {
  unsigned char *d = dest;
  const unsigned char *s = src;

  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }
  return dest;
}

/*
 * memcmp - compare memory
 */
int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;

  while (n--) {
    if (*p1 != *p2)
      return *p1 - *p2;
    p1++;
    p2++;
  }
  return 0;
}

/*
 * memchr - find byte in memory
 */
void *memchr(const void *s, int c, size_t n) {
  const unsigned char *p = s;
  while (n--) {
    if (*p == (unsigned char)c)
      return (void *)p;
    p++;
  }
  return NULL;
}

/*
 * bzero - zero memory (BSD compatibility)
 */
void bzero(void *s, size_t n) { memset(s, 0, n); }
