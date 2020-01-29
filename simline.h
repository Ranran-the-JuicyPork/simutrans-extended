/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMLINE_H
#define SIMLINE_H


#include "simtypes.h"
#include "simcolor.h"
#include "convoihandle_t.h"
#include "linehandle_t.h"
#include "simconvoi.h"
#include "simskin.h"

#include "tpl/minivec_tpl.h"
#include "tpl/vector_tpl.h"
#include "utils/plainstring.h"
#include "tpl/koordhashtable_tpl.h"
#include "dataobj/schedule.h"

#define MAX_MONTHS				12 // Max history
#define MAX_NON_MONEY_TYPES		4 // number of non money types in line's financial statistic

							// Exp|Std|Description
enum line_cost_t {
	LINE_CAPACITY =	0,			//  0 | 0 | the amount of ware that could be transported, theoretically
	LINE_TRANSPORTED_GOODS,		//  1 | 1 | the amount of ware that has been transported
	LINE_AVERAGE_SPEED,			//  2 |   | average speed of all convoys in the line
	LINE_COMFORT,				//  3 |   | the average comfort rating of all vehicles on this line (weighted by numbers)
	LINE_REVENUE,				//  4 | 3 | the income this line generated
	LINE_OPERATIONS,			//  5 | 4 | the cost of operations this line generated
	LINE_PROFIT,				//  6 | 5 | total profit of line
	LINE_CONVOIS,				//  7 | 2 | number of convois for this line
	LINE_DISTANCE,				//  8 | 6 | distance covered by all convois
	LINE_REFUNDS,				//  9 |   | Total refunds paid to passengers/goods owners desiring to use this line but kept waiting too long to do so.
	LINE_DEPARTURES,			// 10 |   | number of departures of convoys on this line from scheduled points
	LINE_DEPARTURES_SCHEDULED,	// 11 |   | number of departures scheduled on this line from scheduled departure points
	LINE_WAYTOLL,				// 12 | 8 |
	MAX_LINE_COST				// 13 | 9 | Total number of cost items
};

class karte_ptr_t;
class loadsave_t;
class player_t;
class schedule_t;

class simline_t {

public:
	enum linetype { line = 0, truckline = 1, trainline = 2, shipline = 3, airline = 4, monorailline=5, tramline=6, maglevline=7, narrowgaugeline=8, MAX_LINE_TYPE};

	enum line_fireight_group { all_ftype = 0, all_pas = 1, all_mail = 2, all_freight = 3 };

	enum states { line_normal_state = 0, line_no_convoys = 1, line_loss_making = 2, line_nothing_moved = 4, line_overcrowded = 8, line_missing_scheduled_slots = 16, line_has_obsolete_vehicles = 32, line_has_upgradeable_vehicles = 64	};

protected:
	schedule_t * schedule;
	player_t *player;
	linetype type;

	bool withdraw;

private:
	static karte_ptr_t welt;
	plainstring name;

	/**
	 * Handle for ourselves. Can be used like the 'this' pointer
	 * Initialized by constructors
	 * @author Hj. Malthaner
	 */
	linehandle_t self;

	/*
	 * the current state saved as color
	 * Meanings are BLACK (ok), WHITE (no convois), YELLOW (no vehicle moved), RED (last month income minus), BLUE (at least one convoi vehicle is obsolete)
	 */
	COLOR_VAL state_color;

	/*
	 * a list of all convoys assigned to this line
	 * @author hsiegeln
	 */
	vector_tpl<convoihandle_t> line_managed_convoys;

	/*
	 * @author hsiegeln
	 * a list of all catg_index, which can be transported by this line.
	 */
	minivec_tpl<uint8> goods_catg_index;

	// The classes of passengers/mail carried by this line
	// Cached to reduce recalculation times in the path
	// explorer.
	vector_tpl<uint8> passenger_classes_carried;
	vector_tpl<uint8> mail_classes_carried;

	/*
	 * struct holds new financial history for line
	 * @author hsiegeln
	 */
	sint64 financial_history[MAX_MONTHS][MAX_LINE_COST];

	/**
	 * creates empty schedule with type depending on line-type
	 */
	void create_schedule();

	void init_financial_history();

	/*
	 * whether the next convoy applied to this line should have its
	 * reverse_schedule flag set. Only applies to bidirectional schedules.
	 * @author yobbobandana
	 */
	bool start_reversed;

	uint16 livery_scheme_index;

	/**
	* The table of point-to-point average speeds.
	* @author jamespetts
	*/
	journey_times_map average_journey_times;

	// @author: suitougreentea
	times_history_map journey_times_history;

	uint8 state;

public:
	simline_t(player_t *player, linetype type);
	simline_t(player_t *player, linetype type, loadsave_t *file);

	~simline_t();

	linehandle_t get_handle() const { return self; }

	/*
	 * add convoy to route
	 * @author hsiegeln
	 */
	void add_convoy(convoihandle_t cnv, bool from_loading = false);

	/*
	 * remove convoy from route
	 * @author hsiegeln
	 */
	void remove_convoy(convoihandle_t cnv);

