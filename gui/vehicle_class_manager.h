/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_VEHICLE_CLASS_MANAGER_H
#define GUI_VEHICLE_CLASS_MANAGER_H


/*
 * Convoi details window
 */

#include "gui_frame.h"
#include "simwin.h"
#include "components/gui_container.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "../convoihandle_t.h"

#include "../vehicle/simvehicle.h"

class scr_coord;


class gui_convoy_fare_class_changer_t : public gui_aligned_container_t, private action_listener_t
{
	convoihandle_t cnv;

	bool any_class = false;

	uint8 old_vehicle_count=0;
	bool old_reversed=false;
	uint8 old_player_nr;

	// expand/collapse things
	gui_aligned_container_t cont_accommodation_table;
	gui_label_t lb_collapsed;
	button_t show_hide_accommodation_table;
	bool show_accommodation_table = false;

	gui_aligned_container_t cont_vehicle_table;

	button_t init_class;

	void update_vehicles();
	void add_accommodation_rows(uint8 catg);

public:
	gui_convoy_fare_class_changer_t(convoihandle_t cnv);

	//void set_cnv(convoihandle_t c) { cnv = c; }

	void draw(scr_coord offset) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};


class gui_cabin_fare_changer_t : public gui_aligned_container_t, private action_listener_t
{
	vehicle_t *vehicle;
	uint8 cabin_class;

	button_t buttons[5];
	char *class_name_untranslated[5];

public:
	gui_cabin_fare_changer_t(vehicle_t *v, uint8 original_class);
	void draw(scr_coord offset) OVERRIDE;
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

class gui_accommodation_fare_changer_t : public gui_aligned_container_t, private action_listener_t
{
	convoihandle_t cnv;
	linehandle_t line;
	uint8 catg;
	uint8 accommodation_class;
	uint8 fare_class; // 255 = assigned multiple

	sint8 player_nr;

	sint16 old_temp = 0;

	button_t buttons[5];
	char *class_name_untranslated[5];

	void set_class_reassignment(convoihandle_t target_convoy);
	void update_button_state();

public:
	gui_accommodation_fare_changer_t(linehandle_t line, convoihandle_t cnv, uint8 goods_catg_index, uint8 original_class=0, uint8 current_fare_class=0);
	void draw(scr_coord offset) OVERRIDE;
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};


#endif
