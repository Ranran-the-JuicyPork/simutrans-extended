/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "halt_detail.h"
#include "halt_info.h"
#include "components/gui_button_to_chart.h"

#include "../simworld.h"
#include "../simware.h"
#include "../simcolor.h"
#include "../simconvoi.h"
#include "../simintr.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../simline.h"

#include "../freight_list_sorter.h"

#include "../dataobj/schedule.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../vehicle/simvehicle.h"

#include "../utils/simstring.h"

#include "../descriptor/skin_desc.h"

#include "../player/simplay.h"


#define CHART_HEIGHT (100)
#define PAX_EVALUATIONS 5

#define L_BUTTON_WIDTH button_size.w
#define L_CHART_INDENT (66)

static karte_ptr_t welt;

// helper class to compute departure board
class dest_info_t {
public:
	bool compare( const dest_info_t &other ) const;
	halthandle_t halt;
	sint32 delta_ticks;
	convoihandle_t cnv;

	dest_info_t() : delta_ticks(0) {}
	dest_info_t( halthandle_t h, sint32 d_t, convoihandle_t c) : halt(h), delta_ticks(d_t), cnv(c) {}
	bool operator == (const dest_info_t &other) const { return ( this->cnv==other.cnv ); }
};

// class to compute and fill the departure board
class gui_departure_board_t : public gui_aligned_container_t
{
	static bool compare_hi(const dest_info_t &a, const dest_info_t &b) { return a.delta_ticks <= b.delta_ticks; }

	vector_tpl<dest_info_t> destinations;
	vector_tpl<dest_info_t> origins;

	uint32 calc_ticks_until_arrival( convoihandle_t cnv );

	void insert_image(convoihandle_t cnv);
public:
	halthandle_t halt;

	// if nothing changed, this is the next refresh to recalculate the content of the departure board
	sint8 next_refresh;

	gui_departure_board_t() : gui_aligned_container_t()
	{
		next_refresh = -1;
		set_table_layout(3,0);
	}

	void update_departures();

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};


static const char *sort_text[halt_info_t::SORT_MODES] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge",
	"origin (detail)",
	"origin (amount)",
	"destination (detail)",
	"wealth (detail)",
	"wealth (via)"/*,
	"transferring time"*/
};

static const char cost_type[MAX_HALT_COST][64] =
{
	"Happy",
	"Turned away",
	"Gave up waiting",
	"Too slow",
	"No route (pass.)",
	"Mail delivered",
	"No route (mail)",
	"hl_btn_sort_waiting",
	"Arrived",
	"Departed",
	"Convoys"
};

static const char cost_tooltip[MAX_HALT_COST][128] =
{
	"The number of passengers who have travelled successfully from this stop",
	"The number of passengers who have been turned away from this stop because it is overcrowded",
	"The number of passengers who had to wait so long that they gave up",
	"The number of passengers who decline to travel because the journey would take too long",
	"The number of passengers who could not find a route to their destination",
	"The amount of mail successfully delivered from this stop",
	"The amount of mail which could not find a route to its destination",
	"The number of passengers/units of mail/goods waiting at this stop",
	"The number of passengers/units of mail/goods that have arrived at this stop",
	"The number of passengers/units of mail/goods that have departed from this stop",
	"The number of convoys that have serviced this stop"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_TOO_WAITING,
	HALT_TOO_SLOW,
	HALT_NOROUTE,
	HALT_MAIL_DELIVERED,
	HALT_MAIL_NOROUTE,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED
};

#define COL_WAITING COL_DARK_TURQUOISE
#define COL_ARRIVED COL_LIGHT_BLUE

const uint8 cost_type_color[MAX_HALT_COST] =
{
	COL_HAPPY,
	COL_OVERCROWD,
	COL_TOO_WAITNG,
	COL_TOO_SLOW,
	COL_NO_ROUTE,
	COL_MAIL_DELIVERED,
	COL_MAIL_NOROUTE,
	COL_WAITING,
	COL_ARRIVED,
	COL_PASSENGERS,
	COL_TURQUOISE
};

struct type_symbol_t {
	haltestelle_t::stationtyp type;
	const skin_desc_t **desc;
};

