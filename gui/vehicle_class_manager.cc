/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Vehicle class manager
 */

#include <stdio.h>

#include "vehicle_class_manager.h"

#include "../simconvoi.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../simworld.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../simline.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"

#include "components/gui_divider.h"
#include "components/gui_image.h"


#define L_CAR_NUM_CELL_WIDTH ( proportional_string_width(translator::translate("LOCO_SYM"))+proportional_string_width("188") )

// Since the state before editing is not recorded, it is returned to the initial (recomended) value
void gui_convoy_fare_class_changer_t::reset_fare_class()
{
	if (!cnv.is_bound()) { return; }
	if (cnv->get_owner() == world()->get_active_player() && !world()->get_active_player()->is_locked()) {
		for (uint8 veh = 0; veh < cnv->get_vehicle_count(); veh++) {
			vehicle_t *v = cnv->get_vehicle(veh);
			const uint8 g_classes = v->get_cargo_type()->get_number_of_classes();
			if (g_classes == 1) {
				continue; // no classes in this goods
			}
			// init all capacities in this vehicle
			for (uint8 c = 0; c < g_classes; c++) {
				v->set_class_reassignment(c, c);
			}
		}
		update_vehicles();
	}
}

gui_convoy_fare_class_changer_t::gui_convoy_fare_class_changer_t(convoihandle_t cnv)
{
	this->cnv = cnv;

	set_table_layout(1,0);
	set_margin(scr_size(D_H_SPACE, D_V_SPACE), scr_size(D_SCROLLBAR_WIDTH+D_H_SPACE, D_MARGIN_BOTTOM));
	init_class.init(button_t::roundbox, "reset_all_classes");
	init_class.set_tooltip("resets_all_classes_to_their_defaults");
	init_class.add_listener( this );
	add_component(&init_class);
	add_table(4,1)->set_alignment(ALIGN_TOP);
	{
		show_hide_accommodation_table.init(button_t::roundbox, "+");
		show_hide_accommodation_table.set_width(display_get_char_width('+') + gui_theme_t::gui_button_text_offset.w + gui_theme_t::gui_button_text_offset_right.x);
		show_hide_accommodation_table.add_listener(this);
		add_component(&show_hide_accommodation_table);

		lb_collapsed.set_text("fare_class_settings_based_on_accommodation");
		add_component(&lb_collapsed);

		cont_accommodation_table.set_table_layout(5,0);
		add_component(&cont_accommodation_table);
		new_component<gui_fill_t>();
	}
	end_table();

	new_component<gui_margin_t>(0,D_V_SPACE);

	cont_vehicle_table.set_table_layout(3,0);
	cont_vehicle_table.set_alignment(ALIGN_TOP);
	add_component(&cont_vehicle_table);

	update_vehicles();
}

