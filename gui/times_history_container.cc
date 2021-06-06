/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "times_history_container.h"

#include "../simhalt.h"
#include "../simline.h"
#include "../dataobj/schedule.h"
#include "../display/viewport.h"


#include "components/gui_schedule_item.h"
#include "../player/simplay.h"
#include "components/gui_image.h"
#include "components/gui_divider.h"

#define L_TIME_6_DIGITS_WIDTH (proportional_string_width("88:88:88")+6) // May be shared with another gui
gui_times_history_t::gui_times_history_t(linehandle_t line_, convoihandle_t convoi_, bool line_reversed_display)
{
	line = line_;
	convoy = convoi_;
	reversed = line_reversed_display;

	set_table_layout(1,0);
	set_spacing(scr_size(1,0));
	init();
}


void gui_times_history_t::init()
{
	if (line.is_bound()) {
		schedule = line->get_schedule();
		map = &line->get_journey_times_history();
		player = line->get_owner();
		mirrored = schedule->is_mirrored();
		build_table();
	}
	else if (convoy.is_bound()) {
		schedule = convoy->get_schedule();
		map = &convoy->get_journey_times_history();
		player = convoy->get_owner();
		mirrored = schedule->is_mirrored();
		build_table();
	}
}

void gui_times_history_t::construct_data(slist_tpl<uint8> *schedule_indices, slist_tpl<departure_point_t *> *time_keys) {
	const minivec_tpl<schedule_entry_t>& entries = schedule->entries;
	const uint8 size = entries.get_count();

	if (size <= 1) return;

	if (mirrored) {
		sint16 first_halt_index = -1;
		sint16 last_halt_index = -1;

		for (sint16 i = 0; i < size - 1; i++) {
			const halthandle_t halt = haltestelle_t::get_halt(entries[i].pos, player);
			if (halt.is_bound()) {
				if (first_halt_index == -1 || i < first_halt_index) first_halt_index = i;
				if (last_halt_index == -1 || i > last_halt_index) last_halt_index = i;
				schedule_indices->append(i);
				time_keys->append(new departure_point_t(i, false));
			}
		}

		const halthandle_t halt_last = haltestelle_t::get_halt(entries[size - 1].pos, player);
		if (halt_last.is_bound()) {
			last_halt_index = size - 1;
			schedule_indices->append(size - 1);
			time_keys->append(new departure_point_t(size - 1, true));
		}

		for (sint16 i = size - 2; i >= 1; i--) {
			const halthandle_t halt = haltestelle_t::get_halt(entries[i].pos, player);
			if (halt.is_bound()) {
				schedule_indices->append(i);
				time_keys->append(new departure_point_t(i, true));
			}
		}

		const halthandle_t halt_first = haltestelle_t::get_halt(entries[0].pos, player);
		if (halt_first.is_bound()) {
			schedule_indices->append(0);
		}
	}
	else if (reversed) {
		sint16 first_halt_index = -1;
		for (sint16 i = size - 1; i >= 0; i--) {
			const halthandle_t halt = haltestelle_t::get_halt(entries[i].pos, player);
			if (halt.is_bound()) {
				if (first_halt_index == -1 || i > first_halt_index) first_halt_index = i;
				schedule_indices->append(i);
				time_keys->append(new departure_point_t(i, true));
			}
		}
		schedule_indices->append(first_halt_index);
	}
	else {
		sint16 first_halt_index = -1;
		for (sint16 i = 0; i < size; i++) {
			const halthandle_t halt = haltestelle_t::get_halt(entries[i].pos, player);
			if (halt.is_bound()) {
				if (first_halt_index == -1 || i < first_halt_index) first_halt_index = i;
				schedule_indices->append(i);
				time_keys->append(new departure_point_t(i, false));
			}
		}
		schedule_indices->append(first_halt_index);
	}
}

