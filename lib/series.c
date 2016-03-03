#include "etvdb_private.h"
#include <inttypes.h>

/* internal functions */
static Eina_Bool _parse_series_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset UNUSED, unsigned length);

/**
 * @brief Overall Series Functions
 * @defgroup Series
 *
 * @{
 *
 * These functions retrieve general series data
 */

/**
 * @brief Get Series data by TVDB Series ID
 *
 * This function will retreive the data for one series,
 * identified by its TVDB ID.
 * This is a TVDB Base Series Record, if you need all the
 * episodes, you additionally need to call etvdb_series_populate().
 *
 * @see etvdb_series_populate.
 *
 * @param id TVDB ID of a series
 *
 * @return a Series structure on success,
 * @return NULL on failure.
 *
 * @ingroup Series
 */
EAPI Series *etvdb_series_by_id_get(uint32_t id)
{
	char uri[URI_MAX];
	Download xml;
	Eina_List *list = NULL;
	Series *s = NULL;

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/series/%"PRIu32"/%s.xml",
			etvdb_api_key, id, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get series data from server.");

	_xml_count = _xml_depth = _xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_series_cb, &list))
		CRIT("Parsing series data failed. If it happens again, please report a bug.");

	free(xml.data);

	/* we assume that only a single episode is in the list
	 * should it be more (which would be a TVDB bug), its a memleak */
	s = eina_list_data_get(list);
	list = eina_list_remove_list(list, list);

	return s;
}

/**
 * @brief Get a Series from a list and initialize it fully
 *
 * This function gets a Series from a List, usually one generated
 * with etvdb_series_find() and retreives the full Base Series Record for it.
 * This can be done by using eina_list_nth() and etvdb_series_by_id_get
 * for example, so this is only a convenience function, hiding the problem,
 * that TVDB search only returns a subset of a Base Series Record per series.
 *
 * @param list a list containing etvdb Series structures
 * @param number the number of the list item to use
 *
 * @return a fully initialized Series on success.
 * @return NULL on failure.
 *
 * @ingroup Series
 */
EAPI Series *etvdb_series_from_list_get(Eina_List *list, int number)
{
	int count;
	Series *s;

	if (number < 0) {
		ERR("Invalid Number. Only 0 and above are allowed.");
		return NULL;
	}

	count = eina_list_count(list);
	if (number >= count) {
		ERR("The selected list only has %d entries.", count);
		return NULL;
	}

	s = eina_list_nth(list, number);
	s = etvdb_series_by_id_get(s->id);

	return s;
}

/**
 * @brief Find Series by Name
 *
 * This function takes a name to search for.
 * It can also search by IMDB ID (parameter starting with "tt"),
 * or by zap2it ID (starting with "SH").
 * Important: TVDBs search only delivers a subset of a Base Series Record,
 * so you will have to retrieve the full Record by etvdb_series_by_id_get()
 * after the correct Series has been selected from the list.
 *
 * It is suggested to not use Eina_List functions to retreive one of the
 * Series, but call evdb_series_from_list_get() instead, because it will spare
 * you some further hassle, though of course you can do so if you wish.
 *
 * @param name string to search for (name or id of the series).
 *
 * @return a list containing all found series.
 *
 * @ingroup Series
 */
EAPI Eina_List *etvdb_series_find(const char *name)
{
	char *buf;
	char uri[URI_MAX];
	Download xml;
	Eina_List *list = NULL;

	if (!name)
		return NULL;

	if (!memcmp(name, "tt", 2)) {
		DBG("Searching by IMDB ID: %s", name);
		snprintf(uri, URI_MAX, TVDB_API_URI"/GetSeriesByRemoteID.php?imdbid=%s&language=%s",
				name, etvdb_language);
	} else if (!memcmp(name, "SH", 2)) {
		DBG("Searching by zap2it ID: %s", name);
		snprintf(uri, URI_MAX, TVDB_API_URI"/GetSeriesByRemoteID.php?zap2it=%s&language=%s",
				name, etvdb_language);
	} else {
		buf = curl_easy_escape(curl_handle, name, strlen(name));
		DBG("Searching by Name: %s", name);
		snprintf(uri, URI_MAX, TVDB_API_URI"/GetSeries.php?seriesname=%s&language=%s",
				buf, etvdb_language);
		free(buf);
	}

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get series data from server.");

	_xml_count = _xml_depth = _xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_series_cb, &list))
		CRIT("Parsing Series data failed. If it happens again, please report a bug.");

	free(xml.data);

	return list;
}

/**
 * @brief Populate a Series structure with Episode data
 *
 * This function populates a Series structure with all available and
 * supported Episode data.
 *
 * The Series has to be initialized and at least contain an ID.
 *
 * As this can be a fairly large amount of data (up to serveral
 * ten thousand lines XML), this function can be slow (largely limited
 * by the TVDB download speed).
 * Only use it when more than a few specific episode records are required.
 *
 * Since you might want to populate a Series that already contains episode data,
 * for example after using etvdb_episode_by_id_get(), this function will dump
 * and free all existing associated episodes.
 * Be aware of any pointers left to existing episodes before using it!
 *
 * @param s pointer to Series structure.
 *
 * @return EINA_TRUE on success
 * @return EINA_FALSE on failure.
 *
 * @ingroup Series
 *
 * @see etvdb_series_find()
 * @see etvdb_series_free()
 */
