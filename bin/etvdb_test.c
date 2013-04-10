#include <etvdb.h>

Eina_Bool print_languages(const Eina_Hash *hash, const void *key, void *ser_data, void *fser_data) {
	printf("\tShort: %s, Full: %s\n", (char *)key, (char *)ser_data);
	return EINA_TRUE;
}

int main() {
	Eina_Hash *languages;
	Eina_List *series, *seasons, *episodes, *l, *sl;
	Series *ser_data;
	Episode *epi_data;
	int i = 0;

	if (!etvdb_init(NULL))
		return 1;

	etvdb_server_time_get();

	languages = etvdb_languages_get(NULL);

	printf("All Languages:\n");
	eina_hash_foreach(languages, print_languages, NULL);

	printf("\nSetting language to 'en'\n\n");
	if (!etvdb_language_set(languages, "en"))
		return 1;

	printf("Try to find some languages:\n");
	printf("\tLanguage for 'en': %s\n", (char *)eina_hash_find(languages, "en"));
	printf("\tLanguage for 'sv': %s\n\n", (char *)eina_hash_find(languages, "sv"));

	series = etvdb_series_find("The Simpsons");
	printf("Counted %d Series, Searchstring: 'The Simpsons':\n", eina_list_count(series));

	EINA_LIST_FOREACH(series, l, ser_data)
		printf("\tSeries ID: %s, Serienname: %s\n", ser_data->id, ser_data->name);
	printf("\n");

	ser_data = eina_list_nth(series, 0);
	episodes = etvdb_episodes_get(ser_data->id);
	printf("Counted %d Episodes, SearchID: '%s':\n", eina_list_count(episodes), ser_data->id);

	EINA_LIST_FOREACH(episodes, l, epi_data)
		printf("\tEpisode ID: %s, Episodename: %s\n", epi_data->id, epi_data->name);
	printf("\n");

	printf("Populating the series structure with episode data:\n");
	etvdb_series_populate(ser_data);
	seasons = ser_data->specials;
	printf("\tCounted Special Episodes: %d\n", eina_list_count(seasons));
	seasons = ser_data->seasons;
	printf("\tCounted Seasons: %d\n", eina_list_count(seasons));
	EINA_LIST_FOREACH(seasons, l, sl) {
		i++;
		printf("\tEpisodes in Season %d: %d\n", i, eina_list_count(sl));
	}

	/* free stuff */
	eina_hash_free(languages);

	EINA_LIST_FREE(episodes, epi_data)
		etvdb_episode_free(epi_data);

	EINA_LIST_FREE(series, ser_data)
		etvdb_series_free(ser_data);

	etvdb_shutdown();

	return 0;
}
