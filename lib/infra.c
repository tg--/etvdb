#include "etvdb_private.h"

/* internal functions */
static Eina_Bool _parse_time_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length);
static Eina_Bool _parse_lang_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length);
static void _hash_free_cb(void *data);

/**
 * @brief General TVDB infrastructure API
 * @defgroup Infrastructure
 *
 * @{
 *
 * These functions retrieve basic infrastructure Data
 * from the TVDB which are not directly related to any media.
 */

/**
 * @brief Function to retrieve supported languages.
 *
 * This function parses a xml file containing tvdb language data,
 * and adds it to a hash table (which it initializes).
 * The xml data will be downloaded, or (optionally) read from a file.
 *
 * If you no longer need the data, you'll have to free the hashtable
 * (with eina_hash_free()), but not the data in the table.
 *
 * @param lang_file_path Path to a XML file containing TVDB's supported languages.
 * This allows to provide a custom XML language file.
 * It is recommended to pass NULL. In this case the languages.xml file provide with
 * etvdb will be used and, if not available, the languages will be retrieved online.
 *
 * @return a pointer to a Eina hashtable on success, NULL on failure
 *
 * @ingroup Infrastructure
 */
EAPI Eina_Hash *etvdb_languages_get(const char *lang_file_path)
{
	char uri[URI_MAX];
	Download xml;
	Eina_File *file = NULL;
	Eina_Hash *hash = NULL;

	hash = eina_hash_string_superfast_new(_hash_free_cb);

	if(!hash) {
		ERR("Hash table not valid or initialized.");
		return NULL;
	}

	/* use path passed to the function or default */
	if (lang_file_path) {
		file = eina_file_open(lang_file_path, EINA_FALSE);
	}
	else {
		file = eina_file_open(DATA_LANG_FILE_XML, EINA_FALSE);
	}

	/* if no local file can be read, download xml data */
	if (file) {
		xml.len = eina_file_size_get(file);
		xml.data = eina_file_map_all(file, EINA_FILE_POPULATE);
		if(!xml.data)
			return NULL;

		DBG("Read %s file with size %d", eina_file_filename_get(file), (int)xml.len);
	}
	else {
		xml.data = malloc(1);
		if (!xml.data)
			return NULL;
		xml.len = 0;

		snprintf(uri, URI_MAX, TVDB_API_URI"/%s/languages.xml", etvdb_api_key);
		curl_easy_setopt(curl_handle, CURLOPT_URL, uri);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _dl_to_mem_cb);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&xml);
		if (curl_easy_perform(curl_handle)) {
			ERR("Couldn't get languages from server.");
			return NULL;
		}
	}

	_xml_count = 0;
	_xml_depth = 0;
	_xml_sibling = 0;

	if (!eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_lang_cb, hash))
		CRIT("Parsing of languages.xml failed. Probably invalid XML file.");

	if (file)
		eina_file_close(file);
	else
		free(xml.data);

	return hash;
}

/**
 * @brief Change the global language setting
 *
 * This function sets the global language to a user setting.
 * It is optional and there always is a default setting in place.
 *
 * @param hash hash table holding supported languages, as generated
 * by @see etvdb_languages_get()
 *
 * @param lang 2 character language code, e.g. "en" or "fr" (required)
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure
 *
 * @ingroup Infrastructure
 */
EAPI Eina_Bool etvdb_language_set(Eina_Hash *hash, char *lang)
{
	if (!lang || (strlen(lang) != 2)) {
		ERR("Invalid languages. Falling back to default.");
		return EINA_FALSE;
	}

	if (!eina_hash_find(hash, lang)) {
		WARN("Language %s not found. Using default.", lang);
		return EINA_FALSE;
	}

	strcpy(etvdb_language, lang);

	return EINA_TRUE;
}

/**
 * @brief Function to retrieve time from TVDB's servers
 *
 * This function will retrieve the server time from TVDB which is useful
 * to update existing data.
 * For larger persistent data sets, it is recommended to store this time with the data.
 *
 * @return > 0 if successful
 * If 0 is returned, the time could not be retrieved from the TVDB servers.
 *
 * @ingroup Infrastructure
 */
