#include "etvdb_private.h"

/* internal functions */
static Eina_Bool _parse_episodes_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset UNUSED, unsigned length);

/**
 * @brief Overall Episode Functions
 * @defgroup Episodes
 *
 * @{
 *
 * These functions retrieve general episode data
 */

/**
 * @brief Get all Episodes of a Series
 *
 * This function takes a etvdb Series struct and retrieves
 * all episodes for this series.
 * This list will contain all episodes without being structured
 * as seasons.
 * If you don't require the list specifically, it is suggested to
 * use etvdb_series_populate() instead.
 *
 * @param s initialized TVDB Series structure
 *
 * @return a list containing all episodes of a series.
 *
 * @ingroup Episodes
 */
EAPI Eina_List *etvdb_episodes_get(Series *s)
{
	char uri[URI_MAX];
	Download xml;
	Parser_Data pdata;

	pdata.s = s;
	pdata.data = NULL;

	if (!s->id) {
		ERR("Passed series data is not valid.");
		return NULL;
	}

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/series/%"PRIu32"/all/%s.xml", etvdb_api_key, s->id, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get series data from server.");

	pdata.xml_count = pdata.xml_depth = pdata.xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &pdata))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	return pdata.data;
}

/**
 * @brief Get the Episode that airs next
 *
 * This function returns the Episode that airs next
 * after today or a given date.
 *
 * The date string has to be in an ISO 8601 format and contain only the date.
 * Behaviour for any other input is undefined.
 * Example: 2013-03-28
 *
 * @param s Series data
 * @param date a string containing an ISO 8601 date or NULL for today
 *
 * @return the Episode that airs next after today or a given date
 * @return NULL on failure
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_airs_next_get(Series *s, char *date)
{
	char *tstr;
	struct tm *ltime;
	time_t t;
	Eina_List *lseasons, *lepisodes, *l, *ll;
	Episode *e = NULL;

	tstr = malloc(sizeof("2013-03-28"));

	if (date) {
		DBG("Requsted Date: %s", date);
		tstr = strncpy(tstr, date, sizeof("2013-03-28") - 1);
	} else {
		time(&t);
		ltime = localtime(&t);
		strftime(tstr, sizeof("2013-03-28"), "%F", ltime);
	}

	DBG("Selected Date: %s", tstr);

	lseasons = s->seasons;
	EINA_LIST_FOREACH(lseasons, l, lepisodes) {
		EINA_LIST_FOREACH(lepisodes, ll, e) {
			if (e->firstaired && (strcmp(e->firstaired, tstr) > 0))
				goto END;
		}
	}

END:
	free(tstr);
	return e;
}

/**
 * @brief Get episode data for a specific date
 *
 * This function will retrieve the data for one episode,
 * that first aired at a specific date.
 *
 * It will NOT fetch any data, so the passed Series structure has to be fully initialized,
 * e.g. with @ref etvdb_series_by_id_get.
 *
 * @param s Fully initialized TVDB Series structure.
 * @param date a ISO 8601 date string, e.g. "2014-05-25"
 *
 * @return a fully initialized Episode structure on success,
 * @return NULL on failure.
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_by_date_get(Series *s, const char *date)
{
	Eina_List *l, *ll, *sl;
	Episode *e = NULL;

	if (!s->id) {
		ERR("Passed series data is not valid.");
		return NULL;
	}

	EINA_LIST_FOREACH(s->seasons, l, sl) {
		EINA_LIST_FOREACH(sl, ll, e) {
			if (!e) {
				DBG("No episodes found!");
				break;
			}

			if (!e->firstaired) {
				DBG("Episode %s has no date in TVDB.", e->name);
				e = NULL;
			} else {
				DBG("Checking episode %s with date %s for match with %s", e->name, e->firstaired, date);
				if (!strncmp(e->firstaired, date, sizeof("2000-01-01"))) {
					DBG("Episode %s aired on %s", e->name, date);
					break;
				} else
					e = NULL;
			}
		}

		if (e)
			break;
	}

	/* also search in special episodes */
	if (!e) {
		EINA_LIST_FOREACH(s->specials, l, e) {
			if (!e) {
				DBG("No special episodes! No match in regular episodes.");
				break;
			}

			if (!e->firstaired) {
				DBG("Episode %s has no date in TVDB.", e->name);
				e = NULL;
			} else {
				DBG("Checking episode %s with date %s for match with %s", e->name, e->firstaired, date);
				if (!strncmp(e->firstaired, date, sizeof("2000-01-01")))
					break;
				else
					e = NULL;
			}
		}
	}

	return e;
}

