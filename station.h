#ifndef STATION_H
#define STATION_H

typedef struct GoodsEntry {
	uint16 waiting_acceptance;
	byte days_since_pickup;
	byte rating;
	byte enroute_from;
	byte enroute_time;
	byte last_speed;
	byte last_age;
} GoodsEntry;

struct Station {
	TileIndex xy;
	TileIndex bus_tile;
	TileIndex lorry_tile;
	TileIndex train_tile;
	TileIndex airport_tile;
	TileIndex dock_tile;
	City *city;
	byte station_platforms;
	byte alpha_order;
	uint16 string_id;

	ViewportSign sign;

	uint16 had_vehicle_of_type;
	
	byte time_since_load;
	byte time_since_unload;
	byte delete_ctr;
	byte owner;
	byte facilities;
	byte airport_type;
	byte truck_stop_status;
	byte bus_stop_status;
	byte blocked_months;

	uint16 airport_flags;
	uint16 index;

	// Bitmask of goods that will ever be delivered to this station from industries.
	uint16 accepted_goods;

	VehicleID last_vehicle;
	GoodsEntry goods[NUM_CARGO];
};

enum {
	FACIL_TRAIN = 1,
	FACIL_TRUCK_STOP = 2,
	FACIL_BUS_STOP = 4,
	FACIL_AIRPORT = 8,
	FACIL_DOCK = 0x10,
};

enum {
//	HVOT_PENDING_DELETE = 1<<0, // not needed anymore
	HVOT_TRAIN = 1<<1,
	HVOT_BUS = 1 << 2,
	HVOT_TRUCK = 1 << 3,
	HVOT_AIRCRAFT = 1<<4,
	HVOT_SHIP = 1 << 5,

	HVOT_BUOY = 1 << 6
};

// Airport types
enum {
	AT_SMALL = 0,
	AT_LARGE = 1,
	AT_HELIPORT = 2,
	AT_OILFIELD = 3,
};

void ModifyStationRatingAround(TileIndex tile, byte owner, int amount, uint radius);

uint MoveGoodsToStation(uint tile, int w, int h, int type, uint amount);
void ShowStationViewWindow(int station);
void UpdateAllStationVirtCoord();

VARDEF Station _stations[250];

#define DEREF_STATION(i) (&_stations[i])
#define FOR_ALL_STATIONS(st) for(st=_stations; st != endof(_stations); st++)


void GetProductionAroundTiles(uint *produced, uint tile, int w, int h);
void GetAcceptanceAroundTiles(uint *accepts, uint tile, int w, int h);

#endif /* STATION_H */