const type_symbol_t symbols[] = {
	{ haltestelle_t::railstation, &skinverwaltung_t::zughaltsymbol },
	{ haltestelle_t::loadingbay, &skinverwaltung_t::autohaltsymbol },
	{ haltestelle_t::busstop, &skinverwaltung_t::bushaltsymbol },
	{ haltestelle_t::dock, &skinverwaltung_t::schiffshaltsymbol },
	{ haltestelle_t::airstop, &skinverwaltung_t::airhaltsymbol },
	{ haltestelle_t::monorailstop, &skinverwaltung_t::monorailhaltsymbol },
	{ haltestelle_t::tramstop, &skinverwaltung_t::tramhaltsymbol },
	{ haltestelle_t::maglevstop, &skinverwaltung_t::maglevhaltsymbol },
	{ haltestelle_t::narrowgaugestop, &skinverwaltung_t::narrowgaugehaltsymbol }
};


// helper class
gui_halt_type_images_t::gui_halt_type_images_t(halthandle_t h)
{
	halt = h;
	set_table_layout(lengthof(symbols), 1);
	set_alignment(ALIGN_LEFT | ALIGN_CENTER_V);
	assert( lengthof(img_transport) == lengthof(symbols) );
	// indicator for supplied transport modes
	haltestelle_t::stationtyp const halttype = halt->get_station_type();
	for(uint i=0; i < lengthof(symbols); i++) {
		if ( *symbols[i].desc ) {
			add_component(img_transport + i);
			img_transport[i].set_image( (*symbols[i].desc)->get_image_id(0));
			img_transport[i].enable_offset_removal(true);
			img_transport[i].set_visible( (halttype & symbols[i].type) != 0);
		}
	}
}

void gui_halt_type_images_t::draw(scr_coord offset)
{
	haltestelle_t::stationtyp const halttype = halt->get_station_type();
	for(uint i=0; i < lengthof(symbols); i++) {
		img_transport[i].set_visible( (halttype & symbols[i].type) != 0);
	}
	gui_aligned_container_t::draw(offset);
}

// main class
halt_info_t::halt_info_t(halthandle_t halt) :
		gui_frame_t("", NULL),
		departure_board( new gui_departure_board_t()),
		text_freight(&freight_info),
		scrolly_freight(&container_freight, true, true),
		scrolly_departure(departure_board, true, true),
		view(koord3d::invalid, scr_size(max(64, get_base_tile_raster_width()), max(56, get_base_tile_raster_width() * 7 / 8)))
{
	if (halt.is_bound()) {
		init(halt);
	}
}