void gui_convoy_fare_class_changer_t::update_vehicles()
{
	any_class = false;
	cont_vehicle_table.remove_all();
	if (cnv.is_bound()) {
		const PIXVAL owner_color = color_idx_to_rgb( cnv->get_owner()->get_player_color1() + env_t::gui_player_color_dark);

		cont_accommodation_table.remove_all();
		// header
		cont_accommodation_table.new_component_span<gui_label_t>("accommodation", owner_color, gui_label_t::left,2);
		cont_accommodation_table.new_component_span<gui_label_buf_t>(owner_color, gui_label_t::left,3)->buf().printf("%s / %s", translator::translate("Capacity"), translator::translate("Comfort"));
		// draw borders
		cont_accommodation_table.new_component_span<gui_divider_t>(2);
		cont_accommodation_table.new_component_span<gui_divider_t>(3);
		add_accommodation_rows(goods_manager_t::INDEX_PAS);
		add_accommodation_rows(goods_manager_t::INDEX_MAIL);

		// draw headers
		cont_vehicle_table.new_component<gui_label_t>("No.", owner_color, gui_label_t::left);
		cont_vehicle_table.new_component<gui_label_t>("Name", owner_color, gui_label_t::left);
		cont_vehicle_table.new_component<gui_label_buf_t>(owner_color, gui_label_t::left)->buf().printf("%s / %s", translator::translate("Capacity"), translator::translate("Comfort"));

		// draw borders
		cont_vehicle_table.new_component<gui_divider_t>()->init(scr_coord(0,0), L_CAR_NUM_CELL_WIDTH, LINESPACE*0.5);
		cont_vehicle_table.new_component<gui_divider_t>();
		cont_vehicle_table.new_component<gui_divider_t>();

		old_reversed = cnv->is_reversed();
		old_vehicle_count = cnv->get_vehicle_count();
		old_player_nr = world()->get_active_player_nr();
		const uint8 catering_level = cnv->get_catering_level(goods_manager_t::INDEX_PAS);

		for (uint8 veh = 0; veh < cnv->get_vehicle_count(); veh++) {
			vehicle_t *v = cnv->get_vehicle(veh);
			const vehicle_desc_t *desc = v->get_desc();
			const uint16 month_now = world()->get_timeline_year_month();
			const bool is_passenger_vehicle = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS;

			// vehicle bar
			//const PIXVAL veh_bar_color = desc->is_obsolete(month_now) ? COL_OBSOLETE : (desc->is_future(month_now) || desc->is_retired(month_now)) ? COL_OUT_OF_PRODUCTION : COL_SAFETY;
			//cont_vehicle_table.new_component<gui_vehicle_bar_t>(veh_bar_color, scr_size(D_LABEL_HEIGHT*4, D_LABEL_HEIGHT-2))->set_flags(desc->get_basic_constraint_prev(reversed), desc->get_basic_constraint_next(reversed), desc->get_interactivity());

			// 1: car number
			gui_label_buf_t *lb = cont_vehicle_table.new_component<gui_label_buf_t>(desc->has_available_upgrade(month_now) ? COL_UPGRADEABLE : SYSCOL_TEXT_WEAK, gui_label_t::centered);
			lb->buf().printf("%s%d", cnv->get_car_numbering(veh) < 0 ? translator::translate("LOCO_SYM") : "", abs(cnv->get_car_numbering(veh)));
			lb->set_fixed_width( L_CAR_NUM_CELL_WIDTH );
			lb->update();

			// 2: vehicle name
			lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
			lb->buf().printf("%s", translator::translate(desc->get_name()));
			lb->set_fixed_width(D_BUTTON_WIDTH*1.8);
			lb->set_tooltip(translator::translate(desc->get_name()));
			lb->update();

			// 3: cabin capacity table
			const uint8 g_classes = v->get_cargo_type()->get_number_of_classes();
			if (g_classes > 1) {
				bool is_lowest_class = true;
				cont_vehicle_table.add_table(5,0); // cabins row
				for (uint8 cy = 0; cy < g_classes; cy++) {
					if (desc->get_capacity(cy)) {
						if (is_lowest_class) {
							// 3-1: category symbol
							cont_vehicle_table.new_component<gui_image_t>((desc->get_total_capacity() || desc->get_overcrowded_capacity()) ? desc->get_freight_type()->get_catg_symbol() : IMG_EMPTY, 0, ALIGN_CENTER_V, true);
							is_lowest_class = false;
						}
						else {
							cont_vehicle_table.new_component<gui_empty_t>();
						}

						// 3-2: capacity of this accommodation class
						lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
						lb->buf().printf("%3d", desc->get_capacity(cy));
						if (is_passenger_vehicle && v->get_overcrowded_capacity(cy)) {
							lb->buf().printf(" (%d)", v->get_overcrowded_capacity(cy));
						}
						lb->set_fixed_width(proportional_string_width("8888(888)"));
						lb->update();

						// 3-3: comfort of this accommodation class
						if (is_passenger_vehicle) {
							lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::centered);
							lb->buf().printf(" %d ", desc->get_comfort(cy));
							lb->set_fixed_width(proportional_string_width(" 888 "));
							lb->set_tooltip(translator::translate("Comfort"));
							lb->update();
						}
						else {
							cont_vehicle_table.new_component<gui_margin_t>(proportional_string_width(" 888 "));
						}

						// 3-4: catering bonus
						if (catering_level && is_passenger_vehicle) {
							lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_UP_TRIANGLE, gui_label_t::left);
							lb->buf().printf("+%d ", v->get_comfort(catering_level, v->get_reassigned_class(cy)) - v->get_comfort(0, v->get_reassigned_class(cy)));
							lb->set_fixed_width(proportional_string_width("+88 "));
							lb->set_tooltip("catering bonus\nfor travelling");
							lb->update();
						}
						else {
							cont_vehicle_table.new_component<gui_margin_t>(catering_level ? proportional_string_width("+88 ") : 0);
						}

						// 3-5: Consecutive buttons
						cont_vehicle_table.new_component<gui_cabin_fare_changer_t>(v, cy);
					}
				}
				cont_vehicle_table.end_table();
			}
			else {
				// This category does not have any class
				cont_vehicle_table.add_table(5,0); // cabins row
				{
					// 3-1: category symbol
					cont_vehicle_table.new_component<gui_image_t>((desc->get_total_capacity() || desc->get_overcrowded_capacity()) ? desc->get_freight_type()->get_catg_symbol() : IMG_EMPTY, 0, ALIGN_CENTER_V, true);
					// 3-2
					lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left);
					if (desc->get_capacity()) {
						lb->buf().printf("%3d", desc->get_capacity());
						if (is_passenger_vehicle && desc->get_overcrowded_capacity()) {
							lb->buf().printf(" (%d)", desc->get_overcrowded_capacity());
						}
					}
					else {
						lb->buf().append("-");
					}
					lb->set_fixed_width(proportional_string_width("8888(888)"));
					lb->update();

					// 3-3: comfort
					if (is_passenger_vehicle) {
						lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::centered);
						lb->buf().printf(" %d ", desc->get_comfort());
						lb->set_fixed_width(proportional_string_width(" 888 "));
						lb->set_tooltip(translator::translate("Comfort"));
						lb->update();
					}
					else {
						cont_vehicle_table.new_component<gui_empty_t>();
					}

					// 3-4: catering bonus
					if (catering_level && is_passenger_vehicle) {
						lb = cont_vehicle_table.new_component<gui_label_buf_t>(SYSCOL_UP_TRIANGLE, gui_label_t::left);
						lb->buf().printf("+%d ", v->get_comfort(catering_level, 0) - v->get_comfort(0,0));
						lb->set_fixed_width(proportional_string_width("+88 "));
						lb->set_tooltip("catering bonus\nfor travelling");
						lb->update();
					}
					else {
						cont_vehicle_table.new_component<gui_margin_t>(catering_level ? proportional_string_width("+88 ") : 0);
					}

					// 3-5: empty (no buttons)
					cont_vehicle_table.new_component<gui_empty_t>();
				}
				cont_vehicle_table.end_table();
			}
		}
		const bool edit_enable = ((cnv->get_owner() == world()->get_active_player()) && !world()->get_active_player()->is_locked() && any_class);
		init_class.enable( edit_enable );

		cont_accommodation_table.set_visible(edit_enable && any_class && show_accommodation_table);
		show_hide_accommodation_table.set_visible(edit_enable && any_class);
		lb_collapsed.set_visible(edit_enable && any_class && !show_accommodation_table);

	}
}

