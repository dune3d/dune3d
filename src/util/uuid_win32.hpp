#pragma once
typedef unsigned char uuid_t[16];

extern void uuid_clear(uuid_t uu);
extern void uuid_generate_random(uuid_t out);
extern void uuid_unparse(const uuid_t uu, char *out);
extern int uuid_parse(const char *in, uuid_t uu);
extern int uuid_compare(const uuid_t uu1, const uuid_t uu2);
extern int uuid_is_null(const uuid_t uu);