void halt_info_t::init(halthandle_t halt)
{
	this->halt = halt;
	if(halt->get_station_type() & haltestelle_t::airstop && halt->has_no_control_tower())
	{
		sprintf(edit_name, "%s [%s]", halt->get_name(), translator::translate("NO CONTROL TOWER"));
		set_name(edit_name);
	}
	else {
		set_name(halt->get_name());
	}
	set_owner(halt->get_owner());

	halt->set_sortby( env_t::default_sortmode );

	/*
	const scr_size button_size(max(D_BUTTON_WIDTH, 100), D_BUTTON_HEIGHT);
	scr_size freight_selector_size = button_size;
	freight_selector_size.w = button_size.w+30;
	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);
	*/
	//const sint16 client_width = 3*(L_BUTTON_WIDTH + D_H_SPACE) + max(freight_selector_size.w, view.get_size().w );
	//const sint16 total_width = D_MARGIN_LEFT + client_width + D_MARGIN_RIGHT;

	//cursor.y += D_EDIT_HEIGHT + D_V_SPACE;
	set_table_layout(1,0);

	// top part
	add_table(2,2)->set_alignment(ALIGN_CENTER_H);
	{
		// input name
		tstrncpy(edit_name, halt->get_name(), lengthof(edit_name));
		input.set_text(edit_name, lengthof(edit_name));
		input.add_listener(this);
		add_component(&input);

		button.init(button_t::roundbox, "Details");
		button.set_tooltip("Open station/stop details");
		button.add_listener(this);
		add_component(&button);

		container_top = add_table(1,0);
		{
			// status images
			add_table(5,1)->set_alignment(ALIGN_CENTER_V);
			{
				add_component(&indicator_color);
				// indicator for enabled freight type
				img_enable[0].set_image(skinverwaltung_t::passengers->get_image_id(0));
				img_enable[1].set_image(skinverwaltung_t::mail->get_image_id(0));
				img_enable[2].set_image(skinverwaltung_t::goods->get_image_id(0));

				for(uint i=0; i<3; i++) {
					add_component(img_enable + i);
					img_enable[i].enable_offset_removal(true);
				}
				img_types = new_component<gui_halt_type_images_t>(halt);
			}
			end_table();
			// capacities
			add_table(6,1);
			{
				add_component(&lb_capacity[0]);
				if (welt->get_settings().is_separate_halt_capacities()) {
					new_component<gui_image_t>(skinverwaltung_t::passengers->get_image_id(0));
					add_component(&lb_capacity[1]);
					new_component<gui_image_t>(skinverwaltung_t::mail->get_image_id(0));
					add_component(&lb_capacity[2]);
					new_component<gui_image_t>(skinverwaltung_t::goods->get_image_id(0));
				}
			}
			end_table();
			add_component(&lb_happy);
		}
		end_table();

		add_component(&view);
		view.set_location(halt->get_basis_pos3d());

		new_component<gui_empty_t>();
	}
	end_table();


	// tabs: waiting, departure, chart

	add_component(&switch_mode);
	switch_mode.add_listener(this);
	switch_mode.add_tab(&scrolly_freight, translator::translate("Hier warten/lagern:"));

	// list of waiting cargo
	// sort mode
	container_freight.set_table_layout(1,0);
	container_freight.add_table(2,1);
	container_freight.new_component<gui_label_t>("Sort waiting list by");

	// hsiegeln: added sort_button
	// Ves: Made the sort button into a combobox

	freight_sort_selector.clear_elements();
	for (int i = 0; i < SORT_MODES; i++)
	{
		freight_sort_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
	}
	uint8 sortmode = env_t::default_sortmode < SORT_MODES ? env_t::default_sortmode: env_t::default_sortmode-2; // If sorting by accommodation in vehicles, we might want to sort by classes
	freight_sort_selector.set_selection(sortmode);
	halt->set_sortby(sortmode);
	freight_sort_selector.set_focusable(true);
	freight_sort_selector.set_highlight_color(1);
	freight_sort_selector.set_max_size(scr_size(D_BUTTON_WIDTH * 2, LINESPACE * 5 + 2 + 16));
	freight_sort_selector.add_listener(this);
	container_freight.add_component(&freight_sort_selector);
	container_freight.end_table();

	container_freight.add_component(&text_freight);

	// departure board
	switch_mode.add_tab(&scrolly_departure, translator::translate("Departure board"));
	departure_board->next_refresh = -1;
	departure_board->halt = halt;
	departure_board->update_departures();
	departure_board->set_size( departure_board->get_min_size() );

	// chart
	switch_mode.add_tab(&container_chart, translator::translate("Chart"));
	container_chart.set_table_layout(1,0);

	chart.set_min_size(scr_size(0, CHART_HEIGHT));
	chart.set_dimension(12, 10000);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	container_chart.add_component(&chart);

	container_chart.add_table(4,2);
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		uint16 curve = chart.add_curve(color_idx_to_rgb(cost_type_color[cost]), halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true, 0);

		button_t *b = container_chart.new_component<button_t>();
		b->init(button_t::box_state_automatic | button_t::flexible, cost_type[cost]);
		b->background_color = color_idx_to_rgb(cost_type_color[cost]);
		b->pressed = false;

		button_to_chart.append(b, &chart, curve);
	}
	container_chart.end_table();

	update_components();
	set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
}


halt_info_t::~halt_info_t()
{
	if(  halt.is_bound()  &&  strcmp(halt->get_name(),edit_name)  &&  edit_name[0]  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "h%u,%s", halt.get_id(), edit_name );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );
		welt->set_tool( tool, halt->get_owner() );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
	delete departure_board;
}


koord3d halt_info_t::get_weltpos(bool)
{
	return halt->get_basis_pos3d();
}


bool halt_info_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center(get_weltpos(false)));
}


