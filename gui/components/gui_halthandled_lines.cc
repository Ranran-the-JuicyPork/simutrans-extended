/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_halthandled_lines.h"

#include "../../simcolor.h"
#include "../../simworld.h"

#include "../../utils/cbuffer_t.h"

#include "../gui_theme.h"

#include "../../player/simplay.h"
#include "../../linehandle_t.h"
#include "../../simline.h"

gui_halthandled_lines_t::gui_halthandled_lines_t(halthandle_t halt)
{
	this->halt = halt;
}

void gui_halthandled_lines_t::draw(scr_coord offset)
{
	static karte_ptr_t welt;
	int xoff = 0;
	int yoff = 0;

	if (halt.is_bound()) {
		offset.x += pos.x;

		cbuffer_t buf;
		for (uint8 lt = 1; lt < simline_t::MAX_LINE_TYPE; lt++) {
			if (!halt->get_station_type() & simline_t::linetype_to_stationtype[lt]) {
				continue;
			}
			bool found = false;
			for (sint8 i = 0; i < MAX_PLAYER_COUNT; i++) {
				cbuffer_t count_buf;
				uint line_count = 0;
				if (!halt->registered_lines.empty()) {
					for (uint32 line_idx = 0; line_idx < halt->registered_lines.get_count(); line_idx++) {
						if (i == halt->registered_lines[line_idx]->get_owner()->get_player_nr()
							&& lt == halt->registered_lines[line_idx]->get_linetype())
						{
							line_count++;
							if (!found) {
								found = true;
								xoff += D_H_SPACE;
								display_color_img(halt->registered_lines[line_idx]->get_linetype_symbol(), offset.x + xoff - 23, offset.y + yoff - 42 + FIXED_SYMBOL_YOFF, 0, false, true);
								xoff += 21;
							}
						}
					}
				}
				if (line_count) {
					count_buf.clear();
					count_buf.append(line_count, 0);
					uint text_width = proportional_string_width(count_buf);
					display_filled_roundbox_clip(offset.x + xoff, offset.y + yoff+1, text_width + 6, LINEASCENT + 4, color_idx_to_rgb(welt->get_player(i)->get_player_color1()+3), true);
					display_proportional_clip_rgb(offset.x + xoff + 3, offset.y + yoff+2, count_buf, ALIGN_LEFT, color_idx_to_rgb(COL_WHITE), true);
					xoff += text_width + 5 + 2;
				}
				uint lineless_convoy_cnt = 0;
				if (!halt->registered_convoys.empty()) {
					for (uint32 c = 0; c < halt->registered_convoys.get_count(); c++) {
						if (i == halt->registered_convoys[c]->get_owner()->get_player_nr()
							&& lt == halt->registered_convoys[c]->get_schedule()->get_type())
						{
							lineless_convoy_cnt++;
							if (!found) {
								xoff += D_H_SPACE;
								found = true;
								display_color_img(halt->registered_convoys[c]->get_schedule()->get_schedule_type_symbol(), offset.x + xoff - 23, offset.y + yoff - 42, 0, false, true);
								xoff += 21;
							}
						}
					}
					if (lineless_convoy_cnt) {
						count_buf.clear();
						count_buf.append(lineless_convoy_cnt, 0);
						uint text_width = proportional_string_width(count_buf);
						display_fillbox_wh_clip_rgb(offset.x + xoff, offset.y + yoff+1, text_width + 4, LINEASCENT + 4, color_idx_to_rgb(COL_WHITE), true);
						display_proportional_clip_rgb(offset.x + xoff+2, offset.y + yoff+2, count_buf, ALIGN_LEFT, color_idx_to_rgb(welt->get_player(i)->get_player_color1() + 1), true);
						xoff += text_width + 5 + 2;
					}
				}
			}
		}
	}
	xoff += D_H_SPACE;
	scr_size size(xoff, yoff + LINEASCENT+4);
	if (size != get_size()) {
		set_size(size);
	}
}
