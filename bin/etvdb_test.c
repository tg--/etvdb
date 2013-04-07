#include <etvdb.h>

Eina_Bool print_languages(const Eina_Hash *hash, const void *key, void *data, void *fdata) {
	printf("\tShort: %s, Full: %s\n", (char *)key, (char *)data);
	return EINA_TRUE;
}

int main() {
	Eina_Hash *languages;
	Eina_List *series, *episodes, *l;
	Series *data; 
	Episode *data2;

	if (!etvdb_init(NULL))
		return 1;

	etvdb_server_time_get();

	languages = etvdb_languages_get(NULL);

	printf("All Languages:\n");
	eina_hash_foreach(languages, print_languages, NULL);

	printf("\nSetting language to 'en'\n");
	if (!etvdb_language_set(languages, "en"))
		return 1;

	printf("\nTry to find some languages:\n");
	printf("\tLanguage for 'en': %s\n", (char *)eina_hash_find(languages, "en"));
	printf("\tLanguage for 'sv': %s\n", (char *)eina_hash_find(languages, "sv"));

	series = etvdb_series_find("Super");
	printf("\nCounted %d Series, Searchstring: 'Super':\n", eina_list_count(series));

	EINA_LIST_FOREACH(series, l, data)
		printf("\tSeries ID: %s, Serienname: %s\n", data->id, data->name);

	data = eina_list_nth(series, 1);
	episodes = etvdb_episodes_get(data->id);
	printf("\nCounted %d Episodes, SearchID: '%s':\n", eina_list_count(episodes), data->id);

	EINA_LIST_FOREACH(episodes, l, data2)
		printf("\tEpisode ID: %s, Episodename: %s\n", data2->id, data2->name);

	/* free stuff */
	eina_hash_free(languages);
	etvdb_series_free(series);

	EINA_LIST_FOREACH(episodes, l, data2)
		etvdb_episode_free(data2);


	etvdb_shutdown();

	return 0;
}
