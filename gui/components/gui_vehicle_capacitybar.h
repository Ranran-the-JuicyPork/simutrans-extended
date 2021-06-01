/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_VEHICLE_CAPACITYBAR_H
#define GUI_COMPONENTS_GUI_VEHICLE_CAPACITYBAR_H


#include "gui_aligned_container.h"

#include "../../convoihandle_t.h"
#include "../../simconvoi.h"


 // UI TODO: Incorporate a mode without capacity display into the convoy assembler
class gui_convoy_loading_info_t : public gui_aligned_container_t
{
	convoihandle_t cnv;

	uint8 old_vehicle_count = 0;
	sint32 old_weight = -1;
	bool old_reversed = false;

	bool show_loading;

	void update_list();

public:
	gui_convoy_loading_info_t(convoihandle_t cnv, bool show_loading_info = true);

	void set_convoy(convoihandle_t c) { cnv = c; };

	void draw(scr_coord offset) OVERRIDE;
};



#endif
