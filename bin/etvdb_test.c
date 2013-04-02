#include <etvdb.h>

Eina_Bool print_languages(const Eina_Hash *hash, const void *key, void *data, void *fdata) {
	printf("\tShort: %s, Full: %s\n", (char *)key, (char *)data);
	return EINA_TRUE;
}

int main() {
	Eina_Hash *languages;
	Eina_List *series, *l;
	Series *data; 

	if (!etvdb_init(NULL))
		return 1;

	etvdb_server_time_get();

	languages = etvdb_languages_get(NULL);

	printf("All Languages:\n");
	eina_hash_foreach(languages, print_languages, NULL);

	printf("\nTry to find some languages:\n");
	printf("\tLanguage for 'en': %s\n", (char *)eina_hash_find(languages, "en"));
	printf("\tLanguage for 'sv': %s\n", (char *)eina_hash_find(languages, "sv"));

	series = etvdb_series_get("Super");
	printf("\nCounted %d Series, Searchstring: 'Super':\n", eina_list_count(series));

	EINA_LIST_FOREACH(series, l, data)
		printf("\tID: %s, Serienname: %s\n", data->id, data->name);

	/* free stuff */
	eina_hash_free(languages);
	etvdb_series_free(series);


	etvdb_shutdown();

	return 0;
}
