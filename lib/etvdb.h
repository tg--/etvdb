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
 *  @li @ref TVDB functions
 */

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
 * \brief  This array holds the TVDB api key for ETVDB.
 * You should never manually overwrite it.
 * @see etvdb_init() */
char etvdb_api_key[17];

/** this structure represents a TVDB Series */
typedef struct _etvdb_series {
	char *id; /**< TVDB ID */
	char *imdb_id; /**< IMDB Series ID */
	char *name; /**< Series Name */
	char *overview; /**< Series Description */
} Series;

/**
 * @file
 * @brief This is the public etvdb API.
 */
EAPI Eina_Bool etvdb_init(char api_key[17]);
EAPI Eina_Bool etvdb_shutdown(void);

EAPI Eina_Hash     *etvdb_languages_get(const char *lang_file_path);
EAPI time_t         etvdb_server_time_get(void);
EAPI Eina_List     *etvdb_series_get(const char *s);
EAPI void           etvdb_series_free(Eina_List *list);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ETVDB_H__ */
