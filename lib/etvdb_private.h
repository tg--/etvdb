#ifndef __ETVDB_PRIVATE_H__
#define __ETVDB_PRIVATE_H__

#include <stdlib.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include "entities.h"
#include "etvdb.h"

#define CRIT(...) EINA_LOG_DOM_CRIT(_etvdb_log_dom, __VA_ARGS__)
#define ERR(...)  EINA_LOG_DOM_ERR(_etvdb_log_dom, __VA_ARGS__)
#define INFO(...) EINA_LOG_DOM_INFO(_etvdb_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_etvdb_log_dom, __VA_ARGS__)
#define DBG(...)  EINA_LOG_DOM_DBG(_etvdb_log_dom, __VA_ARGS__)

#define HTML2UTF(x, y) decode_html_entities_utf8(x, y)

/* this copies a non-nul-terminated buffer and terminates It
 * len is the size of src, dst has to be len+1 */
#define MEM2STR(dst, src, slen) \
	memcpy(dst, src, slen); \
	dst[slen] = '\0';


#define ETVDB_API_KEY "A34C5A0CAF0F3EFD"
#define TVDB_API_URI "http://thetvdb.com/api"

#ifndef DATA_LANG_FILE_XML
  #define DATA_LANG_FILE_XML "../data/languages.xml"
#endif

#ifndef URI_MAX
  #define URI_MAX 4096
#endif

/* eina logging domain for etvdb */
int _etvdb_log_dom;

/* auxiliary variables for the xml parsers */
int _xml_count, _xml_depth, _xml_sibling;
CURL *curl_handle;

typedef struct _download {
	size_t len;
	char *data;
} Download;

size_t _dl_to_mem_cb(char *ptr, size_t size, size_t nmemb, void *userdata);

#endif /* __ETVDB_PRIVATE_H__ */
