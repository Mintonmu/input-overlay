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

#include "dialog_setup.hpp"
#include "../tool.hpp"
#include "../util/constants.hpp"
#include "../util/localization.hpp"
#include "../util/notifier.hpp"
#include <fstream>

#if _DEBUG
#define TEXTURE_PATH "E:\\projects\\obs-plugins\\input-overlay\\build\\presets\\wasd.png"
#define CONFIG_PATH "E:\\projects\\obs-plugins\\input-overlay\\build\\presets\\wasd.json"
#else
#define TEXTURE_PATH m_tool->get_texture_path()
#define CONFIG_PATH m_tool->get_config_path()
#endif

void dialog_setup::init()
{
	dialog::init();
	int8_t id = 2;

	// info labels
	auto info = std::string(LABEL_BUILD);
	info.append(TIMESTAMP);

	add<label>(8, 22, info.c_str(), FONT_WSTRING_LARGE, this, ELEMENT_UNLOCALIZED | ELEMENT_ABSOLUTE_POSITION);
	add<label>(8, 58, LANG_LABEL_INFO, this, ELEMENT_ABSOLUTE_POSITION);
	add<label>(8, 35, LANG_LABEL_TEXTURE_PATH, this);
	m_texture_path = add<textbox>(8, 55, m_dimensions.w - 16, 20, TEXTURE_PATH, this);

	add<label>(8, 85, LANG_LABEL_CONFIG_PATH, this);
	m_config_path = add<textbox>(8, 105, m_dimensions.w - 16, 20, CONFIG_PATH, this);

	add<label>(8, 135, LANG_LABEL_DEFAULT_WIDTH, this);
	add<label>((m_dimensions.w / 2) + 4, 135, LANG_LABEL_DEFAULT_HEIGHT, this);

	add<label>(8, 185, LANG_LABEL_ELEMENT_H_SPACE, this);
	add<label>((m_dimensions.w / 2) + 4, 185, LANG_LABEL_ELEMENT_V_SPACE, this);

	m_def_w = add<textbox>(8, 155, (m_dimensions.w / 2) - 16, 20, "0", this);
	m_def_h = add<textbox>((m_dimensions.w / 2) + 4, 155, (m_dimensions.w / 2) - 12, 20, "0", this);

	m_h_space = add<textbox>(8, 205, (m_dimensions.w / 2) - 16, 20, "0", this);
	m_v_space = add<textbox>((m_dimensions.w / 2) + 4, 205, (m_dimensions.w / 2) - 12, 20, "0", this);

	m_def_w->set_flags(TEXTBOX_NUMERIC);
	m_def_h->set_flags(TEXTBOX_NUMERIC);

	m_h_space->set_flags(TEXTBOX_NUMERIC);
	m_v_space->set_flags(TEXTBOX_NUMERIC);

	m_config_path->set_flags(TEXTBOX_DROP_FILE);
	m_texture_path->set_flags(TEXTBOX_DROP_FILE);

	add<button>(8, m_dimensions.h - 32, LANG_BUTTON_OK, this)->set_id(ACTION_OK);
	add<button>(116, m_dimensions.h - 32, LANG_BUTTON_EXIT, this)->set_id(ACTION_CANCEL);

	m_lang_box = add<combobox>(m_dimensions.w - 148, m_dimensions.h - 28, 140, 20, this, ELEMENT_UNLOCALIZED);

	const auto files = m_helper->get_localization()->get_languages();

	for (auto const &f : *files) {
		m_lang_box->add_item(f.m_language);
	}

	m_lang_box->select_item(m_helper->get_localization()->get_english_id());

	set_flags(DIALOG_CENTERED | DIALOG_TEXTINPUT);
}

void dialog_setup::draw_background()
{
	dialog::draw_background();
}

void dialog_setup::action_performed(const int8_t action_id)
{
	dialog::action_performed(action_id);

	auto valid_texture = false;
	auto writable_config = false;
	json cfg_json;
	std::string err;

	switch (action_id) {
	case ACTION_OK:
		valid_texture = m_helper->util_check_texture_path(m_texture_path->get_text()->c_str());
		m_have_existing_cfg = !util::is_empty(*m_config_path->get_text());
		if (!m_have_existing_cfg)
			writable_config = util::can_access(*m_config_path->get_text());

		if (m_have_existing_cfg && util::load_json(*m_config_path->get_text(), err, cfg_json)) {
			const auto def_w = cfg_json[CFG_DEFAULT_WIDTH];
			const auto def_h = cfg_json[CFG_DEFAULT_HEIGHT];
			const auto space_h = cfg_json[CFG_H_SPACE];
			const auto space_v = cfg_json[CFG_V_SPACE];

			if (def_w.is_number())
				m_def_w->set_text(def_w.number_value());
			if (def_h.is_number())
				m_def_h->set_text(def_h.number_value());
			if (space_h.is_number())
				m_h_space->set_text(space_h.number_value());
			if (space_v.is_number())
				m_v_space->set_text(space_v.number_value());
		}
		if (valid_texture && (writable_config || m_have_existing_cfg)) {
			m_tool->action_performed(TOOL_ACTION_SETUP_EXIT);
		} else {
			if (m_texture_path->get_text()->empty() || !valid_texture) {
				m_texture_path->set_alert(true);
				m_notifier->add_msg(MESSAGE_ERROR, m_helper->loc(LANG_ERROR_INVALID_TEXTURE_PATH));
			}

			if (m_config_path->get_text()->empty() || !writable_config) {
				m_config_path->set_alert(true);
				m_notifier->add_msg(MESSAGE_ERROR, m_helper->loc(LANG_ERROR_INVALID_CONFIG_PATH));
			}
		}
		break;
	case ACTION_CANCEL:
		m_helper->exit_loop();
		break;
	case ACTION_FILE_DROPPED:
		if (util::load_json(*m_config_path->get_text(), err, cfg_json)) {
			const auto def_w = cfg_json[CFG_DEFAULT_WIDTH];
			const auto def_h = cfg_json[CFG_DEFAULT_HEIGHT];
			const auto space_h = cfg_json[CFG_H_SPACE];
			const auto space_v = cfg_json[CFG_V_SPACE];

			if (def_w.is_number())
				m_def_w->set_text(def_w.number_value());
			if (def_h.is_number())
				m_def_h->set_text(def_h.number_value());
			if (space_h.is_number())
				m_h_space->set_text(space_h.number_value());
			if (space_v.is_number())
				m_v_space->set_text(space_v.number_value());
		} else {
			m_notifier->add_msg(MESSAGE_ERROR, m_helper->loc(LANG_MSG_LOAD_ERROR));
			m_notifier->add_msg(MESSAGE_ERROR, err.c_str());
		}
		break;
	case ACTION_COMBO_ITEM_SELECTED:
		m_helper->get_localization()->load_lang_by_id(m_lang_box->get_selected());
		reload_lang();
		break;
	default:;
	}
}

const char *dialog_setup::get_config_path() const
{
	return m_config_path->get_text()->c_str();
}

const char *dialog_setup::get_texture_path() const
{
	return m_texture_path->get_text()->c_str();
}

SDL_Point dialog_setup::get_rulers() const
{
	return SDL_Point{std::stoi(m_h_space->get_text()->c_str()), std::stoi(m_v_space->get_text()->c_str())};
}

SDL_Point dialog_setup::get_default_dim() const
{
	return SDL_Point{std::stoi(m_def_w->get_text()->c_str()), std::stoi(m_def_h->get_text()->c_str())};
}

bool dialog_setup::should_load_cfg() const
{
	return m_have_existing_cfg;
}