void halt_info_t::update_components()
{
	indicator_color.set_color(halt->get_status_farbe());

	lb_capacity[0].buf().printf("%s: %u", translator::translate("Storage capacity"), halt->get_capacity(0));
	lb_capacity[0].update();
	lb_capacity[1].buf().printf("  %u", halt->get_capacity(1));
	lb_capacity[1].update();
	lb_capacity[2].buf().printf("  %u", halt->get_capacity(2));
	lb_capacity[2].update();

	if(  has_character( 0x263A )  ) {
		utf8 happy[4], unhappy[4];
		happy[ utf16_to_utf8( 0x263A, happy ) ] = 0;
		unhappy[ utf16_to_utf8( 0x2639, unhappy ) ] = 0;
		lb_happy.buf().printf(translator::translate("Passengers %d %s, %d %s, %d no route"), halt->get_pax_happy(), happy, halt->get_pax_unhappy(), unhappy, halt->get_pax_no_route());
	}
	else {
		lb_happy.buf().printf(translator::translate("Passengers %d %c, %d %c, %d no route"), halt->get_pax_happy(), 30, halt->get_pax_unhappy(), 31, halt->get_pax_no_route());
	}
	lb_happy.update();

	img_enable[0].set_visible(halt->get_pax_enabled());
	img_enable[1].set_visible(halt->get_mail_enabled());
	img_enable[2].set_visible(halt->get_ware_enabled());
	container_top->set_size( container_top->get_size());

	// buffer update now only when needed by halt itself => dedicated buffer for this
	int old_len = freight_info.len();
	halt->get_freight_info(freight_info);
	if(  old_len != freight_info.len()  ) {
		text_freight.recalc_size();
		container_freight.set_size(container_freight.get_min_size());
	}

	if (switch_mode.get_aktives_tab() == &scrolly_departure) {
		departure_board->update_departures();
	}
	set_dirty();
}

