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
 * This function takes a string, a TVDB Series ID, and retreives
 * all episodes for this series.
 * This list will contain all episodes without being structured
 * as seasons.
 *
 * @param id TVDB ID of a series
 *
 * @return a list containing all episodes of a series.
 *
 * @ingroup Episodes
 */
EAPI Eina_List *etvdb_episodes_get(const char *id)
{
	char uri[URI_MAX];
	Download xml;
	Eina_List *list = NULL;

	snprintf(uri, URI_MAX, TVDB_API_URI"/%s/series/%s/all/%s.xml", etvdb_api_key, id, etvdb_language);

	xml.data = malloc(1);
	if (!xml.data)
		return NULL;
	xml.len = 0;

	curl_easy_setopt(curl_handle, CURLOPT_URL, uri);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _dl_to_mem_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&xml);
	if (curl_easy_perform(curl_handle))
		ERR("Couldn't get series data from server.");

	_xml_count = 0;
	_xml_depth = 0;
	_xml_sibling = 0;

	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_episodes_cb, &list))
		CRIT("Parsing Episode data failed. If it happens again, please report a bug.");

	free(xml.data);

	return list;
}

/**
 * @brief Free a Episode structure
 *
 * This function frees a Episode structure and its data.
 *
 * @param episode pointer to Episode structure
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
 * @}
 */

/* this callback parses the episodes of tvdb's all document and populates a Series structure */
static Eina_Bool _parse_episodes_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length)
{
	char buf[length + 1];
	enum nname { UNKNOWN, ID, NAME, IMDB, OVERVIEW, NUMBER, SEASON };
	Episode *episode;
	Eina_List **list = data;

	if (type == EINA_SIMPLE_XML_OPEN) {
		if (_xml_depth == 0 && !strncmp("Data>", content, strlen("Data>")))
			_xml_depth++;
		else if (_xml_depth == 1 && !strncmp("Episode>", content, strlen("Episode>"))) {
			_xml_depth++;
			episode = malloc(sizeof(Episode));
			episode->id = NULL;
			episode->imdb_id = NULL;
			episode->name = NULL;
			episode->overview = NULL;
			episode->number = 0;
			episode->season = 0;
			*list = eina_list_append(*list, episode);
		} else if (_xml_depth == 2 && !strncmp("id>", content, strlen("id>")))
			_xml_sibling = ID;
		else if (_xml_depth == 2 && !strncmp("EpisodeName>", content, strlen("EpisodeName>")))
			_xml_sibling = NAME;
		else if (_xml_depth == 2 && !strncmp("IMDB_ID>", content, strlen("IMDB_ID>")))
			_xml_sibling = IMDB;
		else if (_xml_depth == 2 && !strncmp("Overview>", content, strlen("Overview>")))
			_xml_sibling = OVERVIEW;
		else if (_xml_depth == 2 && !strncmp("EpisodeNumber>", content, strlen("EpisodeNumber>")))
			_xml_sibling = NUMBER;
		else if (_xml_depth == 2 && !strncmp("SeasonNumber>", content, strlen("SeasonNumber>")))
			_xml_sibling = SEASON;
		else
			_xml_sibling = UNKNOWN;
	} else if (type == EINA_SIMPLE_XML_CLOSE && !strncmp("Episode>", content, strlen("Episode>"))) {
		_xml_count++;
		_xml_depth--;
	} else if (type == EINA_SIMPLE_XML_DATA && _xml_depth == 2) {
		episode = eina_list_nth(*list, _xml_count);
		if (_xml_sibling == ID) {
			episode->id = (void *)malloc(sizeof(buf));
			snprintf(episode->id, sizeof(buf), "%s", content);
			DBG("Found ID: %s", episode->id);
		} else if (_xml_sibling == NAME) {
			episode->name = (void *)malloc(sizeof(buf));
			snprintf(buf, sizeof(buf), "%s", content);
			HTML2UTF(episode->name, buf);
			DBG("Found Name: %s", episode->name);
		} else if (_xml_sibling == IMDB) {
			episode->imdb_id = (void *)malloc(sizeof(buf));
			snprintf(episode->imdb_id, sizeof(buf), "%s", content);
			DBG("Found IMDB_ID: %s", episode->imdb_id);
		} else if (_xml_sibling == OVERVIEW) {
			episode->overview = (void *)malloc(sizeof(buf));
			snprintf(buf, sizeof(buf), "%s", content);
			HTML2UTF(episode->overview, buf);
			DBG("Found Overview: %zu chars", strlen(episode->overview));
		} else if (_xml_sibling == NUMBER) {
			snprintf(buf, sizeof(buf), "%s", content);
			episode->number = atoi(buf);
			DBG("Found Episode Number: %d", episode->number);
		} else if (_xml_sibling == SEASON) {
			snprintf(buf, sizeof(buf), "%s", content);
			episode->season = atoi(buf);
			DBG("Found Season Number: %d", episode->season);
		}
	}

	return EINA_TRUE;
}
