#ifndef __CHAINED_KEY_H__
#define __CHAINED_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct chained_key {
	char *key;
	struct chained_key *prev;
	struct chained_key *next;
} chained_key_t;

chained_key_t *chain_key_create(void);
int chain_key_insert(chained_key_t *chain, char *data);
int chain_key_exists(chained_key_t *chain, char *data);
void chain_key_destroy(chained_key_t *chain);

#ifdef __cplusplus
}
#endif

#endif /* __CHAINED_KEY_H__ */
