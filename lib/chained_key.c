#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "chained_key.h"

chained_key_t *chain_key_create(void)
{
	chained_key_t *ret = malloc(sizeof(chained_key_t));

	if (ret == NULL) {
		return NULL;
	}

	ret->key = NULL;
	ret->prev = NULL;
	ret->next = NULL;
	return ret;
}

int chain_key_insert(chained_key_t *chain, char *key)
{
	if (chain == NULL) {
		return -1;
	}

	chained_key_t *tmp = chain;
	chained_key_t *rtmp;

	while (tmp->next != NULL) {
		tmp = tmp->next;
	}

	rtmp = chain_key_create();

	if (rtmp == NULL) {
		return -1;
	}

	rtmp->key = strdup(key);
	rtmp->prev = tmp;
	tmp->next = rtmp;

	return 0;
}

int chain_key_exists(chained_key_t *chain, char *key)
{
	if (chain == NULL) {
		return -1;
	}

	if (!strcmp(key, chain->key == NULL ? "" : chain->key)) {
		return 0;
	}

	return chain_key_exists(chain->next, key);
}

static void __chain_destroy_single_node(chained_key_t *chain)
{
	if (chain->key != NULL) {
		free(chain->key);
	}

	free(chain);
}

void chain_key_destroy(chained_key_t *chain)
{
	if (chain == NULL) {
		return;
	}

	chain_key_destroy(chain->next);
	__chain_destroy_single_node(chain);
}