EAPI Eina_Bool etvdb_series_populate(Series *s)
{
	Eina_List *all, *l, *lnex, *sl;
	Episode *e;

	/* remove all existing list nodes to avoid corrupt data */
	EINA_LIST_FREE(s->specials, e)
		etvdb_episode_free(e);

	EINA_LIST_FOREACH_SAFE(s->seasons, l, lnex, sl) {
		EINA_LIST_FREE(sl, e);
		s->seasons = eina_list_remove_list(s->seasons, l);
	}

	if (!s->id) {
		ERR("No ID for the selected Series found.");
		return EINA_FALSE;
	}

	all = etvdb_episodes_get(s);
	if (!all) {
		ERR("Couldn't get Episodes for Series %"PRIu32, s->id);
		return EINA_FALSE;
	}

	EINA_LIST_FOREACH_SAFE(all, l, lnex, e) {
		/* specials are season 0 */
		if (e->season == 0)
			eina_list_move(&s->specials, &all, e);
		else if (eina_list_count(s->seasons) < e->season) {
			sl = NULL;
			eina_list_move(&sl, &all, e);
			s->seasons =  eina_list_append(s->seasons, sl);
		} else {
			sl = (Eina_List *)eina_list_nth(s->seasons, e->season - 1);
			eina_list_move(&sl, &all, e);
		}
	}

	if (eina_list_count(all) != 0) {
		ERR("Not all episodes could be added to the Season structure.");
		return EINA_FALSE;
	}

	EINA_LIST_FREE(all, e)
		etvdb_episode_free(e);

	return EINA_TRUE;
}

/**
 * @brief Free a Series structure
 *
 * This function frees a Series structure and its data.
 *
 * @param s pointer to Series structure
 *
 * @ingroup Series
 */
EAPI void etvdb_series_free(Series *s)
{
	Eina_List *sl;
	Episode *e;

	EINA_LIST_FREE(s->seasons, sl) {
		EINA_LIST_FREE(sl, e)
			etvdb_episode_free(e);
	}

	EINA_LIST_FREE(s->specials, e)
		etvdb_episode_free(e);

	free(s->imdb_id);
	free(s->name);
	free(s->overview);
	free(s);
}

/**
 * @brief Count episodes of one season in a Series structure
 *
 * This functions counts the number of episodes of one season
 * in a Series structure.
 *
 * @param s pointer to Series static
 * @param season number of the season
 *
 * @return number of seasons >=1 on success
 * @return 0 on error or empty structure
 *
 * @ingroup Series
 */
EAPI int etvdb_series_episodes_count(Series *s, int season)
{
	return eina_list_count(eina_list_nth(s->seasons, season - 1));
}

/**
 * @}
 */

/* this callback parses found series and puts them in a list */
static Eina_Bool _parse_series_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset UNUSED, unsigned length)
{
	char buf[length + 1];
	enum nname { UNKNOWN, ID, NAME, IMDB, OVERVIEW, RUNTIME };
	Series *series;
	Eina_List **list = data;

	switch (type) {
	case EINA_SIMPLE_XML_OPEN:
		switch (_xml_depth) {
		case 0:
			if (!TAGCMP("Data", content))
				_xml_depth++;
			break;
		case 1:
			if (!TAGCMP("Series", content)) {
				_xml_depth++;
				series = malloc(sizeof(Series));
				series->id = 0;
				series->imdb_id = NULL;
				series->name = NULL;
				series->overview = NULL;
				series->seasons = NULL;
				series->specials = NULL;
				series->runtime = 0;
				*list = eina_list_append(*list, series);
			}
			break;
		case 2:
			if (!TAGCMP("id", content))
				_xml_sibling = ID;
			else if (!TAGCMP("SeriesName", content))
				_xml_sibling = NAME;
			else if (!TAGCMP("IMDB_ID", content))
				_xml_sibling = IMDB;
			else if (!TAGCMP("Overview", content))
				_xml_sibling = OVERVIEW;
			else if (!TAGCMP("Runtime", content))
				_xml_sibling = RUNTIME;
			else
				_xml_sibling = UNKNOWN;
			break;
		}
		break;
	case EINA_SIMPLE_XML_CLOSE:
		if(!TAGCMP("Series", content)) {
			_xml_count++;
			_xml_depth--;
		}
		break;
	case EINA_SIMPLE_XML_DATA:
		if (_xml_depth == 2) {
			series = eina_list_nth(*list, _xml_count);

			switch (_xml_sibling) {
			case ID:
				MEM2STR(buf, content, length);
				sscanf(buf, "%"SCNu32, &series->id);
				DBG("Found ID: %"PRIu32, series->id);
				break;
			case NAME:
				series->name = malloc(length + 1);
				MEM2STR(buf, content, length);
				HTML2UTF(series->name, buf);
				DBG("Found Name: %s", series->name);
				break;
			case IMDB:
				series->imdb_id = malloc(length + 1);
				MEM2STR(series->imdb_id, content, length);
				DBG("Found IMDB_ID: %s", series->imdb_id);
				break;
			case OVERVIEW:
				series->overview = malloc(length + 1);
				MEM2STR(buf, content, length);
				HTML2UTF(series->overview, buf);
				DBG("Found Overview: %zu chars", strlen(series->overview));
				break;
			case RUNTIME:
				MEM2STR(buf, content, length);
				sscanf(buf, "%"SCNu16, &series->runtime);
				DBG("Found Runtime: %"PRIu16, series->runtime);
				break;
			}
		}
		break;
	default:
		break;
	}

	return EINA_TRUE;
}
