#include "etvdb_private.h"

/* internal functions */
static Eina_Bool _parse_series_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length);

/**
 * @brief Overall Series Functions
 * @defgroup Series
 *
 * @{
 *
 * These functions retrieve general series data
 */

/**
 * @brief Find Series by Name
 *
 * This function takes a name to search for.
 * It can also search by IMDB ID (parameter starting with "tt"),
 * or by zap2it ID (starting with "SH").
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

	if (!strncmp(name, "tt", 2)) {
		DBG("Searching by IMDB ID: %s", name);
		snprintf(uri, URI_MAX, TVDB_API_URI"/GetSeriesByRemoteID.php?imdbid=%s&language=%s",
				name, etvdb_language);
	} else if (!strncmp(name, "SH", 2)) {
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

	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_series_cb, &list))
		CRIT("Parsing Series data failed. If it happens again, please report a bug.");

	free(xml.data);

	return list;
}

/**
 * @brief Free a List filled with Series nodes
 *
 * This function frees a list filled exclusively with
 * Series nodes. It will free all node data and the list itself.
 *
 * As not every item does contain all supported Series data,
 * it is necessary to check each 
 *
 * If, for some reason, you have other data in the list,
 * you'll have to free it yourself.
 *
 * @param list Eina_List filled only with Series nodes
 *
 * @ingroup Series
 */
EAPI void etvdb_series_free(Eina_List *list)
{
	Series *data;

	EINA_LIST_FREE(list, data) {
		free(data->id);
		free(data->imdb_id);
		free(data->name);
		free(data->overview);
		free(data);
	}
}
/**
 * @}
 */

/* this callback parses found series and puts them in a list */
static Eina_Bool _parse_series_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length)
{
	char buf[length + 1];
	enum nname { UNKNOWN, ID, NAME, IMDB, OVERVIEW};
	Series *series;
	Eina_List **list = data;

	if (type == EINA_SIMPLE_XML_OPEN) {
		if (_xml_depth == 0 && !strncmp("Data>", content, strlen("Data>")))
			_xml_depth++;
		else if (_xml_depth == 1 && !strncmp("Series>", content, strlen("Series>"))) {
			_xml_depth++;
			series = malloc(sizeof(Series));
			series->id = NULL;
			series->imdb_id = NULL;
			series->name = NULL;
			series->overview = NULL;
			*list = eina_list_append(*list, series);
		} else if (_xml_depth == 2 && !strncmp("seriesid>", content, strlen("seriesid>")))
			_xml_sibling = ID;
		else if (_xml_depth == 2 && !strncmp("SeriesName>", content, strlen("SeriesName>")))
			_xml_sibling = NAME;
		else if (_xml_depth == 2 && !strncmp("IMDB_ID>", content, strlen("IMDB_ID>")))
			_xml_sibling = IMDB;
		else if (_xml_depth == 2 && !strncmp("Overview>", content, strlen("Overview>")))
			_xml_sibling = OVERVIEW;
		else
			_xml_sibling = UNKNOWN;
	} else if (type == EINA_SIMPLE_XML_CLOSE && !strncmp("Series>", content, strlen("Series>"))) {
		_xml_count++;
		_xml_depth--;
	} else if (type == EINA_SIMPLE_XML_DATA && _xml_depth == 2) {
		series = eina_list_nth(*list, _xml_count);
		if (_xml_sibling == ID) {
			series->id = (void *)malloc(sizeof(buf));
			snprintf(series->id, sizeof(buf), "%s", content);
			DBG("Found ID: %s", series->id);
		} else if (_xml_sibling == NAME) {
			series->name = (void *)malloc(sizeof(buf));
			snprintf(buf, sizeof(buf), "%s", content);
			HTML2UTF(series->name, buf);
			DBG("Found Name: %s", series->name);
		} else if (_xml_sibling == IMDB) {
			series->imdb_id = (void *)malloc(sizeof(buf));
			snprintf(series->imdb_id, sizeof(buf), "%s", content);
			DBG("Found IMDB_ID: %s", series->imdb_id);
		} else if (_xml_sibling == OVERVIEW) {
			series->overview = (void *)malloc(sizeof(buf));
			snprintf(buf, sizeof(buf), "%s", content);
			HTML2UTF(series->overview, buf);
			DBG("Found Overview: %zu chars", strlen(series->overview));
		}
	}

	return EINA_TRUE;
}