EAPI time_t etvdb_server_time_get(void)
{
	time_t server_time = 0;
	Download xml;

	xml.data = malloc(1);
	xml.len = 0;

	curl_easy_setopt(curl_handle, CURLOPT_URL, TVDB_API_URI"/Updates.php?type=none");
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _dl_to_mem_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&xml);
	if (curl_easy_perform(curl_handle)) {
		ERR("Couldn't get time from server.");
		return 0;
	}

	_xml_depth = 0;
	eina_simple_xml_parse(xml.data, xml.len, EINA_TRUE, _parse_time_cb, (void *)&server_time);
	DBG("Server Time: %ld", server_time);

	free(xml.data);

	return server_time;
}
/**
 * @}
 */

/* this callback parses TVDBs languages.xml format and writes the data in a hashtable */
static Eina_Bool _parse_lang_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length)
{
	char buf[length + 1];
	char itoa[sizeof(_xml_count) + 1];
	char key[3];
	enum nname { UNKNOWN, NAME, ABBR };

	switch (type) {
	case EINA_SIMPLE_XML_OPEN:
		switch (_xml_depth) {
		case 0:
			if (!strncmp("Languages>", content, strlen("Languages>")))
				_xml_depth++;
			break;
		case 1:
			if (!strncmp("Language>", content, strlen("Language>"))) {
				_xml_depth++;
				eina_convert_itoa(_xml_count, itoa);
				eina_hash_add(data, itoa, "");
			}
			break;
		case 2:
			if (!strncmp("name>", content, strlen("name>")))
				_xml_sibling = NAME;
			else if (!strncmp("abbreviation>", content, strlen("abbreviation>")))
				_xml_sibling = ABBR;
			else
				_xml_sibling = UNKNOWN;
			break;
		}
		break;
	case EINA_SIMPLE_XML_CLOSE:
		if (!strncmp("Language>", content, strlen("Language>"))) {
			_xml_count++;
			_xml_depth--;
		}
		break;
	case EINA_SIMPLE_XML_DATA:
		if (_xml_depth == 2) {
			eina_convert_itoa(_xml_count, itoa);
			MEM2STR(buf, content, length);

			switch (_xml_sibling) {
			case NAME:
				DBG("Found Name: %s", buf);
				MEM2STR(key, (char *)eina_hash_find(data, itoa), 2);

				/* if the entry of the hash is empty, add data to it;
				 * else take the data and use it as 2-char hash */
				if (key[0] == '\0')
					eina_hash_set(data, itoa, strdup(buf));
				else {
					eina_hash_move(data, itoa, key);
					free(eina_hash_modify(data, key, strdup(buf)));
				}
				break;
			case ABBR:
				DBG("Found Abbreviation: %s", buf);

				/* if the hash is found an contains an empty string, add key as data;
				 * if found, but not empty, rename the key
				 * otherwise the xml is invalid */
				void *p = (char *)eina_hash_find(data, itoa);
				if (p)
					if (strncmp(p, "", 1) == 0)
						eina_hash_set(data, itoa, strdup(buf));
					else
						eina_hash_move(data, itoa, buf);
				else
					return EINA_FALSE;
				break;
			}
		}
	default:
		break;
	} /* switch (type) */
	return EINA_TRUE;
}

/* this callback parses and stores TVDBs server time */
static Eina_Bool _parse_time_cb(void *data, Eina_Simple_XML_Type type, const char *content,
		unsigned offset, unsigned length)
{
	char buf[length + 1];

	switch (type) {
	case EINA_SIMPLE_XML_OPEN:
		if (_xml_depth == 0 && !strncmp("Items>", content, strlen("Items>")))
			_xml_depth++;
		else if (_xml_depth == 1 && !strncmp("Time>", content, strlen("Time>")))
			_xml_depth++;
		break;
	case EINA_SIMPLE_XML_DATA:
		if (_xml_depth == 2) {
			MEM2STR(buf, content, length);
			*((int*)data) = strtol(buf, NULL, 10);
		}
		break;
	default:
		break;
	}

	return EINA_TRUE;
}

/* free the data in the hashtable */
static void _hash_free_cb(void *data) {
	free(data);
}
