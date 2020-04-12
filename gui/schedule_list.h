/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef gui_schedule_list_h
#define gui_schedule_list_h

#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "components/gui_convoiinfo.h"
#include "halt_list_stats.h"
#include "../simline.h"

class gui_convoy_location_info_t : public gui_world_component_t
{
private:
	linehandle_t line;
	inline COLOR_VAL get_convoy_arrow_color(int cnv_state) const {
		switch (cnv_state)
		{
		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::CAN_START_ONE_MONTH:
			return COL_ORANGE;
		case convoi_t::CAN_START_TWO_MONTHS:
		case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
			return COL_RED + 1;
		case convoi_t::OUT_OF_RANGE:
		case convoi_t::NO_ROUTE:
		case convoi_t::NO_ROUTE_TOO_COMPLEX:
			return COL_RED;
		case convoi_t::EMERGENCY_STOP:
			return COL_YELLOW;
		case convoi_t::DRIVING:
		default:
			return COL_GREEN;
		}
		return COL_BLACK;
	}

	uint8 mode = 0;

public:
	gui_convoy_location_info_t(linehandle_t line);

	void set_line(linehandle_t l) { line = l; }
	void set_mode(uint8 m) { mode = m; }

	void draw(scr_coord offset);
};


class player_t;
class schedule_list_gui_t : public gui_frame_t, public action_listener_t
{
private:
	player_t *player;

	button_t bt_new_line, bt_change_line, bt_delete_line, bt_withdraw_line, bt_line_class_manager, bt_times_history, bt_mode_convois, bt_mode_cnv_location;
	gui_container_t cont, cont_haltestellen, cont_charts, cont_convoys, cont_cnv_location;
	gui_scrollpane_t scrolly_convois, scrolly_haltestellen, scrolly_cnv_location;
	gui_scrolled_list_t scl;
	gui_speedbar_t filled_bar;
	gui_textinput_t inp_name, inp_filter;
	gui_label_t lbl_filter;
	gui_chart_t chart;
	button_t filterButtons[MAX_LINE_COST];
	gui_tab_panel_t tabs; // line selector
	gui_tab_panel_t info_tabs;
	gui_convoy_location_info_t cnv_location;

	// vector of convoy info objects that are being displayed
	vector_tpl<gui_convoiinfo_t *> convoy_infos;

	// vector of stop info objects that are being displayed
	vector_tpl<halt_list_stats_t *> stop_infos;

	gui_combobox_t livery_selector;

	sint32 selection, capacity, load, loadfactor;

	uint32 old_line_count;
	schedule_t *last_schedule;
	uint32 last_vehicle_count;

	// only show schedules containing ...
	char schedule_filter[512], old_schedule_filter[512];

	// so even japanese can have long enough names ...
	char line_name[512], old_line_name[512];

	// resets textinput to current line name
	// necessary after line was renamed
	void reset_line_name();

	// rename selected line
	// checks if possible / necessary
	void rename_line();

	void display(scr_coord pos);

	void update_lineinfo(linehandle_t new_line);

	linehandle_t line;

	vector_tpl<linehandle_t> lines;

	void build_line_list(int filter);

	static uint16 livery_scheme_index;
	vector_tpl<uint16> livery_scheme_indices;

	cbuffer_t tab_name;
	uint8 cnv_location_display_mode = 0;

public:
	/// last selected line per tab
	static linehandle_t selected_line[MAX_PLAYER_COUNT][simline_t::MAX_LINE_TYPE];


	schedule_list_gui_t(player_t* player_);
	~schedule_list_gui_t();
	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "Line Management"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_help_filename() const { return "linemanagement.txt"; }

	/**
	* Does this window need a min size button in the title bar?
	* @return true if such a button is needed
	* @author Hj. Malthaner
	*/
	bool has_min_sizer() const {return true;}

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	void draw(scr_coord pos, scr_size size);

	/**
	* Set window size and adjust component sizes and/or positions accordingly
	* @author Hj. Malthaner
	*/
	virtual void set_windowsize(scr_size size);

	bool infowin_event(event_t const*) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Select line and show its info
	 * @author isidoro
	 */
	void show_lineinfo(linehandle_t line);

	/**
	 * called after renaming of line
	 */
	void update_data(linehandle_t changed_line);

	// following: rdwr stuff
	void rdwr( loadsave_t *file );
	uint32 get_rdwr_id();

	enum convoy_location_display_mode_t { cnv_location_name = 0, cnv_location_payload, MAX_CONVOY_LOCATION_MODES };
};

#endif