/**
 * @brief Get episode data for one specific Episode
 *
 * This function will retreive the data for one episode,
 * according to its episode ID.
 *
 * You may notice that unlike all other episode functions this one
 * uses the Series argument differently. This is intentional, because this function
 * is unique in that it can initialize a Series structure on it's own, since the
 * TVDB Episode ID does not require to know the Series beforehand.
 *
 * Please note, that while the series initialization includes associating it to the episode,
 * the reverse is not true, so the returned episode data cannot be found in the series.
 *
 * @param id TVDB ID of a episode
 *
 * @param s TVDB Series structure. If necessary it will be initialized. May NOT be NULL!
 *
 * @return a Episode structure on success,
 * @return NULL on failure.
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_by_id_get(uint32_t id, Series **s)
{
	char uri[URI_MAX];
	Download xml;
	Parser_Data pdata;
	Episode *e = NULL;

	pdata.s = *s;
	pdata.data = NULL;

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/episodes/%"PRIu32"/%s.xml", etvdb_api_key, id, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get episode data from server.");

	pdata.xml_count = pdata.xml_depth = pdata.xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &pdata))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	/* we assume that only a single episode is in the list
	 * should it be more (which would be a TVDB bug), its a memleak */
	e = eina_list_data_get(pdata.data);
	pdata.data = eina_list_remove_list(pdata.data, pdata.data);

	*s = pdata.s;

	return e;
}

/**
 * @brief Get episode data for one specific Episode
 *
 * This function will retreive the data for one episode,
 * according to its season and episode number.
 *
 * @param s initialized TVDB Series structure
 * @param season season number of the episode
 * @param episode episode number in the season
 *
 * @return a Episode structure on success,
 * @return NULL on failure.
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_by_number_get(Series *s, int season, int episode)
{
	char uri[URI_MAX];
	Download xml;
	Episode *e = NULL;
	Parser_Data pdata;

	pdata.s = s;
	pdata.data = NULL;

	if (!s->id) {
		ERR("Passed series data is not valid.");
		return NULL;
	}

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/series/%"PRIu32"/default/%d/%d/%s.xml",
			etvdb_api_key, s->id, season, episode, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get episode data from server.");

	pdata.xml_count = pdata.xml_depth = pdata.xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &pdata))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	/* we assume that only a single episode is in the list
	 * should it be more (which would be a TVDB bug), its a memleak */
	e = eina_list_data_get(pdata.data);
	pdata.data = eina_list_remove_list(pdata.data, pdata.data);

	return e;
}

/**
 * @brief Free a Episode structure
 *
 * This function frees a Episode structure and its data.
 * Note that a referenced parent series won't be freed.
 *
 * @param e pointer to Episode structure
 *
 * @ingroup Episodes
 */
EAPI void etvdb_episode_free(Episode *e)
{
	free(e->imdb_id);
	free(e->name);
	free(e->overview);
	free(e->firstaired);
	free(e);
}

/**
 * @brief Get Episode data from Series data
 *
 * This function will return a pointer to Episode data inside a
 * initialized Series struct.
 * It can also get special episode by passing 0 as season number.
 *
 * @param s Series data
 * @param season season number, 0 for specials
 * @param episode episode number
 *
 * @return pointer to episode structure on success
 * @return NULL on failure
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_from_series_get(Series *s, int season, int episode)
{
	if (season == 0)
		return eina_list_nth(s->specials, episode - 1);
	else
		return eina_list_nth(eina_list_nth(s->seasons, season - 1), episode - 1);
}

/**
 * @brief Get the Episode that aired most recently
 *
 * This function returns the Episode that initially aired last
 * before today or a given date.
 *
 * The date string has to be in an ISO 8601 format and contain only the date.
 * Behaviour for any other input is undefined.
 * Example: 2013-03-28
 *
 * @param s Series data
 * @param date a string containing an ISO 8601 date or NULL for today
 *
 * @return the Episode aired latest
 * @return NULL on failure
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_latest_aired_get(Series *s, char *date)
{
	char *tstr;
	struct tm *ltime;
	time_t t;
	Eina_List *lseasons, *lepisodes, *l, *ll;
	Episode *e = NULL;

	tstr = malloc(sizeof("2013-03-28"));

	if (date) {
		DBG("Requsted Date: %s", date);
		tstr = strncpy(tstr, date, sizeof("2013-03-28") - 1);
	} else {
		time(&t);
		ltime = localtime(&t);
		strftime(tstr, sizeof("2013-03-28"), "%F", ltime);
	}

	DBG("Selected Date: %s", tstr);

	lseasons = eina_list_last(s->seasons);
	EINA_LIST_REVERSE_FOREACH(lseasons, l, lepisodes) {
		EINA_LIST_REVERSE_FOREACH(lepisodes, ll, e) {
			if (e->firstaired) {
				if (strcmp(e->firstaired, tstr) <= 0) {
					DBG("Latest Episode aired on: %s", e->firstaired);
					goto END;
				}
			}
		}
	}

END:
	free(tstr);
	return e;
}

/**
 * @brief Initialize new Episode structure
 *
 * This function will set up a new Episode structure,
 * and take care of all details, so you don't have to.
 *
 * @see etvdb_episode_free()
 *
 * @returns the pointer to an allocated Episode structure on success,
 * or NULL on failure.
 */
