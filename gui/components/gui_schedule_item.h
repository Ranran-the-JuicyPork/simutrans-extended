/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SCHEDULE_ITEM_H
#define GUI_COMPONENTS_GUI_SCHEDULE_ITEM_H


#include "gui_container.h"
#include "gui_label.h"


class gui_schedule_entry_number_t : public gui_container_t
{
	uint8 p_color_idx;
	uint8 style;
	uint number;
	gui_label_buf_t lb_number;
public:
	enum number_style {
		halt = 0,
		interchange,
		depot,
		waypoint,
		none
	};

	gui_schedule_entry_number_t(uint number, uint8 p_color_idx = 8, uint8 style_ = number_style::halt);

	void draw(scr_coord offset);

	void init(uint number_, uint8 color_idx, uint8 style_ = number_style::halt) { number = number_; p_color_idx = color_idx; style = style_; };

	void set_number_style(uint8 style_) { style = style_; };
	void set_color(uint8 color_idx) { p_color_idx = color_idx; };

	scr_size get_min_size() const OVERRIDE { return size; }
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};

#endif
