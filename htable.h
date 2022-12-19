#ifndef _HTABLE_H_
#define _HTABLE_H_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct ht_bucket {
	void* key;
	void* value;
} ht_bucket;

typedef struct ht {
    ht_bucket* buckets;
    size_t capacity;
    size_t length;
} ht;

ht* ht_create(void);
void ht_destroy(ht* table);
void* ht_get(ht* table, void* key);
void* ht_put(ht* table, void* key, void* value);
size_t ht_length(ht* table);
bool ht_contains_key(ht* table, void* key);

#endif // _HTABLE_H_