/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void halt_info_t::draw(scr_coord pos, scr_size size)
{
	assert(halt.is_bound());

	update_components();

		/* ex unique company name
		info_buf.printf("%s", halt->get_owner()->get_name());
		display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, PLAYER_FLAG|color_idx_to_rgb(halt->get_owner()->get_player_color1()+0), true);
		*/

		// passengers/mail evaluations
		/*
		if (halt->get_pax_enabled() || halt->get_mail_enabled()) {
			top += LINESPACE + D_V_SPACE;
			left = pos.x + D_MARGIN_LEFT;
			info_buf.printf("%s", translator::translate("Evaluation:"));
			display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			info_buf.clear();
			top += LINESPACE+2;

			const int ev_value_offset_left = display_get_char_max_width(":") + 12;
			uint16 ev_indicator_width = min( 255, get_windowsize().w - D_MARGIN_LEFT - D_MARGIN_RIGHT * 2 - ev_value_offset_left - view.get_size().w);
			PIXVAL color = color_idx_to_rgb(MN_GREY0);

			if (halt->get_pax_enabled()) {
				left = pos.x + D_MARGIN_LEFT + 10;
				int pax_ev_num[5] = { halt->haltestelle_t::get_pax_happy(), halt->haltestelle_t::get_pax_unhappy(), halt->haltestelle_t::get_pax_too_waiting(), halt->haltestelle_t::get_pax_too_slow(), halt->haltestelle_t::get_pax_no_route() };
				int pax_sum = pax_ev_num[0];
				for (uint8 i = 1; i < PAX_EVALUATIONS; i++) {
					pax_sum += pax_ev_num[i];
				}
				uint8 indicator_height = pax_sum < 100 ? D_INDICATOR_HEIGHT - 1 : D_INDICATOR_HEIGHT;
				if (pax_sum > 999)  { indicator_height++; }
				if (pax_sum > 9999) { indicator_height++; }
				// pax symbol
				display_color_img(skinverwaltung_t::passengers->get_image_id(0), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
				left += 11;

				info_buf.printf(": %d ", pax_sum);
				left += display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true) + display_get_char_max_width(":");
				info_buf.clear();

				// value + symbol + tooltip / error message
				if (!halt->get_ware_summe(goods_manager_t::passengers) && !halt->has_pax_user(2, true)) {
					// there is no passenger demand
					if (skinverwaltung_t::alerts) {
						display_color_img(skinverwaltung_t::alerts->get_image_id(2), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
					}
					left += 13;
					info_buf.printf("%s", translator::translate("This stop has no user"));
					display_proportional_rgb(left, top, info_buf, ALIGN_LEFT,  color_idx_to_rgb(MN_GREY0), true);
				}
				else if (!halt->get_ware_summe(goods_manager_t::passengers) && !halt->has_pax_user(2, false)) {
					// there is demand but it seems no passengers using
					if (skinverwaltung_t::alerts) {
						display_color_img(skinverwaltung_t::alerts->get_image_id(2), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
					}
					left += 13;
					info_buf.printf("%s", translator::translate("No passenger service"));
					display_proportional_rgb(left, top, info_buf, ALIGN_LEFT,  color_idx_to_rgb(MN_GREY0), true);
				}
				else {
					// passenger evaluation icons ok?
					if (skinverwaltung_t::pax_evaluation_icons) {
						info_buf.printf("(");
						for (int i = 0; i < PAX_EVALUATIONS; i++) {
							info_buf.printf("%d", pax_ev_num[i]);
							left += display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true) + 1;
							display_color_img(skinverwaltung_t::pax_evaluation_icons->get_image_id(i), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
							if (abs((int)(left - get_mouse_x())) < 14 && abs((int)(top + LINESPACE / 2 - get_mouse_y())) < LINESPACE / 2 + 2) {
								tooltip_buf.clear();
								tooltip_buf.printf("%s: %s", translator::translate(cost_type[i]), translator::translate(cost_tooltip[i]));
								win_set_tooltip(left, top + (int)(LINESPACE*1.6), tooltip_buf);
							}
							info_buf.clear();
							left += 11;
							if (i < PAX_EVALUATIONS - 1) {
								info_buf.printf(", ");
							}
						}
						info_buf.printf(")");
					}
					else {
						info_buf.printf("(");
						if (has_character(0x263A)) {
							utf8 happy[4];
							happy[utf16_to_utf8(0x263A, happy)] = 0;
							info_buf.printf("%s  ", happy);
						}
						else {
							info_buf.printf("%c  ", 30);
						}
						for (int i = 0; i < PAX_EVALUATIONS; i++) {
							info_buf.printf("%d", pax_ev_num[i]);
							if (i < PAX_EVALUATIONS - 1) {
								info_buf.printf(", ");
							}
						}
						if (has_character(0x2639)) {
							utf8 unhappy[4];
							unhappy[utf16_to_utf8(0x2639, unhappy)] = 0;
							info_buf.printf("  %s", unhappy);
						}
						else {
							info_buf.printf("%c  ", 31);
						}
						info_buf.printf(")");
					}
					display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				}
				info_buf.clear();

				// Evaluation ratio indicator
				top += LINESPACE + 1;
				left = pos.x + D_MARGIN_LEFT + 10 + ev_value_offset_left;
				info_buf.clear();
				if (!pax_sum) {
					color = color_idx_to_rgb(halt->has_pax_user(2, false) ? MN_GREY0 : MN_GREY1);
					display_fillbox_wh_clip_rgb(left, top, ev_indicator_width, indicator_height, color, true);
				}
				else
				{
					// calc and draw ratio indicator
					uint8 colored_width = 0;
					uint8 indicator_left = 0;
					sint8 fraction_sum = 0;
					for (int i = 0; i < PAX_EVALUATIONS; i++) {
						colored_width = ev_indicator_width * pax_ev_num[i] / pax_sum;
						if (int fraction = ((ev_indicator_width * pax_ev_num[i] * 10 / pax_sum) % 10)) {
							fraction_sum += fraction;
							if (fraction_sum > 5) {
								fraction_sum -= 10;
								colored_width++;
							}
						}
						if (pax_ev_num[i]) {
							display_fillbox_wh_clip_rgb(left + indicator_left, top, colored_width, indicator_height, color_idx_to_rgb(cost_type_color[i]), true);
						}
						indicator_left += colored_width;
					}
				}
				info_buf.clear();
				top += LINESPACE;
			}
			// Mail service evaluation
			if (halt->get_mail_enabled()) {
				left = pos.x + D_MARGIN_LEFT + 10;
				int mail_sum = halt->haltestelle_t::get_mail_delivered() + halt->haltestelle_t::get_mail_no_route();
				uint8 mail_delivered_factor = mail_sum ? (uint8)(((double)(halt->haltestelle_t::get_mail_delivered()) * 255.0 / (double)(mail_sum)) + 0.5) : 0;
				uint8 indicator_height = mail_sum < 100 ? D_INDICATOR_HEIGHT - 1 : D_INDICATOR_HEIGHT;
				if (mail_sum > 999)  { indicator_height++; }
				if (mail_sum > 9999) { indicator_height++; }

				// mail symbol
				display_color_img(skinverwaltung_t::mail->get_image_id(0), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
				left += 11;

				info_buf.printf(": %d ", mail_sum);
				left += display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true) + display_get_char_max_width(":");
				info_buf.clear();

				// value + symbol + tooltip / error message
				if (!halt->get_ware_summe(goods_manager_t::mail) && !halt->haltestelle_t::gibt_ab(goods_manager_t::get_info(1))) {
					if (skinverwaltung_t::alerts) {
						display_color_img(skinverwaltung_t::alerts->get_image_id(2), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
					}
					left += 13;
					if (!halt->has_mail_user(2, true)) {
						// It seems there are no mail users in the vicinity
						info_buf.printf("%s", translator::translate("This stop does not have mail customers"));
					}
					else if (!halt->has_mail_user(2, false)) {
						// This station seems not to be used by mail vehicles
						info_buf.printf("%s", translator::translate("No mail service"));
					}
					display_proportional_rgb(left, top, info_buf, ALIGN_LEFT,  color_idx_to_rgb(MN_GREY0), true);
				}
				else {
					// if symbols ok
					if (skinverwaltung_t::mail_evaluation_icons) {
						info_buf.printf("(%d", halt->haltestelle_t::get_mail_delivered());
						left += display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true) + 1;

						display_color_img(skinverwaltung_t::mail_evaluation_icons->get_image_id(0), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
						if (abs((int)(left - get_mouse_x())) < 14 && abs((int)(top + LINESPACE / 2 - get_mouse_y())) < LINESPACE / 2 + 2) {
							tooltip_buf.clear();
							tooltip_buf.printf("%s: %s", translator::translate(cost_type[5]), translator::translate(cost_tooltip[5]));
							win_set_tooltip(left, top + (int)(LINESPACE*1.6), tooltip_buf);
						}
						info_buf.clear();
						left += 11;

						info_buf.printf(", %d", halt->haltestelle_t::get_mail_no_route());
						left += display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true) + 1;
						info_buf.clear();
						display_color_img(skinverwaltung_t::mail_evaluation_icons->get_image_id(1), left, top + FIXED_SYMBOL_YOFF, 0, false, false);
						if (abs((int)(left - get_mouse_x())) < 14 && abs((int)(top + LINESPACE / 2 - get_mouse_y())) < LINESPACE / 2 + 2) {
							tooltip_buf.clear();
							tooltip_buf.printf("%s: %s", translator::translate(cost_type[6]), translator::translate(cost_tooltip[6]));
							win_set_tooltip(left, top + (int)(LINESPACE*1.6), tooltip_buf);
						}
						left += 11;
						info_buf.printf(")");
					}
					else {
						// pakset does not have symbols
						info_buf.printf("(");
						info_buf.printf(translator::translate("%d delivered, %d no route"), halt->haltestelle_t::get_mail_delivered(), halt->haltestelle_t::get_mail_no_route());
						info_buf.printf(")");
					}
					display_proportional_rgb(left, top, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				}
				// Evaluation ratio indicator
				top += LINESPACE + 1;
				left = pos.x + D_MARGIN_LEFT + 10 + ev_value_offset_left;
				info_buf.clear();
				if (!mail_sum) {
					color =  color_idx_to_rgb(halt->has_mail_user(2, false) ? MN_GREY0 : MN_GREY1);
					display_fillbox_wh_clip_rgb(left, top, ev_indicator_width, indicator_height, color, true);
				}
				else
				{
					display_fillbox_wh_clip_rgb(left, top, ev_indicator_width, indicator_height, color_idx_to_rgb(COL_MAIL_NOROUTE), true);
					if (mail_delivered_factor) {
						display_fillbox_wh_clip_rgb(left, top, ev_indicator_width*mail_delivered_factor / 255, indicator_height, color_idx_to_rgb(COL_MAIL_DELIVERED), true);
					}
				}

			}
		}*/

		// TODO: Display the status of the halt in written text and color
		// There exists currently no fixed states for stations, so those will have to be invented // Ves
		// Suggestions for states:
		// - No convoys serviced last month
		// - No goods where waiting last month
		// - No control tower (for airports)
		// - Station is overcrowded
		// - *Some* explanations to whatever triggers the red states,
		//   as it probably is more than one thing that triggers it, it would be nice to be specific

	gui_frame_t::draw(pos, size);
}


