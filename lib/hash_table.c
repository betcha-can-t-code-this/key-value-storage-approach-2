#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "chained_key.h"
#include "djb2.h"
#include "hash_table.h"

hash_table_t *hash_table_create(void)
{
	hash_table_t *ht = malloc(sizeof(hash_table_t));

	if (ht == NULL) {
		return NULL;
	}

	ht->hash = 0;
	ht->value = NULL;
	ht->chained = NULL;
	ht->prev = NULL;
	ht->next = NULL;

	return ht;
}

static int __hash_table_handle_collision(hash_table_t *ht, char *key, char *value)
{
	if (!chain_key_exists(ht->chained, key)) {
		return 0;
	}

	if (chain_key_insert(ht->chained, key) < 0) {
		return -1;
	}

	// free previous string value
	free(ht->value);

	// assign new value to it's associated key
	// in hash table.
	ht->value = strdup(value);

	return 0;
}

int hash_table_insert(hash_table_t *ht, char *key, char *value)
{
	if (ht == NULL) {
		return -1;
	}

	hash_table_t *tmp = ht;
	hash_table_t *rtmp;
	unsigned long hash = djb2_hash(key);

	while (tmp->next != NULL) {
		if (hash == tmp->hash) {
			return __hash_table_handle_collision(tmp, key, value);
		}

		tmp = tmp->next;
	}

	rtmp = hash_table_create();

	if (rtmp == NULL) {
		return -1;
	}

	rtmp->hash = hash;
	rtmp->value = strdup(value);
	rtmp->chained = chain_key_create();

	// insert first known key to
	// chained key storage.
	chain_key_insert(rtmp->chained, key);

	rtmp->prev = tmp;
	tmp->next = rtmp;

	return 0;
}

hash_table_t *hash_table_search(hash_table_t *ht, char *key)
{
	if (ht == NULL) {
		return NULL;
	}

	if (djb2_hash(key) == ht->hash
		&& (ht->chained == NULL ? -1 : chain_key_exists(ht->chained, key)) == 0) {
		return ht;
	}

	return hash_table_search(ht->next, key);
}

static void __hash_table_destroy_single_node(hash_table_t *ht)
{
	free(ht->value);
	chain_key_destroy(ht->chained);
	free(ht);
}

int hash_table_delete(hash_table_t *ht, char *key)
{
	if (ht == NULL) {
		return -1;
	}

	hash_table_t *tmp;

	if ((tmp = hash_table_search(ht, key)) == NULL) {
		return -1;
	}

	if (tmp->next != NULL) {
		tmp->next->prev = tmp->prev;
	}

	tmp->prev->next = tmp->next;

	__hash_table_destroy_single_node(tmp);

	return 0;
}

void hash_table_destroy(hash_table_t *ht)
{
	if (ht == NULL) {
		return;
	}

	hash_table_destroy(ht->next);
	__hash_table_destroy_single_node(ht);
}