	/*
	 * get convoy
	 * @author hsiegeln
	 */
	convoihandle_t get_convoy(int i) const { return line_managed_convoys[i]; }

	/*
	 * return number of manages convoys in this line
	 * @author hsiegeln
	 */
	uint32 count_convoys() const { return line_managed_convoys.get_count(); }

	vector_tpl<convoihandle_t> const& get_convoys() const { return line_managed_convoys; }

	/*
	 * returns the state of the line
	 * @author prissi
	 */
	COLOR_VAL get_state_color() const { return state_color; }
	// This has multiple flags
	uint8 get_state() const { return state; }

	/*
	 * return schedule of line
	 * @author hsiegeln
	 */
	schedule_t * get_schedule() const { return schedule; }

	void set_schedule(schedule_t* schedule);

	/*
	 * get name of line
	 * @author hsiegeln
	 */
	char const* get_name() const { return name; }
	void set_name(const char *str) { name = str; }

	/*
	 * load or save the line
	 */
	void rdwr(loadsave_t * file);

	/**
	 * method to load/save linehandles
	 */
	static void rdwr_linehandle_t(loadsave_t *file, linehandle_t &line);

	/*
	 * register line with stop
	 */
	void register_stops(schedule_t * schedule);

	void finish_rd();

	/*
	 * unregister line from stop
	 */
	void unregister_stops(schedule_t * schedule);
	void unregister_stops();

	/*
	 * renew line registration for stops
	 */
	void renew_stops();

	// called after tiles are removed from stations to change the load of connected convois
	void check_freight();

	sint64* get_finance_history() { return *financial_history; }

	sint64 get_finance_history(int month, line_cost_t cost_type) const { return financial_history[month][cost_type]; }
	sint64 get_stat_converted(int month, int cost_type) const;

	void book(sint64 amount, line_cost_t cost_type)
	{
		if(cost_type != LINE_AVERAGE_SPEED && cost_type != LINE_COMFORT)
		{
			financial_history[0][cost_type] += amount;
		}
		else
		{
			// Average types
			if(rolling_average_count[cost_type] == 65535)
			{
				rolling_average_count[cost_type] /= 2;
				rolling_average[cost_type] /= 2;
			}
			rolling_average[cost_type] += (uint32)amount;
			rolling_average_count[cost_type] ++;
			const sint64 tmp = (sint64)rolling_average[cost_type] / (sint64)rolling_average_count[cost_type];
			financial_history[0][cost_type] = tmp;
		}
	}

	static line_cost_t convoi_to_line_catgory(convoi_t::convoi_cost_t convoi_cost_type);

	void new_month();

	linetype get_linetype() { return type; }
	static linetype get_linetype( const waytype_t wt );

	const minivec_tpl<uint8> &get_goods_catg_index() const { return goods_catg_index; }

	// recalculates the good transported by this line and (in case of changes) will start schedule recalculation
	void recalc_catg_index();

	void calc_classes_carried();

	bool carries_this_or_lower_class(uint8 catg, uint8 g_class);

	int get_replacing_convoys_count() const;

	// @author: jamespetts
	uint32 rolling_average[MAX_LINE_COST];
	uint16 rolling_average_count[MAX_LINE_COST];

	//@author: jamespetts
	bool has_overcrowded() const;

	void set_withdraw( bool yes_no );

	bool get_withdraw() const { return withdraw; }

	player_t *get_owner() const {return player;}
	void set_owner(player_t* value) { player = value; }

	void recalc_status();

	void set_livery_scheme_index (uint16 index) { livery_scheme_index = index; }
	uint16 get_livery_scheme_index() const { return livery_scheme_index; }
	void propogate_livery_scheme();

	inline journey_times_map& get_average_journey_times() { return average_journey_times; }

	inline times_history_map& get_journey_times_history() { return journey_times_history; }

	inline image_id get_linetype_symbol(linetype lt)
	{
		switch (lt)
		{
		case simline_t::truckline:
			return skinverwaltung_t::autohaltsymbol->get_image_id(0);
		case simline_t::trainline:
			return skinverwaltung_t::zughaltsymbol->get_image_id(0);
		case simline_t::shipline:
			return skinverwaltung_t::schiffshaltsymbol->get_image_id(0);
		case simline_t::airline:
			return skinverwaltung_t::airhaltsymbol->get_image_id(0);
		case simline_t::monorailline:
			return skinverwaltung_t::monorailhaltsymbol->get_image_id(0);
		case simline_t::tramline:
			return skinverwaltung_t::tramhaltsymbol->get_image_id(0);
		case simline_t::maglevline:
			return skinverwaltung_t::maglevhaltsymbol->get_image_id(0);
		case simline_t::narrowgaugeline:
			return skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0);
		case simline_t::line:
		case simline_t::MAX_LINE_TYPE:
		default:
			return IMG_EMPTY;
				break;
		}
		return IMG_EMPTY;
	}
	inline image_id get_linetype_symbol()
	{
		return get_linetype_symbol(type);
	}

	sint64 calc_departures_scheduled();
};


#endif
