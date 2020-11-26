/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_VEHICLE_BASE_H
#define VEHICLE_VEHICLE_BASE_H


#include "../obj/simobj.h"
#include "simroadtraffic.h"


class convoi_t;
class grund_t;
class overtaker_t;


class traffic_vehicle_t
{
	private:
		sint64 time_at_last_hop; // ticks
		uint32 dist_travelled_since_last_hop; // yards
		virtual uint32 get_max_speed() { return 0; } // returns y/t
	protected:
		inline void reset_measurements()
		{
			dist_travelled_since_last_hop = 0; //yards
			time_at_last_hop = world()->get_ticks(); //ticks
		}
		inline void add_distance(uint32 distance) { dist_travelled_since_last_hop += distance; } // yards
		void flush_travel_times(strasse_t*); // calculate travel times, write to way, reset measurements
};


/**
 * Base class for all moving things (vehicles, pedestrians, movingobj)
 */
class vehicle_base_t : public obj_t
{
	// BG, 15.02.2014: gr and weg are cached in enter_tile() and reset to NULL in leave_tile().
	grund_t* gr;
	weg_t* weg;
public:
	inline grund_t* get_grund() const
	{
		if (!gr)
			return welt->lookup(get_pos());
		return gr;
	}
	inline weg_t* get_weg() const
	{
		if (!weg)
		{
			// gr and weg are both initialized in enter_tile(). If there is a gr but no weg, then e.g. for ships there IS no way.
			if (!gr)
			{
				// get a local pointer only. Do not assign to instances gr that has to be done by enter_tile() only.
				grund_t* gr2 = get_grund();
				if (gr2)
					return gr2->get_weg(get_waytype());
			}
		}
		return weg;
	}
protected:
	// offsets for different directions
	static sint8 dxdy[16];

	// to make the length on diagonals configurable
	// Number of vehicle steps along a diagonal...
	// remember to subtract one when stepping down to 0
	static uint8 diagonal_vehicle_steps_per_tile;
	static uint8 old_diagonal_vehicle_steps_per_tile;
	static uint16 diagonal_multiplier;

	// [0]=xoff [1]=yoff
	static sint8 overtaking_base_offsets[8][2];

	/**
	 * Actual travel direction in screen coordinates
	 */
	ribi_t::ribi direction;

	// true on slope (make calc_height much faster)
	uint8 use_calc_height:1;

	// if true, use offsets to emulate driving on other side
	uint8 drives_on_left:1;

	/**
	* Thing is moving on this lane.
	* Possible values:
	* (Back)
	* 0 - sidewalk (going on the right side to w/sw/s)
	* 1 - road     (going on the right side to w/sw/s)
	* 2 - middle   (everything with waytype != road)
	* 3 - road     (going on the right side to se/e/../nw)
	* 4 - sidewalk (going on the right side to se/e/../nw)
	* (Front)
	*/
	uint8 disp_lane : 3;

	sint8 dx, dy;

	// number of steps in this tile (255 per tile)
	uint8 steps, steps_next;

	/**
	 * Next position on our path
	 */
	koord3d pos_next;

	/**
	 * Offsets for uphill/downhill.
	 * Have to be multiplied with -TILE_HEIGHT_STEP/2.
	 * To obtain real z-offset, interpolate using steps, steps_next.
	 */
	uint8 zoff_start:4, zoff_end:4;

	// cached image
	image_id image;

	// The current livery of this vehicle.
	// @author: jamespetts, April 2011
	std::string current_livery;

	/**
	 * this vehicle will enter passing lane in the next tile -> 1
	 * this vehicle will enter traffic lane in the next tile -> -1
	 * Unclear -> 0
	 * @author THLeaderH
	 */
	sint8 next_lane;

	/**
	 * Vehicle movement: check whether this vehicle can enter the next tile (pos_next).
	 * @returns NULL if check fails, otherwise pointer to the next tile
	 */
	virtual grund_t* hop_check() = 0;

	/**
	 * Vehicle movement: change tiles, calls leave_tile and enter_tile.
	 * @param gr pointer to ground of new position (never NULL)
	 */
	virtual void hop(grund_t* gr) = 0;

