#include "etvdb_private.h"

/* internal functions */
static Eina_Bool _parse_episodes_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length);

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
	Episode_Parser_Data pdata;

	pdata.s = s;
	pdata.list = NULL;

	if (!s->id) {
		ERR("Passed series data is not valid.");
		return NULL;
	}

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/series/%s/all/%s.xml", etvdb_api_key, s->id, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get series data from server.");

	_xml_count = _xml_depth = _xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &pdata))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	return pdata.list;
}

/**
 * @brief Get episode data for one specific Episode
 *
 * This function will retreive the data for one episode,
 * according to its episode ID.
 *
 * @param id TVDB ID of a episode
 *
 * @param s TVDB Series structure. If necessary it will be initialized, except if you pass NULL.
 *
 * @return a Episode structure on success,
 * @return NULL on failure.
 *
 * @ingroup Episodes
 */
EAPI Episode *etvdb_episode_by_id_get(const char *id, Series *s)
{
	char uri[URI_MAX];
	Download xml;
	Episode_Parser_Data pdata;
	Episode *e = NULL;

	pdata.s = s;
	pdata.list = NULL;

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/episodes/%s/%s.xml", etvdb_api_key, id, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get episode data from server.");

	_xml_count = _xml_depth = _xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &pdata))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	/* we assume that only a single episode is in the list
	 * should it be more (which would be a TVDB bug), its a memleak */
	e = eina_list_data_get(pdata.list);
	pdata.list = eina_list_remove_list(pdata.list, pdata.list);

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
	Episode_Parser_Data pdata;

	pdata.s = s;
	pdata.list = NULL;

	if (!s->id) {
		ERR("Passed series data is not valid.");
		return NULL;
	}

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/series/%s/default/%d/%d/%s.xml",
			etvdb_api_key, s->id, season, episode, etvdb_language);

	CURL_XML_DL_MEM(xml, uri)
		ERR("Couldn't get episode data from server.");

	_xml_count = _xml_depth = _xml_sibling = 0;
	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &pdata))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	/* we assume that only a single episode is in the list
	 * should it be more (which would be a TVDB bug), its a memleak */
	e = eina_list_data_get(pdata.list);
	pdata.list = eina_list_remove_list(pdata.list, pdata.list);

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
	free(e->id);
	free(e->imdb_id);
	free(e->name);
	free(e->overview);
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
 */
EAPI Episode *etvdb_episode_from_series_get(Series *s, int season, int episode)
{
	if (season == 0)
		return eina_list_nth(s->specials, episode - 1);
	else
		return eina_list_nth(eina_list_nth(s->seasons, season - 1), episode - 1);
}
/**
 * @}
 */

/* this callback parses the episodes of tvdb's all document and populates a Series structure */
static Eina_Bool _parse_episodes_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length)
{
	char buf[length + 1];
	enum nname { UNKNOWN, ID, NAME, IMDB, OVERVIEW, NUMBER, SEASON, SERIES };
	Episode *episode;
	/* Eina_List **list = data; */
	Episode_Parser_Data *pdata = data;

	switch (type) {
	case EINA_SIMPLE_XML_OPEN:
		switch (_xml_depth) {
		case 0:
			if (!strncmp("Data>", content, strlen("Data>")))
				_xml_depth++;
			break;
		case 1:
			if (!strncmp("Episode>", content, strlen("Episode>"))) {
				_xml_depth++;
				episode = malloc(sizeof(Episode));
				episode->id = NULL;
				episode->imdb_id = NULL;
				episode->name = NULL;
				episode->overview = NULL;
				episode->series = NULL;
				episode->number = 0;
				episode->season = 0;
				pdata->list = eina_list_append(pdata->list, episode);
			}
			break;
		case 2:
			if (!strncmp("id>", content, strlen("id>")))
				_xml_sibling = ID;
			else if (!strncmp("EpisodeName>", content, strlen("EpisodeName>")))
				_xml_sibling = NAME;
			else if (!strncmp("IMDB_ID>", content, strlen("IMDB_ID>")))
				_xml_sibling = IMDB;
			else if (!strncmp("Overview>", content, strlen("Overview>")))
				_xml_sibling = OVERVIEW;
			else if (!strncmp("EpisodeNumber>", content, strlen("EpisodeNumber>")))
				_xml_sibling = NUMBER;
			else if (!strncmp("SeasonNumber>", content, strlen("SeasonNumber>")))
				_xml_sibling = SEASON;
			else if (!strncmp("seriesid>", content, strlen("seriesid>")))
				_xml_sibling = SERIES;
			else
				_xml_sibling = UNKNOWN;
			break;
		}
		break;
	case EINA_SIMPLE_XML_CLOSE:
		if (!strncmp("Episode>", content, strlen("Episode>"))) {
			_xml_count++;
			_xml_depth--;
		}
		break;
	case EINA_SIMPLE_XML_DATA:
		if (_xml_depth == 2) {
			episode = eina_list_nth(pdata->list, _xml_count);

			switch (_xml_sibling) {
			case ID:
				episode->id = malloc(length + 1);
				MEM2STR(episode->id, content, length);
				DBG("Found ID: %s", episode->id);
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
			case NUMBER:
				MEM2STR(buf, content, length);
				episode->number = atoi(buf);
				DBG("Found Episode Number: %d", episode->number);
				break;
			case SEASON:
				MEM2STR(buf, content, length);
				episode->season = atoi(buf);
				DBG("Found Season Number: %d", episode->season);
				break;
			case SERIES:
				if (pdata->s && !pdata->s->id) {
					MEM2STR(buf, content, length);
					episode->series = etvdb_series_by_id_get(buf);
					DBG("Found Series ID: %s", episode->series->id);
				} else if (pdata->s)
					episode->series = pdata->s;
				break;
			}
		}
		break;
	default:
		break;
	} /* switch (type) */

	return EINA_TRUE;
}
