/*************************************************************************
 * This file is part of input-overlay
 * github.con/univrsal/input-overlay
 * Copyright 2020 univrsal <uni@vrsal.cf>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "localization.hpp"
#include "constants.hpp"
#include "notifier.hpp"
#include "sdl_helper.hpp"
#include <json/json11.hpp>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#else
#include "localization.hpp"
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

localization::localization(const char *lang_folder, sdl_helper *helper)
{
	m_lang_folder = lang_folder;
	m_helper = helper;
	scan_lang_folder();

	if (m_langfiles.empty() || !m_have_default) {
		printf("No valid language files found! GUI will use identifiers instead\n");
	} else {
		load_default_language();
	}
}

void localization::load_lang_by_id(uint8_t id)
{
	if (m_have_default && id < m_langfiles.size()) {
		if (id == m_english_id) {
			m_current = json11::Json::object{};
			return; /* English is always loaded */
		}

		const auto &lang = m_langfiles[id];
		if (!lang.m_file_path.empty()) {
			std::string err;
			util::load_json(lang.m_file_path, err, m_current);
		} else {
			printf("Can't load language with id %i\n", id);
		}
	}
}

std::string localization::localize(const char *id) const
{
	std::string value = "";
	if (m_have_default) {
		if (!m_current.is_null() && (m_current[id]).is_string())
			value = m_current[id].string_value();
		else
			value = m_english[id].string_value();
	}

	if (!m_have_default || value.empty()) {
		printf("Couldn't find get_localization for %s.\n", id);
		value = std::string(id);
	}
	return value;
}

void localization::scan_lang_folder()
{
	json11::Json lang;
	std::string file_name;
#ifdef _WIN32
	WIN32_FIND_DATA data;
	auto path = m_lang_folder;

	path.append("/*.json");
	const auto h_find = FindFirstFile(path.c_str(), &data);

	if (h_find != INVALID_HANDLE_VALUE) {
		do {
			std::string full_path = m_lang_folder + "/" + data.cFileName;
			file_name = data.cFileName;
			if (!(GetFileAttributes(full_path.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) {
#else
	/* Iterating over items in folder on linux
     * and filtering only *.json files
     */
	DIR *dir;
	struct dirent *dirent;

	dir = opendir(m_lang_folder.c_str());

	if (dir) {
		while ((dirent = readdir(dir))) {
			file_name = std::string(dirent->d_name);
			struct stat path_stat {
			};
			std::string full_path = m_lang_folder + "/" + file_name;
			stat(full_path.c_str(), &path_stat);
			char *file_type = strrchr(dirent->d_name, '.');

			if (file_type && !strcmp(file_type, ".json") /* Checks file ending */
				&& S_ISREG(path_stat.st_mode)) {
				std::string full_path = m_lang_folder + "/" + file_name;
#endif
				std::string err;
				lang = json11::Json();
				if (!util::load_json(full_path, err, lang)) {
					printf("Error while loading language from %s: %s\n", full_path.c_str(), err.c_str());
					continue;
				}

				/* After filtering on windows and linux store file name
                 * in file_name
                 */
				auto val = lang[LANG_ID];
				if (val.is_string()) {
					if (file_name == "en_US.json") {
						m_have_default = true;
						m_english_id = m_langfiles.size();
					}
					m_langfiles.emplace_back(full_path, val.string_value());
				} else {
					printf("Invalid lang file %s. Missing language identifier\n", file_name.c_str());
				}
			}
#ifdef _WIN32
		} while (FindNextFile(h_find, &data));
		FindClose(h_find);
#else
		}
		closedir(dir);
#endif
	} else {
		printf("Localization: Couldn't load files from lang folder!\n");
	}
}

void localization::load_default_language()
{
	std::string error;

	if (m_have_default) {
		auto path = m_lang_folder + "/en_US.json";
		if (!util::load_json(path, error, m_english)) {
			m_have_default = false;
			printf("Loading of English translation failed: %s!\n", error.c_str());
			printf("Couldn't load default language! Localization might not work!\n");
		}
	}
}