// also check the convoy has any_class
void gui_convoy_fare_class_changer_t::add_accommodation_rows(uint8 catg)
{
	if (!cnv->get_goods_catg_index().is_contained(catg)) {
		return;
	}
	const uint8 g_classes = goods_manager_t::get_info_catg_index(catg)->get_number_of_classes();
	bool lowest=true;
	const uint8 catering_level = cnv->get_catering_level(catg);

	for (uint8 c = 0; c < g_classes; c++) {
		uint16 accomodation_capacity_sum = 0;
		uint8 min_comfort = 255;
		uint8 max_comfort = 0;
		bool multiple_classes =false;
		uint8 old_class = 255;
		for (uint8 i = 0; i < cnv->get_vehicle_count(); i++) {
			vehicle_t* veh = cnv->get_vehicle(i);
			if (veh->get_cargo_type()->get_catg_index() == catg && veh->get_accommodation_capacity(c)>0) {
				accomodation_capacity_sum += veh->get_accommodation_capacity(c);
				if (catg == goods_manager_t::INDEX_PAS) {
					min_comfort = min(min_comfort, veh->get_desc()->get_comfort(c));
					max_comfort = max(max_comfort, veh->get_desc()->get_comfort(c));
				}
				if (old_class == 255) {
					old_class = veh->get_reassigned_class(c);
				}
				else if(old_class != veh->get_reassigned_class(c)) {
					multiple_classes = true;
				}
			}
		}
		if (accomodation_capacity_sum) {
			if (lowest) {
				lowest = false;
				cont_accommodation_table.new_component<gui_image_t>(goods_manager_t::get_info_catg_index(catg)->get_catg_symbol(), 0, ALIGN_NONE, true);
			}
			else {
				cont_accommodation_table.new_component<gui_margin_t>(D_FIXED_SYMBOL_WIDTH);
			}
			cont_accommodation_table.new_component<gui_label_t>(goods_manager_t::get_translated_wealth_name(catg, c));
			cont_accommodation_table.new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::left)->buf().printf("%6u", accomodation_capacity_sum);

			if (catg==goods_manager_t::INDEX_PAS) {
				if (min_comfort == max_comfort) {
					cont_accommodation_table.new_component<gui_label_buf_t>(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::centered)->buf().append(min_comfort, 0);
				}
				else {
					cont_accommodation_table.new_component<gui_label_buf_t>(SYSCOL_TEXT_HIGHLIGHT, gui_label_t::centered)->buf().printf("%d - %d", min_comfort, max_comfort);
				}
			}
			else {
				cont_accommodation_table.new_component<gui_empty_t>();
			}

			cont_accommodation_table.new_component<gui_accommodation_fare_changer_t>(linehandle_t(), cnv, catg, c, multiple_classes ? -1 : old_class);

			any_class = true;
		}
	}
}

