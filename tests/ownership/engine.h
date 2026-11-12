/* C interface of the synthetic 4D engine (engine.cpp) used by harness.m. */
#ifndef OWNERSHIP_ENGINE_H
#define OWNERSHIP_ENGINE_H

#include <stddef.h>
#include "4DPluginAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

void eng_install(void);
void *eng_new_object(int coll);
void eng_release(void *o);
PA_Unistring eng_U(const char *ascii);
void eng_dispose_string(PA_Unistring *s);

int eng_no_leaks(void);
int eng_violations(const char *what);
int eng_string_left(const char *ascii);
size_t eng_picture_count(void);
size_t eng_object_count(void);

size_t eng_prop_count(void *o);
int eng_prop_type(void *o, const char *key);
int eng_prop_text_is(void *o, const char *key, const char *ascii);
double eng_prop_real(void *o, const char *key);
int eng_prop_bool(void *o, const char *key);
int eng_prop_live_picture(void *o, const char *key);
void *eng_prop_ref(void *o, const char *key);
size_t eng_item_count(void *c);
void *eng_item_ref(void *c, size_t i);

int eng_live_picture(void *p);
void eng_free_picture(void *p);
long eng_handle_size(void *h);
const char *eng_handle_data(void *h);
void eng_free_handle(void *h);

#ifdef __cplusplus
}
#endif

#endif
