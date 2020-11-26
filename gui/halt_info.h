/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_INFO_H
#define GUI_HALT_INFO_H


#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_button.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_location_view_t.h"
#include "components/gui_tab_panel.h"
#include "components/action_listener.h"
#include "components/gui_chart.h"
#include "components/gui_image.h"
#include "components/gui_colorbox.h"
#include "components/gui_combobox.h"

#include "../utils/cbuffer_t.h"
#include "../simhalt.h"
#include "simwin.h"

class gui_departure_board_t;

/**
 * Helper class to show type symbols (train, bus, etc)
 */
class gui_halt_type_images_t : public gui_aligned_container_t
{
	halthandle_t halt;
	gui_image_t img_transport[9];
public:
	gui_halt_type_images_t(halthandle_t h);

	void draw(scr_coord offset) OVERRIDE;
};

/**
 * Main class: the station info window.
 */
class halt_info_t : public gui_frame_t, private action_listener_t
{
private:

	gui_aligned_container_t *container_top;
	gui_label_buf_t lb_capacity[3], lb_happy;
	gui_colorbox_t indicator_color;
	gui_halt_type_images_t *img_types;
	/**
	* Buffer for freight info text string.
	*/
	cbuffer_t freight_info;
	cbuffer_t tooltip_buf;
	gui_label_buf_t joined_buf;

	// departure stuff (departure and arrival times display)
	gui_departure_board_t *departure_board;

	// other UI definitions
	gui_aligned_container_t container_freight, container_chart;
	gui_textarea_t text_freight;
	gui_scrollpane_t scrolly_freight, scrolly_departure;

	gui_textinput_t input;
	gui_chart_t chart;
	location_view_t view;
	button_t detail_button;
	// button_t sort_button;
	gui_combobox_t freight_sort_selector;

	gui_button_to_chart_array_t button_to_chart;

	gui_tab_panel_t switch_mode;

	halthandle_t halt;
	char edit_name[320];

	void show_hide_classes(bool show); // ?

	void update_components();

	void init(halthandle_t halt);

public:
	enum sort_mode_t { by_destination = 0, by_via = 1, by_amount_via = 2, by_amount = 3, by_origin = 4, by_origin_sum = 5, by_destination_detil = 6, by_class_detail = 7, by_class_via = 8, SORT_MODES = 9 };
//	enum sort_mode_t { by_destination = 0, by_via = 1, by_amount_via = 2, by_amount = 3, by_origin = 4, by_origin_sum = 5, by_destination_detil = 6, by_transfer_time = 7, SORT_MODES = 8 };

	halt_info_t(halthandle_t halt = halthandle_t());

	virtual ~halt_info_t();

	const char * get_help_filename() const OVERRIDE {return "station.txt";}

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	koord3d get_weltpos(bool) OVERRIDE;

	bool is_weltpos() OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE;

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_halt_info; }
};

#endif