void gui_convoy_fare_class_changer_t::draw(scr_coord offset)
{
	if (!cnv.is_bound()) { return; }
	if(cnv->is_reversed() != old_reversed || cnv->get_vehicle_count() != old_vehicle_count || old_player_nr != world()->get_active_player_nr() ) {
		update_vehicles();
		set_size(get_size());
	}
	gui_aligned_container_t::draw(offset);
}

bool gui_convoy_fare_class_changer_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (!cnv.is_bound()) { return false; }
	if (comp == &init_class && cnv->get_owner() == world()->get_active_player() && !world()->get_active_player()->is_locked()) {
		reset_fare_class();
		return true;
	}
	else if (comp == &show_hide_accommodation_table) {
		show_accommodation_table = !show_accommodation_table;
		show_hide_accommodation_table.set_text(show_accommodation_table ? "-" : "+");
		show_hide_accommodation_table.pressed = show_accommodation_table;
		//reset_min_windowsize();
		if (show_accommodation_table) {
			update_vehicles();
		}
		else {
			lb_collapsed.set_visible(true);
			cont_accommodation_table.set_visible(false);
		}
	}
	else {
		update_vehicles();
	}
	return false;
}


gui_cabin_fare_changer_t::gui_cabin_fare_changer_t(vehicle_t *v, uint8 original_class)
{
	vehicle = v;
	cabin_class = original_class;
	const vehicle_desc_t *desc = vehicle->get_desc();

	set_table_layout(1,0);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);
	{
		const uint8 g_classes = vehicle->get_cargo_type()->get_number_of_classes();
		const bool is_passenger_vehicle = vehicle->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS;

		add_table(g_classes+1,1)->set_spacing(scr_size(0,0));
		{

			for (uint8 cx = 0; cx<g_classes; cx++) {
				char *button_text = new char[32]();
				sprintf(button_text, "%s%s", cabin_class == cx ? "*": "",
					goods_manager_t::get_translated_wealth_name(vehicle->get_cargo_type()->get_catg_index(), cx));

				buttons[cx].init(cx==0 ? button_t::roundbox_left_state : cx==g_classes-1 ? button_t::roundbox_right_state : button_t::roundbox_middle_state, button_text);
				if (vehicle->get_reassigned_class(cabin_class) == cx) {
					buttons[cx].pressed = true;
				}
				buttons[cx].set_width( scr_coord_val(D_BUTTON_WIDTH*0.8) );
				buttons[cx].enable( buttons[cx].pressed || (vehicle->get_owner()==world()->get_active_player() && !world()->get_active_player()->is_locked()));
				buttons[cx].add_listener(this);
				add_component(&buttons[cx]);
			}
			new_component<gui_fill_t>();
		}
		end_table();
	}
}

void gui_cabin_fare_changer_t::draw(scr_coord offset)
{
	const uint8 g_classes = vehicle->get_cargo_type()->get_number_of_classes();
	for (uint8 cx = 0; cx < g_classes; cx++) {
		buttons[cx].pressed = (vehicle->get_reassigned_class(cabin_class) == cx);
	}
	set_size(get_size());
	gui_aligned_container_t::draw(offset);
}

bool gui_cabin_fare_changer_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if(vehicle->get_owner() == world()->get_active_player() && !world()->get_active_player()->is_locked()) {
		for (uint8 i = 0; i < vehicle->get_cargo_type()->get_number_of_classes(); i++) {
			if (&buttons[i] == comp) {
				vehicle->set_class_reassignment(cabin_class, i);
				buttons[i].pressed = true;
			}
			else {
				buttons[i].pressed = false;
			}
		}
		return true;
	}
	return false;
}