void gui_times_history_t::build_table()
{
	if (convoy.is_bound()) {
		reversed = convoy->get_reverse_schedule();
	}
	remove_all();

	const scr_coord_val heading_pading_x= D_HEADING_HEIGHT*2;
	if (mirrored) {
		new_component<gui_heading_t>("bidirectional_order",
			color_idx_to_rgb(player->get_player_color1()+2), color_idx_to_rgb(COL_WHITE), 0)->set_width(proportional_string_width(translator::translate("bidirectional_order"))+heading_pading_x);
	}
	else if (reversed) {
		new_component<gui_heading_t>("reversed_unidirectional_order",
			color_idx_to_rgb(COL_WHITE), color_idx_to_rgb(player->get_player_color2()+2), 0)->set_width(proportional_string_width(translator::translate("reversed_unidirectional_order"))+heading_pading_x);
	}
	else {
		new_component<gui_heading_t>("normal_unidirectional_order",
			color_idx_to_rgb(COL_WHITE), color_idx_to_rgb(player->get_player_color1()+2), 0)->set_width(proportional_string_width(translator::translate("normal_unidirectional_order"))+heading_pading_x);
	}

	new_component<gui_margin_t>(0, D_V_SPACE);

	slist_tpl<uint8> *schedule_indices = new slist_tpl<uint8>();
	slist_tpl<departure_point_t *> *time_keys = new slist_tpl<departure_point_t *>();

	construct_data(schedule_indices, time_keys);

	const uint8 base_line_style = mirrored ? gui_colored_route_bar_t::line_style::doubled : gui_colored_route_bar_t::line_style::solid;

	if (!schedule_indices->get_count()) {
		return;
	}

	const minivec_tpl<schedule_entry_t>& entries = schedule->entries;
	add_table(8,0)->set_spacing(scr_size(D_H_SPACE, 0));
	{
		// header
		new_component<gui_empty_t>();
		new_component<gui_empty_t>();
		new_component<gui_label_t>("stations");
		new_component_span<gui_label_t>("latest_3", 3);
		new_component_span<gui_label_t>("average", 2);

		new_component<gui_divider_t>();
		new_component<gui_divider_t>()->init(scr_coord(0,0), D_ENTRY_NO_WIDTH);
		new_component<gui_divider_t>();
		new_component_span<gui_divider_t>(3);
		new_component_span<gui_divider_t>(2);

		for (uint32 i = 0; i < schedule_indices->get_count(); i++) {
			const uint8 entry_index = min(schedule_indices->at(i), entries.get_count()-1);
			const uint8 next_entry_index = i == schedule_indices->get_count()-1 ? 0 : min(schedule_indices->at(i+1), entries.get_count()-1);
			const schedule_entry_t entry = entries[entry_index];
			const halthandle_t halt = haltestelle_t::get_halt(entry.pos, player);

			const bool is_interchange = (halt->registered_lines.get_count() + halt->registered_convoys.get_count()) > 1;

			button_t *b = new_component<button_t>();
			b->set_typ(button_t::posbutton_automatic);
			b->set_targetpos(entry.pos.get_2d());

			new_component<gui_schedule_entry_number_t>(entry_index, halt->get_owner()->get_player_color1(),
				is_interchange ? gui_schedule_entry_number_t::number_style::interchange : gui_schedule_entry_number_t::number_style::halt, scr_size(D_ENTRY_NO_WIDTH, max(D_POS_BUTTON_HEIGHT,D_ENTRY_NO_HEIGHT)));
			new_component<gui_label_t>(halt->get_name());

			new_component_span<gui_empty_t>(5);

			// row2
			uint8 line_col_idx = reversed ? player->get_player_color2() : player->get_player_color1();
			if (i < schedule_indices->get_count() - 1) {
				uint32 distance_to_next_stop = shortest_distance(entry.pos.get_2d(), entries[next_entry_index].pos.get_2d()) * world()->get_settings().get_meters_per_tile();
				gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
				lb->buf().printf("%4.1f%s", (double)(distance_to_next_stop / 1000.0), "km");
				lb->set_fixed_width(L_TIME_6_DIGITS_WIDTH);
				lb->update();

				if (mirrored && i >= (schedule_indices->get_count()/2)) {
					line_col_idx = player->get_player_color2();
				}
				new_component<gui_colored_route_bar_t>(line_col_idx, base_line_style);

				new_component<gui_empty_t>();

				// times history
				times_history_data_t history;
				const times_history_data_t *retrieved_value = map->access(*time_keys->at(i));
				if (retrieved_value) { history = *retrieved_value; }

				for (uint8 j = 0; j < TIMES_HISTORY_SIZE; j++) {
					uint32 time = history.get_entry(j);
					lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
					if (time != 0) {
						char time_as_clock[32];
						world()->sprintf_time_tenths(time_as_clock, 32, time);
						lb->buf().append(time_as_clock);
					}
					else {
						lb->buf().append("--:--");
						lb->set_color(SYSCOL_TEXT_INACTIVE);
					}
					lb->set_fixed_width(L_TIME_6_DIGITS_WIDTH);
					lb->update();
				}
				uint32 time = history.get_average_seconds();
				lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
				if (time != 0) {
					char average_time_str[32];
					world()->sprintf_time_secs(average_time_str, 32, time);
					lb->buf().printf("%s", average_time_str);
				}
				else {
					lb->buf().append(translator::translate("no_time_entry"));
					lb->set_color(SYSCOL_TEXT_INACTIVE);
				}
				lb->set_fixed_width(L_TIME_6_DIGITS_WIDTH);
				lb->update();

				// average speed
				if (time != 0) {
				lb = new_component<gui_label_buf_t>(SYSCOL_TEXT_WEAK, gui_label_t::centered);
					sint64 average_speed = 3600 * distance_to_next_stop / time / 100; // x10
					lb->buf().printf("(%.1fkm/h)", (float)average_speed / 10.0);
					lb->set_color(SYSCOL_TEXT_INACTIVE);
					lb->update();
				}
				else {
					new_component<gui_empty_t>();
				}

			}
			else {
				new_component<gui_empty_t>();
				new_component<gui_colored_route_bar_t>(line_col_idx, gui_colored_route_bar_t::line_style::dashed);
				new_component_span<gui_empty_t>(6);
			}
		}
	}
	end_table();
	new_component<gui_margin_t>(0, D_V_SPACE);

	if (line.is_bound()) {
		update_time = world()->get_ticks();
	}
	delete time_keys;
}

void gui_times_history_t::draw(scr_coord offset)
{
	if (convoy.is_bound()){
		if (!convoy->get_schedule()->matches(world(), schedule)) {
			init();
		}
		else if( old_current_stop != schedule->get_current_stop() ) {
			old_current_stop = schedule->get_current_stop();
			build_table();
		}
	}
	else if(line.is_bound()) {
		if (!line->get_schedule()->matches(world(), schedule)) {
			init();
		}
		else if(world()->get_ticks() - update_time > 10000) {
			build_table();
		}
	}

	set_size(get_size());
	gui_aligned_container_t::draw(offset);
}