// a sophisticated guess of a convois arrival time, taking into account the braking too and the current convoi state
uint32 gui_departure_board_t::calc_ticks_until_arrival( convoihandle_t cnv )
{
	/* calculate the time needed:
	 *   tiles << (8+12) / (kmh_to_speed(max_kmh) = ticks
	 */
	uint32 delta_t = 0;
	sint32 delta_tiles = cnv->get_route()->get_count() - cnv->front()->get_route_index();
	uint32 kmh_average = (cnv->get_average_kmh()*900 ) / 1024u;

	// last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 25  ) {
		delta_tiles --;
		delta_t += 3276; // ( (1 << (8+12)) / kmh_to_speed(25) );
	}
	// second last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 50  ) {
		delta_tiles --;
		delta_t += 1638; // ( (1 << (8+12)) / kmh_to_speed(50) );
	}
	// third last braking tile
	if(  delta_tiles > 1  &&  kmh_average > 100  ) {
		delta_tiles --;
		delta_t += 819; // ( (1 << (8+12)) / kmh_to_speed(100) );
	}
	// waiting at signal?
	if(  cnv->get_state() != convoi_t::DRIVING  ) {
		// extra time for acceleration
		delta_t += kmh_average * 25;
	}
	delta_t += ( ((sint64)delta_tiles << (8+12) ) / kmh_to_speed( kmh_average ) );
	return delta_t;
}


