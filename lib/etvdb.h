/* Copyright (C) 2013  Thomas Gstaedtner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __ETVDB_H__
#define __ETVDB_H__

/**
 *  @mainpage etvdb Library Documentation
 *
 *  @date 2013
 *
 *  @section intro What is etvdb?
 *
 *  etvdb is a high-level library for the TVDB (http://www.thetvdb.com) public API.
 *  By using the functions etvdb provides, data from the TVDB can be retrieved,
 *  without the need to manually download or further parse any attributes.
 *
 *  Please note: all functions are synchronous, and many download data via HTTP,
 *  which may take several seconds or even longer to complete; so it might be wise
 *  to call these functions in a separate thread for interactive applications.
 *
 *  It is not meant for bulk data (though interfaces for this may be provided in future).
 *  It is also not a local database, though it can be used to retrieve data for one.
 *
 *  To get started, check out the following sections:
 *  @li @ref Setup
 *  @li @ref Infrastructure
 *  @li @ref Episodes
 *  @li @ref Series
 */

#include <stdlib.h>
#include <string.h>
#include <Eina.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef __GNUC__
# if __GNUC__ >= 4
#  define EAPI __attribute__ ((visibility("default")))
# else
#  define EAPI
# endif
#else
# define EAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief This array holds the TVDB api key for ETVDB.
 * You should never manually overwrite it.
 * @see etvdb_init()
 */
char etvdb_api_key[17];

/**
 * \brief This array holds the language id.
 * It is initialized by default and can be overview
 * via etvdb_language_set().
 * You should never manually overwrite it.
 * @see etvdb_language_set()
 * @see etvdb_init()
 */
char etvdb_language[3];

/**
 * this structure represents a TVDB Series
 *
 * it is roughly comparable to TVDB's Base Series Record.
 */
typedef struct _etvdb_series {
	char *id; /**< TVDB ID */
	char *imdb_id; /**< IMDB Series ID */
	char *name; /**< Series Name */
	char *overview; /**< Series Description */
	Eina_List *seasons; /**< List containing 1 list per season */
	Eina_List *specials; /**< List containing special episodes */
} Series;

/**
 * this structure represents a TVDB Episode
 *
 * it is roughly comparable to TVDB's Base Series Record.
 */
typedef struct _etvdb_episode {
	char *id; /**< TVDB ID */
	char *imdb_id; /**< IMDB Episode ID */
	char *name; /**< Episode Name */
	char *overview; /**< Episode Description */
	int number; /**< Episode Number in Season */
	int season; /**< Season Number in Series */
} Episode;

/**
 * @file
 * @brief This is the public etvdb API.
 */
EAPI Eina_Bool      etvdb_init(char api_key[17]);
EAPI Eina_Bool      etvdb_shutdown(void);

EAPI Eina_Hash     *etvdb_languages_get(const char *lang_file_path);
EAPI Eina_Bool      etvdb_language_set(Eina_Hash *hash, char *lang);
EAPI time_t         etvdb_server_time_get(void);

EAPI Eina_List     *etvdb_series_find(const char *name);
EAPI void           etvdb_series_free(Series *s);
EAPI Eina_Bool      etvdb_series_populate(Series *s);

EAPI Eina_List     *etvdb_episodes_get(const char *id);
EAPI Episode       *etvdb_episode_by_id_get(const char *id);
EAPI Episode       *etvdb_episode_by_number_get(const char *id, int season, int episode);
EAPI void           etvdb_episode_free(Episode *e);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ETVDB_H__ */
