/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#if !defined(_GHB_PRESETS_H_)
#define _GHB_PRESETS_H_

#include "values.h"

void ghb_settings_save(signal_user_data_t *ud, const gchar *name);
void ghb_presets_load(signal_user_data_t *ud);
void ghb_update_from_preset(signal_user_data_t *ud, const gchar *key);
void ghb_settings_init(GhbValue *settings, const char *name);
void ghb_settings_close();
void ghb_globals_to_ui(signal_user_data_t *ud);
void ghb_prefs_load(signal_user_data_t *ud);
void ghb_prefs_to_ui(signal_user_data_t *ud);
void ghb_prefs_save(GhbValue *settings);
void ghb_pref_save(GhbValue *settings, const gchar *key);
void ghb_pref_set(GhbValue *settings, const gchar *key);
void ghb_prefs_store(void);
void ghb_save_queue(GhbValue *queue);
GhbValue* ghb_load_queue();
GhbValue* ghb_load_old_queue(int pid);
void ghb_remove_queue_file(void);
void ghb_remove_old_queue_file(int pid);
gchar* ghb_get_user_config_dir(gchar *subdir);
void ghb_settings_to_ui(signal_user_data_t *ud, GhbValue *dict);
void ghb_clear_presets_selection(signal_user_data_t *ud);
void ghb_select_preset(GtkBuilder *builder, const GhbValue *preset);
void ghb_select_default_preset(GtkBuilder *builder);
void ghb_presets_list_init(signal_user_data_t *ud, gint *indices, gint len);
GhbValue* ghb_parse_preset_path(const gchar *path);
gchar* ghb_preset_path_string(const GhbValue *path);
gboolean ghb_preset_is_custom(void);
gboolean ghb_lock_file(const gchar *name);
int ghb_find_pid_file();
void ghb_unlink_pid_file(int pid);
void ghb_write_pid_file();
GhbValue* ghb_get_current_preset(signal_user_data_t *ud);
GhbValue* ghb_get_current_preset_path(signal_user_data_t *ud);
void ghb_preset_to_settings(GhbValue *settings, GhbValue *preset);
void ghb_prefs_to_settings(GhbValue *settings);
void dump_preset_path(const gchar *msg, const GhbValue *path);
GhbValue* ghb_read_settings_file(const gchar *path);
void ghb_write_settings_file(const gchar *path, GhbValue *dict);

#endif // _GHB_PRESETS_H_
