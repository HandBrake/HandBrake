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

void ghb_settings_save(signal_user_data_t *ud, const gchar *name);
void ghb_presets_load(void);
void ghb_update_from_preset(signal_user_data_t *ud, 
		const gchar *folder, const gchar *name, const gchar *key);
void ghb_prefs_load(signal_user_data_t *ud);
void ghb_settings_init(signal_user_data_t *ud);
void ghb_settings_close();
void ghb_prefs_to_ui(signal_user_data_t *ud);
void ghb_prefs_save(GValue *settings);
void ghb_pref_save(GValue *settings, const gchar *key);
void ghb_set_preset_default(GValue *settings);
void ghb_save_queue(GValue *queue);
GValue* ghb_load_queue();
void ghb_remove_queue_file(void);;
gchar* ghb_get_user_config_dir();
void ghb_settings_to_ui(signal_user_data_t *ud, GValue *dict);
void ghb_clear_presets_selection(signal_user_data_t *ud);
void ghb_select_preset(GtkBuilder *builder, 
		const gchar *folder, const gchar *preset);
void ghb_presets_list_init( signal_user_data_t *ud, 
	GValue *presets, const gchar *parent_name, GtkTreeIter *parent);

#endif // _GHB_PRESETS_H_
