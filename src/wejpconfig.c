/*
 * Wejp's Config File Parser
 *
 * File: wejpconfig.c
 *
 * Copyright (c) 2003-2009 Johannes Heimansberg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define VERSION 20090801
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wejpconfig.h"
#define MAX_LINE_LENGTH 1024

/* Copies at most max characters of s into a new string */
static char *cfg_strndup(const char *s, size_t max)
{
	size_t len = strlen(s);
	char *copy;

	if (len > max)
		len = max;
	copy = (char*)malloc(len + 1);
	if (copy) {
		memcpy(copy, s, len);
		copy[len] = '\0';
	}
	return copy;
}

/* Returns the complete path to ~/"filename" */
char *cfg_get_path_to_config_file(char *filename)
{
	const char *home_directory;
	char *path_to_config_file;

	home_directory = getenv("HOME");
	if (home_directory == NULL || home_directory[0] == '\0')
		home_directory = ".";
	path_to_config_file = (char*)malloc(strlen(home_directory) + strlen(filename) + 2);
	sprintf(path_to_config_file, "%s/%s", home_directory, filename);
	return path_to_config_file;
}

/* Must be called before the first use of a ConfigFile struct element */
void cfg_init_config_file_struct(ConfigFile *cf)
{
	cf->lastkey = 0;
}

/* Checks wether the config file exists or not */
int cfg_check_config_file(char *filename)
{
	FILE *file;
	int  result = 0;
	file = fopen(filename, "r");
	if (file == NULL)
		result = -1;
	else
		fclose(file);
	return result;
}

/* Add a new key to the configuration */
int cfg_add_key(ConfigFile *cf, char *key, char *value)
{
	int result = 0;
	int i;

	if (key == NULL || value == NULL)
		return -1;

	if (cfg_get_key_value(cf, key) != NULL) { /* Key already exists->overwrite */
		for (i = 0; i < cf->lastkey; i++)
			if (strncmp(key, cf->key[i], MAX_LINE_LENGTH-1) == 0) {
				free(cf->value[i]);
				cf->value[i] = cfg_strndup(value, MAX_LINE_LENGTH-2);
				break;
			}
	} else if (cf->lastkey < MAXKEYS) {
		cf->key[cf->lastkey] = cfg_strndup(key, MAX_LINE_LENGTH-2);
		cf->value[cf->lastkey] = cfg_strndup(value, MAX_LINE_LENGTH-2);
		(cf->lastkey)++;
	}
	else
		result = -1;
	return result;
}

/* Frees all memory allocated by read_config_file() */
void cfg_free_config_file_struct(ConfigFile *cf)
{
	int i;
	for (i = 0; i < cf->lastkey; i++) {
		free(cf->key[i]);
		free(cf->value[i]);
	}
	cf->lastkey = 0;
}

/* Loads a config file from disk */
int cfg_read_config_file(ConfigFile *cf, char *filename)
{
	FILE *file;
	char line[MAX_LINE_LENGTH];

	file = fopen(filename, "r");
	if (file == NULL) {
		printf("config: Cannot open config file!\n");
		return -2;
	}

	while (fgets(line, sizeof line, file) != NULL) {
		char *key = line, *value, *end;

		/* drop the line break and anything after a comment character */
		line[strcspn(line, "\r\n")] = '\0';
		if ((end = strchr(line, '#')) != NULL)
			*end = '\0';

		while (*key == ' ' || *key == '\t')
			key++;
		if (*key == '\0')
			continue;

		value = strchr(key, '=');
		if (value == NULL)
			continue;
		*value++ = '\0';

		/* trim blanks around key and value */
		end = key + strlen(key);
		while (end > key && (end[-1] == ' ' || end[-1] == '\t'))
			*--end = '\0';
		while (*value == ' ' || *value == '\t')
			value++;
		end = value + strlen(value);
		while (end > value && (end[-1] == ' ' || end[-1] == '\t'))
			*--end = '\0';

		if (*key != '\0')
			cfg_add_key(cf, key, value);
	}
	fclose(file);
	return 0;
}

/* Saves the configuration to file */
int cfg_write_config_file(ConfigFile *cf, char *filename)
{
	FILE *file;
	int   i = 0, result = 0;
	char  buffer[MAX_LINE_LENGTH];

	file = fopen(filename, "w");
	if (file != NULL) {
		while (i < cf->lastkey) {
			snprintf(buffer, MAX_LINE_LENGTH, "%s=%s\n", cf->key[i], cf->value[i]);
			if (fwrite(buffer, strlen(buffer) * sizeof(char), 1, file) == 0) {
				printf("config: Error while writing to %s.\n", filename);
				break;
			}
			i++;
		}
		fclose(file);
	} else {
		printf("config: Failed opening %s for write access.\n", filename);
		result = -1;
	}
	return result;
}

/* Returns the value (as string) of "key" */
char *cfg_get_key_value(const ConfigFile *cf, char *key)
{
	char *result = NULL;
	int   i;
	
	for (i = 0; i < cf->lastkey; i++)
		if (strncmp(key, cf->key[i], MAX_LINE_LENGTH-1) == 0) {
			result = cf->value[i];
			break;
		}
	return result;
}

int cfg_is_key_available(const ConfigFile *cf, char *key)
{
	return cfg_get_key_value(cf, key) != NULL;
}

int cfg_add_key_if_not_present(ConfigFile *cf, char *key, char *value)
{
	int success = 0;

	if (!cfg_is_key_available(cf, key)) {
		success = (cfg_add_key(cf, key, value) == 0 ? 1 : 0);
	}
	return success;
}
