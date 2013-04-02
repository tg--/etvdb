#include "etvdb_private.h"

/**
 * @brief Basic Setup Functions
 * @defgroup Setup Init / Shutdown
 *
 * @{
 *
 * Basic setup routines.
 *
 * Before any etvdb functions can be used,
 * etvdb has to be initialized with @ref etvdb_init().
 *
 * After the application no longer requires any etvdb functions,
 * etvdb should be properly shut down with @ref etvdb_shutdown()
 * to avoid problems and memory leaks.
 *
 * etvdb logs via Eina's logging subsystem, using the "etvdb" log domain.
 */

/**
 * @brief Initialize etvdb.
 *
 * This function initializes etvdb and its dependencies.
 * It does not depend on any other init routine and has to be called
 * before any etvdb function is called.
 * You need to check its return value, on failure the behaviour of 
 * all functions is undefined.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * @see etvdb_shutdown().
 *
 * @ingroup Init
 */
EAPI Eina_Bool etvdb_init(char *api_key)
{
	if (!eina_init()) {
		printf("Eina couldn't be initialized.\n");
		return 0;
	}

	_etvdb_log_dom = eina_log_domain_register("etvdb", EINA_COLOR_CYAN); 
	if (_etvdb_log_dom < 0) {
		EINA_LOG_CRIT("Eina couldn't initialize a logging domain for: etvdb.");
		return EINA_FALSE;
	}

	if (!api_key) {
		strcpy(etvdb_api_key, ETVDB_API_KEY);
		INFO("Using ETVDBs own API key.");
	} else if (strnlen(api_key, sizeof(etvdb_api_key)) == 16) {
		memcpy(etvdb_api_key, api_key, sizeof(api_key));
		INFO("Using project specific API key.");
	} else {
		CRIT("Invalid API key format.");
		return EINA_FALSE;
	}

	if (curl_global_init(CURL_GLOBAL_NOTHING)) {
		CRIT("cURL support couldn't be initialized.");
		return EINA_FALSE;
	}

	curl_handle = curl_easy_init();
	if (!curl_handle) {
		CRIT("cURL easy support couldn't be initialized.");
		return EINA_FALSE;
	}

#ifdef DEBUG
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
	eina_log_domain_level_set("etvdb", EINA_LOG_LEVEL_DBG);
#else
	eina_log_domain_level_set("etvdb", EINA_LOG_LEVEL_ERR);
#endif

	return EINA_TRUE;
}

/**
 * @brief Shutdown etvdb.
 *
 * This function cleans up after etvdb and will free all memory
 * allocated by the library, not the user of the library.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * @see etvdb_init().
 *
 * @ingroup Init
 */
EAPI Eina_Bool etvdb_shutdown(void)
{
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	eina_log_domain_unregister(_etvdb_log_dom);
	eina_shutdown();
	return 1;
}
/**
 * @}
 */