// refreshes the departure string
void gui_departure_board_t::update_departures()
{
	if (!halt.is_bound()) {
		return;
	}

	if (--next_refresh >= 0) {
		return;
	}

	vector_tpl<dest_info_t> old_origins(origins);

	destinations.clear();
	origins.clear();

	const sint64 cur_ticks = welt->get_ticks();

	typedef inthashtable_tpl<uint16, sint64> const arrival_times_map; // Not clear why this has to be redefined here.
	const arrival_times_map& arrival_times = halt->get_estimated_convoy_arrival_times();
	const arrival_times_map& departure_times = halt->get_estimated_convoy_departure_times();

	convoihandle_t cnv;
	sint32 delta_t;
	const uint32 max_listings = 12;
	uint32 listing_count = 0;

	FOR(arrival_times_map, const& iter, arrival_times)
	{
		if(listing_count++ > max_listings)
		{
			break;
		}
		cnv.set_id(iter.key);
		if(!cnv.is_bound())
		{
			continue;
		}

		if(!cnv->get_schedule())
		{
			//XXX vehicle in depot.
			continue;
		}

		halthandle_t prev_halt = haltestelle_t::get_halt(cnv->front()->last_stop_pos, cnv->get_owner());
		delta_t = iter.value - cur_ticks;

		dest_info_t prev(prev_halt, max(delta_t, 0l), cnv);

		origins.insert_ordered(prev, compare_hi);
	}

	listing_count = 0;

	FOR(arrival_times_map, const& iter, departure_times)
	{
		if(listing_count++ > max_listings)
		{
			break;
		}
		cnv.set_id(iter.key);
		if(!cnv.is_bound())
		{
			continue;
		}

		if(!cnv->get_schedule())
		{
			//XXX vehicle in depot.
			continue;
		}

		halthandle_t next_halt = cnv->get_schedule()->get_next_halt(cnv->get_owner(), halt);
		delta_t = iter.value - cur_ticks;

		dest_info_t next(next_halt, max(delta_t, 0l), cnv);

		destinations.insert_ordered( next, compare_hi );
	}

	// fill the board
	remove_all();
	//slist_tpl<halthandle_t> exclude;
	if(  origins.get_count()>0  )
	{
		new_component_span<gui_label_t>("Arrivals from\n", 3);

		FOR( vector_tpl<dest_info_t>, hi, origins )
		{
			//if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  )
			//{
				//char timebuf[32];
				//welt->sprintf_ticks(timebuf, sizeof(timebuf), hi.delta_ticks );
				gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
				lb->buf().printf("%s", tick_to_string( hi.delta_ticks, false ) );
				lb->update();

				//insert_image(hi.cnv);

				new_component<gui_label_t>(hi.halt.is_bound() ? hi.halt->get_name() : "Unknown");
				//exclude.append( hi.halt );
			//}
		}
	}

	//exclude.clear();
	if(  destinations.get_count()>0  )
	{

		new_component_span<gui_label_t>("Departures to\n", 3);

		FOR( vector_tpl<dest_info_t>, hi, destinations ) {
			//if(  freight_list_sorter_t::by_via_sum != env_t::default_sortmode  ||  !exclude.is_contained( hi.halt )  )
			//{
				gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
				lb->buf().printf("%s", tick_to_string( hi.delta_ticks, false ) );
				lb->update();

				//insert_image(hi.cnv);

				new_component<gui_label_t>(hi.halt.is_bound() ? hi.halt->get_name() : "Unknown");
				//exclude.append( hi.halt );
			//}
		}
	}

	set_size(get_min_size());
	next_refresh = 5;
}


