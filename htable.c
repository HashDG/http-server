#include "htable.h"

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
#define INITIAL_CAPACITY 16

ht* ht_create(void) {
	ht* table;
	if ( (table = malloc(sizeof(ht))) == NULL ) {
		return NULL;
	}
	table->length=0;
	table->capacity=INITIAL_CAPACITY;
	if ( (table->buckets=calloc(table->capacity, sizeof(ht_bucket))) == NULL ) {
		free(table);
		return NULL;
	}
	return table;
}

void ht_destroy(ht* table) {
	for (size_t i = 0; i < table->capacity; i++) {
        free(table->buckets[i].key);
    }    
    free(table->buckets);
    free(table);
}

static unsigned long hash_key(void* k) {
	unsigned long hash = FNV_OFFSET;

	for (char* p = k; *p; p++) {
		hash ^= (unsigned long)(unsigned char)(*p);
        hash *= FNV_PRIME;
	}
	return hash;
}


void* ht_get(ht* table, void* key) {
	unsigned long hash = hash_key(key);
	size_t index = (size_t) (hash & (unsigned long) (table->capacity-1) );

	while (table->buckets[index].key != NULL) {
		if ( (strcmp(key, table->buckets[index].key)) == 0) {
			return table->buckets[index].value;
		}
		if (++index >= table-> capacity) {
			index=0;
		}
	}
	return NULL;
}

static void* ht_set_bucket(ht_bucket* buckets, size_t capacity, void* key, void* value, size_t* plength) {
	unsigned long hash = hash_key(key);
	size_t index = (size_t) (hash & (unsigned long) (capacity-1) );
	
    while (buckets[index].key != NULL) {
        if ( (strcmp(key, buckets[index].key)) == 0) {
            buckets[index].value = value;
            return buckets[index].key;
        }
        if (++index >= capacity) {
            index = 0;
        }
    }
    if (plength != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return NULL;
        }
        (*plength)++;
    }
    buckets[index].key = key;
    buckets[index].value = value;
    return key;
}

static bool ht_expand(ht* table) {
	size_t new_cap = table->capacity*2;
	ht_bucket* new_buckets;
	
	if (new_cap < table->capacity) {
        return false;
    }
    if ((new_buckets = calloc(new_cap, sizeof(ht_bucket))) == NULL) {
    	return false;
    }
    
    for (size_t i = 0; i < table->capacity; i++) {
    	ht_bucket bucket = table->buckets[i];
    	if (bucket.key != NULL) {
    		ht_set_bucket(new_buckets, new_cap, bucket.key, bucket.value, NULL);
    	}
    }
    free(table->buckets);
    table->buckets=new_buckets;
    table->capacity=new_cap;
    return true;
}

void* ht_put(ht* table, void* key, void* value) {
	if (value == NULL) {
		return NULL;
	}
	if (table->length >= table->capacity/2 ) {
		if (!ht_expand(table)) {
			return NULL;
		}
	}
	return ht_set_bucket(table->buckets, table->capacity, key, value, &table->length);
}

size_t ht_length(ht* table) {
	return table->length;
}

bool ht_contains_key(ht* table, void* key) {
	unsigned long hash = hash_key(key);
	size_t index = (size_t) (hash & (unsigned long) (table->capacity-1) );

	while (table->buckets[index].key != NULL) {
		if ( (strcmp(key, table->buckets[index].key)) == 0) {
			return true;
		}
		if (++index >= table-> capacity) {
			index=0;
		}
	}
	return false;
}
