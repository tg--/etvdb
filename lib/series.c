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
	free(s->id);
	free(s->imdb_id);
	free(s->name);
	free(s->overview);
	free(s);
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

	switch (type) {
	case EINA_SIMPLE_XML_OPEN:
		switch (_xml_depth) {
		case 0:
			if (!strncmp("Data>", content, strlen("Data>")))
				_xml_depth++;
			break;
		case 1:
			if (!strncmp("Series>", content, strlen("Series>"))) {
				_xml_depth++;
				series = malloc(sizeof(Series));
				series->id = NULL;
				series->imdb_id = NULL;
				series->name = NULL;
				series->overview = NULL;
				series->seasons = NULL;
				series->specials = NULL;
				*list = eina_list_append(*list, series);
			}
			break;
		case 2:
			if (!strncmp("seriesid>", content, strlen("seriesid>")))
				_xml_sibling = ID;
			else if (!strncmp("SeriesName>", content, strlen("SeriesName>")))
				_xml_sibling = NAME;
			else if (!strncmp("IMDB_ID>", content, strlen("IMDB_ID>")))
				_xml_sibling = IMDB;
			else if (!strncmp("Overview>", content, strlen("Overview>")))
				_xml_sibling = OVERVIEW;
			else
				_xml_sibling = UNKNOWN;
			break;
		}
		break;
	case EINA_SIMPLE_XML_CLOSE:
		if(!strncmp("Series>", content, strlen("Series>"))) {
			_xml_count++;
			_xml_depth--;
		}
		break;
	case EINA_SIMPLE_XML_DATA:
		if (_xml_depth == 2) {
			series = eina_list_nth(*list, _xml_count);

			switch (_xml_sibling) {
			case ID:
				series->id = malloc(length + 1);
				MEM2STR(series->id, content, length);
				DBG("Found ID: %s", series->id);
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
			}
		}
		break;
	default:
		break;
	}

	return EINA_TRUE;
}