	virtual void update_bookkeeping(uint32 steps) = 0;

	void calc_image() OVERRIDE = 0;

	// check for road vehicle, if next tile is free
	vehicle_base_t *get_blocking_vehicle(const grund_t *gr, const convoi_t *cnv, const uint8 current_direction, const uint8 next_direction, const uint8 next_90direction, const private_car_t *pcar, sint8 lane_on_the_tile );

	// If true, two vehicles might crash by lane crossing.
	bool judge_lane_crossing( const uint8 current_direction, const uint8 next_direction, const uint8 other_next_direction, const bool is_overtaking, const bool forced_to_change_lane ) const;

	// only needed for old way of moving vehicles to determine position at loading time
	bool is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const;

	// Players are able to reassign classes of accommodation in vehicles manually
	// during the game. Track these reassignments here with this array.
	uint8 *class_reassignments;

public:
	// only called during load time: set some offsets
	static void set_diagonal_multiplier( uint32 multiplier, uint32 old_multiplier );
	static uint16 get_diagonal_multiplier() { return diagonal_multiplier; }
	static uint8 get_diagonal_vehicle_steps_per_tile() { return diagonal_vehicle_steps_per_tile; }

	static void set_overtaking_offsets( bool driving_on_the_left );

	// if true, this convoi needs to restart for correct alignment
	bool need_realignment() const;

	virtual uint32 do_drive(uint32 dist);	// basis movement code

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const OVERRIDE {return image;}

	sint16 get_hoff(const sint16 raster_width = 1) const;
	uint8 get_steps() const {return steps;} // number of steps pass on the current tile.
	uint8 get_steps_next() const {return steps_next;} // total number of steps to pass on the current tile - 1. Mostly VEHICLE_STEPS_PER_TILE - 1 for straight route or diagonal_vehicle_steps_per_tile - 1 for a diagonal route.

	uint8 get_disp_lane() const { return disp_lane; }

	// to make smaller steps than the tile granularity, we have to calculate our offsets ourselves!
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const;

	/**
	 * Vehicle movement: calculates z-offset of vehicles on slopes,
	 * handles vehicles that are invisible in tunnels.
	 * @param gr vehicle is on this ground
	 * @note has to be called after loading to initialize z-offsets
	 */
	void calc_height(grund_t *gr = NULL);

	void rotate90() OVERRIDE;

	template<class K1, class K2>
	static ribi_t::ribi calc_direction(const K1& from, const K2& to)
	{
		return ribi_type(from, to);
	}

	ribi_t::ribi calc_set_direction(const koord3d& start, const koord3d& ende);
	uint16 get_tile_steps(const koord &start, const koord &ende, /*out*/ ribi_t::ribi &direction) const;

	ribi_t::ribi get_direction() const {return direction;}

	ribi_t::ribi get_90direction() const {return ribi_type(get_pos(), get_pos_next());}

	koord3d get_pos_next() const {return pos_next;}

	waytype_t get_waytype() const OVERRIDE = 0;

	void set_class_reassignment(uint8 original_class, uint8 new_class);

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return true; }

	/**
	 * Vehicle movement: enter tile, add this to the ground.
	 * @pre position (obj_t::pos) needs to be updated prior to calling this functions
	 * @return pointer to ground (never NULL)
	 */
	virtual void enter_tile(grund_t*);

	/**
	 * Vehicle movement: leave tile, release reserved crossing, remove vehicle from the ground.
	 */
	virtual void leave_tile();

	virtual overtaker_t *get_overtaker() { return NULL; }
	virtual convoi_t* get_overtaker_cv() { return NULL; }

#ifdef INLINE_OBJ_TYPE
protected:
	vehicle_base_t(typ type);
	vehicle_base_t(typ type, koord3d pos);
#else
	vehicle_base_t();

	vehicle_base_t(koord3d pos);
#endif

	virtual bool is_flying() const { return false; }
};


template<> inline vehicle_base_t* obj_cast<vehicle_base_t>(obj_t* const d)
{
	return d->is_moving() ? static_cast<vehicle_base_t*>(d) : 0;
}


#endif
