/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_schedule_item.h"

#include "../../dataobj/environment.h"
#include "../../display/simgraph.h"


gui_schedule_entry_number_t::gui_schedule_entry_number_t(uint number_, uint8 p_col, uint8 style_)
{
	number = number_ + 1;
	style = style_;
	p_color_idx = p_col;
	set_size(scr_size(D_ENTRY_NO_WIDTH, D_ENTRY_NO_HEIGHT));

	lb_number.set_align(gui_label_t::centered);
	lb_number.set_size(scr_size(size.w, D_LABEL_HEIGHT));
	lb_number.set_pos(scr_coord(0,2));
	add_component(&lb_number);
}

void gui_schedule_entry_number_t::draw(scr_coord offset)
{
	const PIXVAL base_colval = color_idx_to_rgb(p_color_idx-p_color_idx%8 + env_t::gui_player_color_bright);
	      PIXVAL text_colval = color_idx_to_rgb(p_color_idx-p_color_idx%8 + env_t::gui_player_color_dark);
	if (number > 99) {
		size.w = proportional_string_width("000") + 6;
	}

	// draw the back image
	switch (style) {
		case number_style::halt:
			display_filled_roundbox_clip(pos.x+offset.x, pos.y+offset.y, size.w, D_ENTRY_NO_HEIGHT, base_colval, false);
			text_colval = color_idx_to_rgb(COL_WHITE);
			break;
		case number_style::interchange:
			display_filled_roundbox_clip(pos.x+offset.x,   pos.y+offset.y,   size.w,   D_ENTRY_NO_HEIGHT,   base_colval, false);
			display_filled_roundbox_clip(pos.x+offset.x+2, pos.y+offset.y+2, size.w-4, D_ENTRY_NO_HEIGHT-4, color_idx_to_rgb(COL_WHITE), false);
			break;
		case number_style::depot:
			for (uint8 i = 0; i < 3; i++) {
				const scr_coord_val w = (size.w/2) * (i+1)/4;
				display_fillbox_wh_clip_rgb(pos.x+offset.x + (size.w - w*2)/2, pos.y+offset.y + i, w*2, 1, color_idx_to_rgb(91), false);
			}
			display_fillbox_wh_clip_rgb(pos.x+offset.x, pos.y+offset.y + 3, size.w, D_ENTRY_NO_HEIGHT - 3, color_idx_to_rgb(91), false);
			text_colval = color_idx_to_rgb(COL_WHITE);
			break;
		case number_style::none:
			display_fillbox_wh_clip_rgb(pos.x+offset.x + (size.w - D_ENTRY_NO_WIDTH/2)/2, pos.y+offset.y, D_ENTRY_NO_WIDTH/2, D_ENTRY_NO_HEIGHT, base_colval, true);
			break;
		case number_style::waypoint:
			display_fillbox_wh_clip_rgb(pos.x+offset.x + D_ENTRY_NO_WIDTH/4, pos.y+offset.y, D_ENTRY_NO_WIDTH/2, D_ENTRY_NO_HEIGHT, base_colval, true);
			display_filled_circle_rgb(pos.x+offset.x + size.w/2, pos.y+offset.y + D_ENTRY_NO_HEIGHT/2, D_ENTRY_NO_HEIGHT/2, base_colval);
			break;
		default:
			display_fillbox_wh_clip_rgb(pos.x+offset.x, pos.y+offset.y, size.w, D_ENTRY_NO_HEIGHT, base_colval, false);
			text_colval = color_idx_to_rgb(COL_WHITE);
			break;
	}
	if (style != number_style::waypoint) {
		lb_number.buf().printf("%u", number);
		lb_number.set_color(text_colval);
	}
	lb_number.update();
	lb_number.set_fixed_width(size.w);

	gui_container_t::draw(offset);
}
