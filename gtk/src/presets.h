/*
 * presets.h
 * Copyright (C) John Stebbins 2008-2019 <stebbins@stebbins>
 *
 * presets.h is free software.
 *
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * presets.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with callbacks.h.  If not, write to:
 *  The Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor
 *  Boston, MA  02110-1301, USA.
 */

#if !defined(_GHB_PRESETS_H_)
#define _GHB_PRESETS_H_

#include "handbrake.h"
#include "values.h"

void ghb_presets_load(signal_user_data_t *ud);
void ghb_update_from_preset(signal_user_data_t *ud, const gchar *key);
void ghb_settings_init(GhbValue *settings, const char *name);
void ghb_settings_close();
void ghb_prefs_load(signal_user_data_t *ud);
void ghb_pref_save(GhbValue *settings, const gchar *key);
void ghb_pref_set(GhbValue *settings, const gchar *key);
void ghb_prefs_store(void);
void ghb_save_queue(GhbValue *queue);
GhbValue* ghb_load_old_queue(int pid);
void ghb_remove_old_queue_file(int pid);
gchar* ghb_get_user_config_dir(gchar *subdir);
void ghb_override_user_config_dir(char *dir);
void ghb_settings_to_ui(signal_user_data_t *ud, GhbValue *dict);
void ghb_clear_presets_selection(signal_user_data_t *ud);
void ghb_select_preset(signal_user_data_t *ud, const char *name, int type);
void ghb_select_default_preset(signal_user_data_t *ud);
void ghb_presets_list_init(signal_user_data_t *ud,
                           const hb_preset_index_t *path);
void ghb_presets_menu_init(signal_user_data_t *ud);
int ghb_find_pid_file();
void ghb_write_pid_file();
GhbValue* ghb_get_current_preset(signal_user_data_t *ud);
void ghb_preset_to_settings(GhbValue *settings, GhbValue *preset);
void ghb_prefs_to_settings(GhbValue *settings);
GhbValue* ghb_read_settings_file(const gchar *path);
void ghb_write_settings_file(const gchar *path, GhbValue *dict);
GhbValue* ghb_create_copy_mask(GhbValue *settings);
GhbValue* ghb_settings_to_preset(GhbValue *settings);
void ghb_preset_menu_button_refresh(signal_user_data_t *ud,
                                    const char *name, int type);

#endif // _GHB_PRESETS_H_
