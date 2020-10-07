#include "djb2.h"

unsigned long djb2_hash(char *buf)
{
	unsigned long hash = 5381;
	unsigned int c;

	while ((c = *buf++) != 0) {
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}
