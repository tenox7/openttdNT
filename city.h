
struct City {
	TileIndex xy;

	// Current population of people and amount of houses.
	uint16 population;
	uint16 num_houses;
	
	// City name
	uint16 citynametype;
	uint32 citynameparts;
	
	// NOSAVE: Location of name sign, UpdateCityVirtCoord updates this. 
	ViewportSign sign;
	
	// Makes sure we don't build certain house types twice.
	byte flags12;

	// Which players have a statue?
	byte statues;

	// Sort index in listings
	byte sort_index_obsolete;

	// Player ratings as well as a mask that determines which players have a rating.
	byte have_ratings;
	int16 ratings[8];
		
	// Maximum amount of passengers and mail that can be transported.
	uint16 max_pass;
	uint16 max_mail;
	uint16 new_max_pass;
	uint16 new_max_mail;
	uint16 act_pass;
	uint16 act_mail;
	uint16 new_act_pass;
	uint16 new_act_mail;

	// Amount of passengers that were transported.
	byte pct_pass_transported;
	byte pct_mail_transported;

	// Amount of food and paper that was transported. Actually a bit mask would be enough.
	uint16 act_food;
	uint16 act_paper;
	uint16 new_act_food;
	uint16 new_act_paper;
	
	// Time until we rebuild a house.
	byte time_until_rebuild;

	// When to grow city next time.
	byte grow_counter;
	byte growth_rate;	

	// Fund buildings program in action?
	byte fund_buildings_months;
	
	// Fund road reconstruction in action?
	byte road_build_months;

	// Index in city array
	byte index;

	// NOSAVE: UpdateCityRadius updates this given the house count.
	uint16 radius[5];
};


void InitializeCity();
void ShowTownViewWindow(uint city);
void DeleteTown(City *c);
void ExpandTown(City *c);
bool GrowCity(City *c);
void ResetTownRatingsForPlayer(int player);
City *CreateRandomTown();

#define DEREF_CITY(i) (&_cities[i])
#define FOR_ALL_CITIES(c) for(c=_cities; c != endof(_cities); c++)

VARDEF City _cities[70];

VARDEF bool _town_sort_dirty;
VARDEF byte _town_sort_order;

VARDEF City *_cleared_city;
VARDEF int _cleared_city_rating;