EAPI Episode *etvdb_episode_new()
{
	Episode *e = NULL;

	e = malloc(sizeof(Episode));
	e->id = 0;
	e->imdb_id = NULL;
	e->firstaired = NULL;
	e->name = NULL;
	e->overview = NULL;
	e->series = NULL;
	e->number = 0;
	e->season = 0;

	return e;
}
/**
 * @}
 */

/* this callback parses the episodes of tvdb's all document and populates a Series structure */
static Eina_Bool _parse_episodes_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset UNUSED, unsigned length)
{
	char buf[length + 1];
	enum nname { UNKNOWN, ID, NAME, IMDB, OVERVIEW, FIRSTAIRED, NUMBER, SEASON, SERIES };
	Episode *episode;
	Parser_Data *pdata = data;
	uint32_t id = 0;

	switch (type) {
	case EINA_SIMPLE_XML_OPEN:
		switch (pdata->xml_depth) {
		case 0:
			if (!TAGCMP("Data", content))
				pdata->xml_depth++;
			break;
		case 1:
			if (!TAGCMP("Episode", content)) {
				pdata->xml_depth++;
				episode = etvdb_episode_new();
				pdata->data = eina_list_append(pdata->data, episode);
			}
			break;
		case 2:
			if (!TAGCMP("id", content))
				pdata->xml_sibling = ID;
			else if (!TAGCMP("EpisodeName", content))
				pdata->xml_sibling = NAME;
			else if (!TAGCMP("IMDB_ID", content))
				pdata->xml_sibling = IMDB;
			else if (!TAGCMP("Overview", content))
				pdata->xml_sibling = OVERVIEW;
			else if (!TAGCMP("FirstAired", content))
				pdata->xml_sibling = FIRSTAIRED;
			else if (!TAGCMP("EpisodeNumber", content))
				pdata->xml_sibling = NUMBER;
			else if (!TAGCMP("SeasonNumber", content))
				pdata->xml_sibling = SEASON;
			else if (!TAGCMP("seriesid", content))
				pdata->xml_sibling = SERIES;
			else
				pdata->xml_sibling = UNKNOWN;
			break;
		}
		break;
	case EINA_SIMPLE_XML_CLOSE:
		if (!TAGCMP("Episode", content)) {
			pdata->xml_count++;
			pdata->xml_depth--;
		}
		break;
	case EINA_SIMPLE_XML_DATA:
		if (pdata->xml_depth == 2) {
			episode = eina_list_nth(pdata->data, pdata->xml_count);

			switch (pdata->xml_sibling) {
			case ID:
				MEM2STR(buf, content, length);
				sscanf(buf, "%"SCNu32, &episode->id);
				DBG("Found ID: %"PRIu32, episode->id);
				break;
			case NAME:
				episode->name = malloc(length + 1);
				MEM2STR(buf, content, length);
				HTML2UTF(episode->name, buf);
				DBG("Found Name: %s", episode->name);
				break;
			case IMDB:
				episode->imdb_id = malloc(length + 1);
				MEM2STR(episode->imdb_id, content, length);
				DBG("Found IMDB_ID: %s", episode->imdb_id);
				break;
			case OVERVIEW:
				episode->overview = malloc(length + 1);
				MEM2STR(buf, content, length);
				HTML2UTF(episode->overview, buf);
				DBG("Found Overview: %zu chars", strlen(episode->overview));
				break;
			case FIRSTAIRED:
				episode->firstaired = malloc(length + 1);
				MEM2STR(episode->firstaired, content, length);
				DBG("Found First Aired Date: %s", episode->firstaired);
				break;
			case NUMBER:
				MEM2STR(buf, content, length);
				sscanf(buf, "%"SCNu16, &episode->number);
				DBG("Found Episode Number: %d", episode->number);
				break;
			case SEASON:
				MEM2STR(buf, content, length);
				sscanf(buf, "%"SCNu16, &episode->season);
				DBG("Found Season Number: %d", episode->season);
				break;
			case SERIES:
				if (pdata->s && pdata->s->id) {
					DBG("Found Series ID, but using existing one.");
					episode->series = pdata->s;
				} else {
					MEM2STR(buf, content, length);
					sscanf(buf, "%"SCNu32, &id);
					pdata->s = episode->series = etvdb_series_by_id_get(id);
					DBG("Found Series ID: %"PRIu32, episode->series->id);
				}
				break;
			}
		}
		break;
	default:
		break;
	} /* switch (type) */

	return EINA_TRUE;
}
