#ifndef dataobj_livery_scheme_h
#define dataobj_livery_scheme_h 
/*
  * @author: jamespetts, April 2011
  * This file is part of the Simutrans project under the artistic licence.
  * (see licence.txt)
  */

#include <string>

#include "../simtypes.h"
#include "../tpl/vector_tpl.h"

class loadsave_t;
class vehicle_desc_t;

struct livery_t
{
	std::string name;
	uint16 intro_date;
};

class livery_scheme_t
{
private:
	std::string scheme_name;
	vector_tpl<livery_t> liveries;
	uint16 retire_date;
	uint16 scheme_wt;

public:
	livery_scheme_t(const char* n, const uint16 date, const uint16 waytype_flags);
	
	const char* get_name() const { return scheme_name.c_str(); }
	// return livery scheme waytype flags
	uint16 get_waytype() const { return scheme_wt; }

	void add_livery(const char* name, uint16 intro)
	{
		livery_t liv = {name, intro};
		liveries.append(liv);
	}

	bool is_available(uint16 date) const
	{
		if(date == 0)
		{
			// Allow all schemes when the timeline is turned off.
			return true;
		}

		if(date > retire_date)
		{
			return false;
		}
		else
		{
			ITERATE(liveries, i)
			{
				if(date >= liveries.get_element(i).intro_date)
				{ 
					return true;
				}
			}
			
			return false;
		}
	}

	const char* get_latest_available_livery(uint16 date, const vehicle_desc_t* desc) const;

	void rdwr(loadsave_t *file);
};
#endif