void gui_departure_board_t::insert_image(convoihandle_t cnv)
{
	gui_image_t *im = NULL;
	switch(cnv->front()->get_waytype()) {
		case road_wt:
		{
			if (cnv->front()->get_cargo_type() == goods_manager_t::passengers) {
				im = new_component<gui_image_t>(skinverwaltung_t::bushaltsymbol->get_image_id(0));
			}
			else {
				im = new_component<gui_image_t>(skinverwaltung_t::autohaltsymbol->get_image_id(0));
			}
			break;
		}
		case track_wt: {
			if (cnv->front()->get_desc()->get_waytype() == tram_wt) {
				im = new_component<gui_image_t>(skinverwaltung_t::tramhaltsymbol->get_image_id(0));
			}
			else {
				im = new_component<gui_image_t>(skinverwaltung_t::zughaltsymbol->get_image_id(0));
			}
			break;
		}
		case water_wt:       im = new_component<gui_image_t>(skinverwaltung_t::schiffshaltsymbol->get_image_id(0)); break;
		case air_wt:         im = new_component<gui_image_t>(skinverwaltung_t::airhaltsymbol->get_image_id(0)); break;
		case monorail_wt:    im = new_component<gui_image_t>(skinverwaltung_t::monorailhaltsymbol->get_image_id(0)); break;
		case maglev_wt:      im = new_component<gui_image_t>(skinverwaltung_t::maglevhaltsymbol->get_image_id(0)); break;
		case narrowgauge_wt: im = new_component<gui_image_t>(skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0)); break;
		default:             new_component<gui_empty_t>(); break;
	}
	if (im) {
		im->enable_offset_removal(true);
	}
}

/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool halt_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &button) { 			// details button pressed
		create_win( new halt_detail_t(halt), w_info, magic_halt_detail + halt.get_id() );
	} else if (comp == &freight_sort_selector) { 	// @author hsiegeln sort button pressed // @author Ves: changed button to combobox

		sint32 sort_mode = freight_sort_selector.get_selection();
		if (sort_mode < 0)
		{
			freight_sort_selector.set_selection(0);
			sort_mode = 0;
		}
		env_t::default_sortmode = (sort_mode_t)((int)(sort_mode) % (int)SORT_MODES);
		halt->set_sortby(env_t::default_sortmode);
	}
	else if(  comp == &input  ) {
		if(  strcmp(halt->get_name(),edit_name)  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "h%u,%s", halt.get_id(), edit_name );
			tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tool->set_default_param( buf );
			welt->set_tool( tool, halt->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tool;
		}
	}
	else if (comp == &switch_mode) {
		departure_board->next_refresh = -1;
	}

	return true;
}


void halt_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}


void halt_info_t::rdwr(loadsave_t *file)
{
	// window size
	scr_size size = get_windowsize();
	size.rdwr(file);
	// halt
	koord3d halt_pos;
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
	}
	halt_pos.rdwr( file );
	if(  file->is_loading()  ) {
		halt = welt->lookup( halt_pos )->get_halt();
		if (halt.is_bound()) {
			init(halt);
			reset_min_windowsize();
			set_windowsize(size);
		}
	}
	// sort
	file->rdwr_byte( env_t::default_sortmode );

	scrolly_freight.rdwr(file);
	scrolly_departure.rdwr(file);
	switch_mode.rdwr(file);

	// button-to-chart array
	button_to_chart.rdwr(file);

	if (!halt.is_bound()) {
		destroy_win( this );
	}
}
