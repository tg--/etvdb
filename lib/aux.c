#include "etvdb_private.h"

/* this function stores cURL downloads in memory */
size_t _dl_to_mem_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t size_total = size * nmemb;
	Download *mem = (Download *)userdata;

	mem->data = realloc(mem->data, mem->len + size_total + 1);
	if (!mem->data) {
		ERR("Couldn't allocate enough memory.");
		return -1;
	}

	memcpy(&(mem->data[mem->len]), ptr, size_total);
	mem->len += size_total;
	mem->data[mem->len] = 0;

	return size_total;
}
