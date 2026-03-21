const byte _water_display_datas_1[] = {
	ADD_WORD(0xFDD),

	0,15,0,16,1,0x14,
	ADD_WORD(0x8FE8),
	
	0x80 
};


const byte _water_display_datas_2[] = {
	ADD_WORD(0xFDD),

	0,0,0,16,1,0x14, ADD_WORD(0xFEA),
	0,15,0,16,1,0x14, ADD_WORD(0x8FE6),

	0x80
};

const byte _water_display_datas_3[] = {
	ADD_WORD(0xFDD),

	15,0,0,1,0x10,0x14,ADD_WORD(0x8FE9),

	0x80
};

const byte _water_display_datas_4[] = {
	ADD_WORD(0xFDD),

	0,0,0,1,16,0x14, ADD_WORD(0xFEB),
	15,0,0,1,16,0x14, ADD_WORD(0x8FE7),

	0x80
};

const byte * const _water_display_datas[] = {
	_water_display_datas_1,
	_water_display_datas_2,
	_water_display_datas_3,
	_water_display_datas_4,
};

static const SpriteID _water_shore_sprites[15] = {
	0, 0xFDF, 0xFE0, 0xFE4, 0xFDE, 0, 0xFE2, 0, 0xFE1, 0xFE5, 0, 0, 0xFE3, 0, 0
};
