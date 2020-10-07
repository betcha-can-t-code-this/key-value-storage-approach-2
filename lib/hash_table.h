#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "chained_key.h"

typedef struct hash_table {
	unsigned long hash;
	char *value;
	chained_key_t *chained;
	struct hash_table *prev;
	struct hash_table *next;
} hash_table_t;

hash_table_t *hash_table_create(void);
int hash_table_insert(hash_table_t *ht, char *key, char *value);
hash_table_t *hash_table_search(hash_table_t *ht, char *key);
int hash_table_delete(hash_table_t *ht, char *key);
void hash_table_destroy(hash_table_t *ht);

#ifdef __cplusplus
}
#endif

#endif /* __HASH_TABLE_H__ */
