#include <stdio.h>
#include "./lib/hash_table.h"

#define UNUSED(x)	((void)(x))

static void print_hash_table_node(hash_table_t *ht)
{
	printf(
		"node: %p, node->value: %s, node->prev: %p, node->next: %p\n",
		ht,
		ht->value,
		ht->prev,
		ht->next
	);
}

int main(int argc, char **argv)
{
	UNUSED(argc);
	UNUSED(argv);

	hash_table_t *ht = hash_table_create();

	if (ht == NULL) {
		fprintf(stderr, "Virtual memory exhausted.\n");
		return 1;
	}

	hash_table_insert(ht, "foo", "this is a foo.");
	hash_table_insert(ht, "bar", "this is a bar.");
	hash_table_insert(ht, "baz", "this is a baz.");
	hash_table_insert(ht, "quuz", "this is quuz.");
	hash_table_insert(ht, "quux", "this is quux.");

	print_hash_table_node(hash_table_search(ht, "foo"));
	print_hash_table_node(hash_table_search(ht, "bar"));
	print_hash_table_node(hash_table_search(ht, "baz"));
	print_hash_table_node(hash_table_search(ht, "quuz"));
	print_hash_table_node(hash_table_search(ht, "quux"));

	printf("\n");

	hash_table_delete(ht, "baz");
	hash_table_delete(ht, "foo");
	hash_table_delete(ht, "quux");

	print_hash_table_node(hash_table_search(ht, "bar"));
	print_hash_table_node(hash_table_search(ht, "quuz"));

	hash_table_destroy(ht);
	return 0;
}