gui_accommodation_fare_changer_t::gui_accommodation_fare_changer_t(linehandle_t line_, convoihandle_t cnv_, uint8 goods_catg_index, uint8 original_class, uint8 fare_class_)
{
	line = line_;
	cnv = cnv_;
	catg = goods_catg_index;
	accommodation_class = original_class;
	fare_class = fare_class_;
	old_temp = cnv->get_unique_fare_capacity(catg, fare_class);

	if (line.is_bound()) {
		player_nr = line->get_owner()->get_player_nr();
	}
	else if (cnv.is_bound()) {
		player_nr = cnv->get_owner()->get_player_nr();
	}

	set_table_layout(1,0);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);
	{
		const uint8 g_classes = goods_manager_t::get_info_catg_index(catg)->get_number_of_classes();

		add_table(g_classes+1,1)->set_spacing(scr_size(0,0));
		{
			for (uint8 cx = 0; cx<g_classes; cx++) {
				char *button_text = new char[32]();
				sprintf(button_text, "%s%s", accommodation_class == cx ? "*": "",
					goods_manager_t::get_translated_wealth_name(catg, cx));

				buttons[cx].init(cx==0 ? button_t::roundbox_left_state : cx==g_classes-1 ? button_t::roundbox_right_state : button_t::roundbox_middle_state, button_text);
				if (fare_class == cx) {
					buttons[cx].pressed = true;
				}
				buttons[cx].set_width( scr_coord_val(D_BUTTON_WIDTH*0.8) );
				buttons[cx].enable( buttons[cx].pressed || (player_nr ==world()->get_active_player()->get_player_nr() && !world()->get_active_player()->is_locked()));
				buttons[cx].add_listener(this);
				add_component(&buttons[cx]);
			}
			new_component<gui_fill_t>();
		}
		end_table();
	}
}


void gui_accommodation_fare_changer_t::draw(scr_coord offset)
{
	if (cnv.is_bound()) {
		if (fare_class == 255 || (fare_class != 255 && old_temp != cnv->get_unique_fare_capacity(catg, fare_class))) {
			update_button_state();
			old_temp = fare_class == 255 ? 0 : cnv->get_unique_fare_capacity(catg, fare_class);
		}
	}

	set_size(get_size());
	gui_aligned_container_t::draw(offset);
}


// It seems that the class assignment has been changed by an operation from another component, so it needs to be rechecked.
void gui_accommodation_fare_changer_t::update_button_state()
{
	fare_class = 255;
	for (uint8 i = 0; i < cnv->get_vehicle_count(); i++) {
		vehicle_t* veh = cnv->get_vehicle(i);
		if (veh->get_cargo_type()->get_catg_index() == catg && veh->get_accommodation_capacity(accommodation_class) > 0) {
			if (fare_class == 255) {
				fare_class = veh->get_reassigned_class(accommodation_class);
			}
			else if (fare_class != veh->get_reassigned_class(accommodation_class)) {
				// convoy has multiple classes
				fare_class = 255;
				break;
			}
		}
	}
	const uint8 g_classes = goods_manager_t::get_info_catg_index(catg)->get_number_of_classes();
	for (uint8 cx = 0; cx < g_classes; cx++) {
		buttons[cx].pressed = (fare_class == cx);
	}
}

bool gui_accommodation_fare_changer_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (player_nr == world()->get_active_player()->get_player_nr() && !world()->get_active_player()->is_locked()) {

		for (uint8 i = 0; i < goods_manager_t::get_info_catg_index(catg)->get_number_of_classes(); i++) {
			if (&buttons[i] == comp) {
				fare_class = i;
				buttons[i].pressed = true;
				if (line.is_bound()) {
					for (uint8 j = 0; j < line->count_convoys(); j++) {
						set_class_reassignment(line->get_convoy(j));
					}
				}
				else if (cnv.is_bound()) {
					set_class_reassignment(cnv);
					old_temp = cnv->get_unique_fare_capacity(catg, fare_class);
				}
			}
			else {
				buttons[i].pressed = false;
			}
		}
		return true;
	}
	return false;
}


void gui_accommodation_fare_changer_t::set_class_reassignment(convoihandle_t target_convoy)
{
	if (target_convoy.is_bound()) {
		for (uint8 v = 0; v < target_convoy->get_vehicle_count(); v++){
			vehicle_t* veh = cnv->get_vehicle(v);
			if (veh->get_cargo_type()->get_catg_index() != catg) { continue; }
			veh->set_class_reassignment(accommodation_class, fare_class);
		}
	}
}
