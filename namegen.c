#include "stdafx.h"
#include "ttd.h"


#define GETNUM(x, y) (((uint16)(seed >> x) * (y))>>16)

static void AppendPart(byte **buf, int num, const char *names)
{
	byte *s;

	while (--num>=0) {
		do names++; while (names[-1]);
	}
	
	for(s=*buf; (*s++ = *names++) != 0;) {}
	*buf = s - 1;
}

#define MK(x) x "\x0"

#define NUM_ENGLISH_1 4
static const char english_1[] = 
	MK("Great ")
	MK("Little ")
	MK("New ")
	MK("Fort ")
;

#define NUM_ENGLISH_2 26 
static const char english_2[] =
	MK("Wr")
	MK("B")
	MK("C")
	MK("Ch")
	MK("Br")
	MK("D")
	MK("Dr")
	MK("F")
	MK("Fr")
	MK("Fl")
	MK("G")
	MK("Gr")
	MK("H")
	MK("L")
	MK("M")
	MK("N")
	MK("P")
	MK("Pr")
	MK("Pl")
	MK("R")
	MK("S")
	MK("S")
	MK("Sl")
	MK("T")
	MK("Tr")
	MK("W")
;

#define NUM_ENGLISH_3 8
static const char english_3[] = 
	MK("ar")
	MK("a")
	MK("e")
	MK("in")
	MK("on")
	MK("u")
	MK("un")
	MK("en")
;

#define NUM_ENGLISH_4 7
static const char english_4[] = 
	MK("n")
	MK("ning")
	MK("ding")
	MK("d")
	MK("")
	MK("t")
	MK("fing")
;

#define NUM_ENGLISH_5 23
static const char english_5[] = 
	MK("ville")
	MK("ham")
	MK("field")
	MK("ton")
	MK("town")
	MK("bridge")
	MK("bury")
	MK("wood")
	MK("ford")
	MK("hall")
	MK("ston")
	MK("way")
	MK("stone")
	MK("borough")
	MK("ley")
	MK("head")
	MK("bourne")
	MK("pool")
	MK("worth")
	MK("hill")
	MK("well")
	MK("hattan")
	MK("burg")
;

#define NUM_ENGLISH_6 9
static const char english_6[] = 
	MK("-on-sea")
	MK(" Bay")
	MK(" Market")
	MK(" Cross")
	MK(" Bridge")
	MK(" Falls")
	MK(" City")
	MK(" Ridge")
	MK(" Springs")
;

static byte MakeEnglishCityName(byte *buf, uint32 seed)
{
	int i;
	byte result;
	byte *start;

	i = GETNUM(0, 54) - 50;
	if (i >= 0)
		AppendPart(&buf, i, english_1);
	
	start = buf;

	AppendPart(&buf, GETNUM(4, NUM_ENGLISH_2), english_2);
	AppendPart(&buf, GETNUM(7, NUM_ENGLISH_3), english_3);
	AppendPart(&buf, GETNUM(10, NUM_ENGLISH_4), english_4);
	AppendPart(&buf, GETNUM(13, NUM_ENGLISH_5), english_5);

	i = GETNUM(15, NUM_ENGLISH_6 + 60) - 60;

	result = 0;
	
	if (i >= 0) {
		if (i <= 1) result = NG_EDGE;
		AppendPart(&buf, i, english_6);
	}

	if (start[0]=='C' && (start[1] == 'e' || start[1] == 'i'))
		start[0] = 'K'; 

	/* FIXME: skip the banned words thing for now */
	return result;
}

#define NUM_GERMAN_1 23
static const char german_1[] = 
	MK("Bruns")
	MK("Lim")
	MK("Han")
	MK("Brun")
	MK("Ober")
	MK("Frank")
	MK("Rott")
	MK("Frei")
	MK("Ravens")
	MK("Schwein")
	MK("Osna")
	MK("Düsseldorf")
	MK("Wester")
	MK("Flens")
	MK("Mühl")
	MK("Heidel")
	MK("Inns")
	MK("Cloppen")
	MK("Pfung")
	MK("Michel")
	MK("Wert")
	MK("Wildes")
	MK("Freuden")
;

#define NUM_GERMAN_2 14
static const char german_2[] =
	MK("gen")
	MK("stadt")
	MK("heim")
	MK("burg")
	MK("haven")
	MK("ford")
	MK("feld")
	MK("mund")
	MK("münster")
	MK("hausen")
	MK("furt")
	MK("brück")
	MK("over")
	MK("wald")
;

static byte MakeGermanCityName(byte *buf, uint32 seed)
{
	AppendPart(&buf, GETNUM(0, NUM_GERMAN_1), german_1);
	AppendPart(&buf, GETNUM(5, NUM_GERMAN_2), german_2);
	return 0;
}

#define NUM_SPANISH_1 86
static const char spanish_1[] = 
	MK("Caracas")
	MK("Maracay")
	MK("Maracaibo")
	MK("Velencia")
	MK("El Dorado")
	MK("Morrocoy\x02")
	MK("Cata")
	MK("Cataito")
	MK("Ciudad Bolivar")
	MK("Barquisimeto")
	MK("Merida\x03")
	MK("Puerto Ordaz\x02")
	MK("Santa Elena")
	MK("San Juan")
	MK("San Luis")
	MK("San Rafael")
	MK("Santiago")
	MK("Barcelona")
	MK("Barinas")
	MK("San Cristobal")
	MK("San Fransisco")
	MK("San Martin")
	MK("Guayana")
	MK("San Carlos")
	MK("El Limon")
	MK("Coro")
	MK("Corocoro")
	MK("Puerto Ayacucho\x02")
	MK("Elorza")
	MK("Arismendi")
	MK("Trujillo")
	MK("Carupano")
	MK("Anaco")
	MK("Lima")
	MK("Cuzco")
	MK("Iquitos")
	MK("Callao")
	MK("Huacho")
	MK("Camana")
	MK("Puerto Chala\x02")
	MK("Santa Cruz")
	MK("Quito")
	MK("Cuenca")
	MK("Huacho")
	MK("Tulcan")
	MK("Esmereldas")
	MK("Ibarra")
	MK("San Lorenzo")
	MK("Macas")
	MK("Morana")
	MK("Machala")
	MK("Zamora")
	MK("Latacunga")
	MK("Tena")
	MK("Cochabamba")
	MK("Ascencion")
	MK("Magdalena")
	MK("Santa Ana")
	MK("Manoa")
	MK("Sucre")
	MK("Oruro")
	MK("Uyuni")
	MK("Potosi")
	MK("Tupiza")
	MK("La Quiaca")
	MK("Yacuiba")
	MK("San Borja")
	MK("Fuerte Olimpio")
	MK("Fortin Esteros")
	MK("Campo Grande")
	MK("Bogota")
	MK("El Banco")
	MK("Zaragosa")
	MK("Neiva")
	MK("Mariano")
	MK("Cali")
	MK("La Palma")
	MK("Andoas")
	MK("Barranca")
	MK("Montevideo")
	MK("Valdivia")
	MK("Arica")
	MK("Temuco")
	MK("Tocopilla")
	MK("Mendoza")
	MK("Santa Rosa");

static byte MakeSpanishCityName(byte *buf, uint32 seed)
{
	AppendPart(&buf, GETNUM(0, NUM_SPANISH_1), spanish_1);
	return 0;	
}

#define NUM_FRENCH_1 70
static const char french_1[] = 
	MK("Agincourt")
	MK("Lille")
	MK("Dinan")
	MK("Aubusson")
	MK("Rodez")
	MK("Bergerac")
	MK("Bordeaux")
	MK("Bayonne")
	MK("Montpellier")
	MK("Montelimar")
	MK("Valence")
	MK("Digne")
	MK("Nice")
	MK("Cannes")
	MK("St. Tropez")
	MK("Marseilles")
	MK("Narbonne")
	MK("SÞte")
	MK("Aurillac")
	MK("Gueret")
	MK("Le Creusot")
	MK("Nevers")
	MK("Auxerre")
	MK("Versailles")
	MK("Meaux")
	MK("ChÔlons")
	MK("CompiÞgne")
	MK("Metz")
	MK("Chaumont")
	MK("Langres")
	MK("Bourg")
	MK("Lyons")
	MK("Vienne")
	MK("Grenoble")
	MK("Toulon")
	MK("Rennes")
	MK("Le Mans")
	MK("Angers")
	MK("Nantes")
	MK("ChÔteauroux")
	MK("OrlÚans")
	MK("Lisieux")
	MK("Cherbourg")
	MK("Morlaix")
	MK("Cognac")
	MK("Agen")
	MK("Tulle")
	MK("Blois")
	MK("Troyes")
	MK("Charolles")
	MK("Grenoble")
	MK("ChambÚry")
	MK("Tours")
	MK("St. Brieuc")
	MK("St. Malo")
	MK("La Rochelle")
	MK("St. Flour")
	MK("Le Puy")
	MK("Vichy")
	MK("St. Valery")
	MK("Beaujolais")
	MK("Narbonne")
	MK("Albi")
	MK("St. Valery")
	MK("Biarritz")
	MK("BÚziers")
	MK("N¯mes")
	MK("Chamonix")
	MK("AngoulÛme")
	MK("Alenþon");

static byte MakeFrenchCityName(byte *buf, uint32 seed)
{
	AppendPart(&buf, GETNUM(0, NUM_FRENCH_1), french_1);
	return 0;
}

static byte MakeAmericanCityName(byte *buf, uint32 seed)
{
	// make american city names equal to english for now.
	return MakeEnglishCityName(buf, seed);
}

#define NUM_SILLY_1 88
static const char silly_1[] = 
	MK("Binky")
	MK("Blubber")
	MK("Bumble")
	MK("Crinkle")
	MK("Crusty")
	MK("Dangle")
	MK("Dribble")
	MK("Flippety")
	MK("Google")
	MK("Muffin")

	MK("Nosey")
	MK("Pinker")
	MK("Quack")
	MK("Rumble")
	MK("Sleepy")
	MK("Sliggles")
	MK("Snooze")
	MK("Teddy")
	MK("Tinkle")
	MK("Twister")

	MK("Pinker")
	MK("Hippo")
	MK("Itchy")
	MK("Jelly")
	MK("Jingle")
	MK("Jolly")
	MK("Kipper")
	MK("Lazy")
	MK("Frogs")
	MK("Mouse")

	MK("Quack")
	MK("Cheeky")
	MK("Lumpy")
	MK("Grumpy")
	MK("Mangle")
	MK("Fiddle")
	MK("Slugs")
	MK("Noodles")
	MK("Poodles")
	MK("Shiver")

	MK("Rumble")
	MK("Pixie")
	MK("Puddle")
	MK("Riddle")
	MK("Rattle")
	MK("Rickety")
	MK("Waffle")
	MK("Sagging")
	MK("Sausage")
	MK("Egg")

	MK("Sleepy")
	MK("Scatter")
	MK("Scramble")
	MK("Silly")
	MK("Simple")
	MK("Tricky")
	MK("Slippery")
	MK("Slimey")
	MK("Slumber")
	MK("Soggy")

	MK("Sliggles")
	MK("Splutter")
	MK("Sulky")
	MK("Swindle")
	MK("Swivel")
	MK("Tasty")
	MK("Tangle")
	MK("Toggle")
	MK("Trotting")
	MK("Tumble")

	MK("Snooze")
	MK("Water")
	MK("Windy")
	MK("Amble")
	MK("Bubble")
	MK("Cheery")
	MK("Cheese")
	MK("Cockle")
	MK("Cracker")
	MK("Crumple")

	MK("Teddy")
	MK("Evil")
	MK("Fairy")
	MK("Falling")
	MK("Fishy")
	MK("Fizzle")
	MK("Frosty")
	MK("Griddle")
;

#define NUM_SILLY_2 15
static const char silly_2[] = 
	MK("ton")
	MK("bury")
	MK("bottom")
	MK("ville")
	MK("well")
	MK("weed")
	MK("worth")
	MK("wig")
	MK("wick")
	MK("wood")
	
	MK("pool")
	MK("head")
	MK("burg")
	MK("gate")
	MK("bridge")
;


static byte MakeSillyCityName(byte *buf, uint32 seed)
{		
	AppendPart(&buf, GETNUM(0, NUM_SILLY_1), silly_1);
	AppendPart(&buf, GETNUM(16, NUM_SILLY_2),silly_2);
	return 0;
}


#define NUM_SWEDISH_1 4
static const char swedish_1[] =
	MK("Gamla ")
	MK("Lilla ")
	MK("Nya ")
	MK("Stora ");

#define NUM_SWEDISH_2 38
static const char swedish_2[] =
	MK("Boll")
	MK("Bor")
	MK("Ed")
	MK("En")
	MK("Erik")
	MK("Es")
	MK("Fin")
	MK("Fisk")
	MK("Grön")
	MK("Hag")
	MK("Halm")
	MK("Karl")
	MK("Kram")
	MK("Kung")
	MK("Land")
	MK("Lid")
	MK("Lin")
	MK("Mal")
	MK("Malm")
	MK("Marie")
	MK("Ner")
	MK("Norr")
	MK("Oskar")
	MK("Sand")
	MK("Skog")
	MK("Stock")
	MK("Stor")
	MK("Ström")
	MK("Sund")
	MK("Söder")
	MK("Tall")
	MK("Tratt")
	MK("Troll")
	MK("Upp")
	MK("Var")
	MK("Väster")
	MK("Ängel")
	MK("Öster");

#define NUM_SWEDISH_2A 42
static const char swedish_2a[] =
	MK("B")
	MK("Br")
	MK("D")
	MK("Dr")
	MK("Dv")
	MK("F")
	MK("Fj")
	MK("Fl")
	MK("Fr")
	MK("G")
	MK("Gl")
	MK("Gn")
	MK("Gr")
	MK("H")
	MK("J")
	MK("K")
	MK("Kl")
	MK("Kn")
	MK("Kr")
	MK("Kv")
	MK("L")
	MK("M")
	MK("N")
	MK("P")
	MK("Pl")
	MK("Pr")
	MK("R")
	MK("S")
	MK("Sk")
	MK("Skr")
	MK("Sl")
	MK("Sn")
	MK("Sp")
	MK("Spr")
	MK("St")
	MK("Str")
	MK("Sv")
	MK("T")
	MK("Tr")
	MK("Tv")
	MK("V")
	MK("Vr");

#define NUM_SWEDISH_2B 9
static const char swedish_2b[] =
	MK("a")
	MK("e")
	MK("i")
	MK("o")
	MK("u")
	MK("y")
	MK("å")
	MK("ä")
	MK("ö");

#define NUM_SWEDISH_2C 26
static const char swedish_2c[] =
	MK("ck")
	MK("d")
	MK("dd")
	MK("g")
	MK("gg")
	MK("l")
	MK("ld")
	MK("m")
	MK("n")
	MK("nd")
	MK("ng")
	MK("nn")
	MK("p")
	MK("pp")
	MK("r")
	MK("rd")
	MK("rk")
	MK("rp")
	MK("rr")
	MK("rt")
	MK("s")
	MK("sk")
	MK("st")
	MK("t")
	MK("tt")
	MK("v");

#define NUM_SWEDISH_3 32
static const char swedish_3[] =
	MK("arp")
	MK("berg")
	MK("boda")
	MK("borg")
	MK("bro")
	MK("bukten")
	MK("by")
	MK("byn")
	MK("fors")
	MK("hammar")
	MK("hamn")
	MK("holm")
	MK("hus")
	MK("hättan")
	MK("kulle")
	MK("köping")
	MK("lund")
	MK("löv")
	MK("sala")
	MK("skrona")
	MK("slätt")
	MK("spång")
	MK("stad")
	MK("sund")
	MK("svall")
	MK("svik")
	MK("såker")
	MK("udde")
	MK("valla")
	MK("viken")
	MK("älv")
	MK("ås");

static byte MakeSwedishCityName(byte *buf, uint32 seed)
{
	int i;

	i = GETNUM(0, 50 + NUM_SWEDISH_1) - 50;
	if (i >= 0) AppendPart(&buf, i, swedish_1);

	if (GETNUM(4, 5) >= 3)
		AppendPart(&buf, GETNUM(7, NUM_SWEDISH_2), swedish_2);
	else {
		AppendPart(&buf, GETNUM(7, NUM_SWEDISH_2A), swedish_2a);
		AppendPart(&buf, GETNUM(10, NUM_SWEDISH_2B), swedish_2b);
		AppendPart(&buf, GETNUM(13, NUM_SWEDISH_2C), swedish_2c);
	}

	AppendPart(&buf, GETNUM(16, NUM_SWEDISH_3), swedish_3);

	return 0;
}


#define NUM_DUTCH_1 8
static const char dutch_1[] =
	MK("Nieuw ")
	MK("Oud ")
	MK("Groot ")
	MK("Zuid ")
	MK("Noord ")
	MK("Oost ")
	MK("West ")
	MK("Klein ");

#define NUM_DUTCH_2 57
static const char dutch_2[] = 
	MK("Hoog")
	MK("Laag")
	MK("Klein")
	MK("Groot")
	MK("Noorder")
	MK("Noord")
	MK("Zuider")
	MK("Zuid")
	MK("Ooster")
	MK("Oost")
	MK("Wester")
	MK("West")
	MK("Hoofd")
	MK("Midden")
	MK("Eind")
	MK("Amster")
	MK("Amstel")
	MK("Dord")
	MK("Rotter")
	MK("Haar")
	MK("Til")
	MK("Enk")
	MK("Dok")
	MK("Veen")
	MK("Leidsch")
	MK("Lely")
	MK("En")
	MK("Kaats")
	MK("U")
	MK("Maas")
	MK("Mar")
	MK("Bla")
	MK("Al")
	MK("Alk")
	MK("Eer")
	MK("Drie")
	MK("Ter")
	MK("Groes")
	MK("Goes")
	MK("Soest")
	MK("Coe")
	MK("Uit")
	MK("Zwaag")
	MK("Hellen")
	MK("Slie")
	MK("IJ")
	MK("Grubben")
	MK("Groen")
	MK("Lek")
	MK("Ridder")
	MK("Schie")
	MK("Olde")
	MK("Roose")
	MK("Haar")
	MK("Til")
	MK("Loos")
	MK("Hil");

#define NUM_DUTCH_3 20
static const char dutch_3[] = 
	MK("Drog")
	MK("Nat")
	MK("Valk")
	MK("Bob")
	MK("Dedem")
	MK("Kollum")
	MK("Best")
	MK("Hoend")
	MK("Leeuw")
	MK("Graaf")
	MK("Uithuis")
	MK("Purm")
	MK("Hard")
	MK("Hell")
	MK("Werk")
	MK("Spijk")
	MK("Vink")
	MK("Wams")
	MK("Heerhug")
	MK("Koning");
	

#define NUM_DUTCH_4 6
static const char dutch_4[] = 
	MK("e")
	MK("er")
	MK("el")
	MK("en")
	MK("o")
	MK("s");

#define NUM_DUTCH_5 56
static const char dutch_5[] = 
	MK("stad")
	MK("vorst")
	MK("dorp")
	MK("dam")
	MK("beek")
	MK("doorn")
	MK("zijl")
	MK("zijlen")
	MK("lo")
	MK("muiden")
	MK("meden")
	MK("vliet")
	MK("nisse")
	MK("daal")
	MK("vorden")
	MK("vaart")
	MK("mond")
	MK("zaal")
	MK("water")
	MK("duinen")
	MK("heuvel")
	MK("geest")
	MK("kerk")
	MK("meer")
	MK("maar")
	MK("hoorn")
	MK("rade")
	MK("wijk")
	MK("berg")
	MK("heim")
	MK("sum")
	MK("richt")
	MK("burg")
	MK("recht")
	MK("drecht")
	MK("trecht")
	MK("tricht")
	MK("dricht")
	MK("lum")
	MK("rum")
	MK("halen")
	MK("oever")
	MK("wolde")
	MK("veen")
	MK("hoven")
	MK("gast")
	MK("kum")
	MK("hage")
	MK("dijk")
	MK("zwaag")
	MK("pomp")
	MK("huizen")
	MK("bergen")
	MK("schede")
	MK("mere")
	MK("end");
	
static byte MakeDutchCityName(byte *buf, uint32 seed)
{
	int i;

	i = GETNUM(0, 50 + NUM_DUTCH_1) - 50;
	if (i >= 0) 
		AppendPart(&buf, i, dutch_1);

	i = GETNUM(6, 9);
	if(i > 4){
		AppendPart(&buf, GETNUM(9, NUM_DUTCH_2), dutch_2);
	} else {
		AppendPart(&buf, GETNUM(9, NUM_DUTCH_3), dutch_3);
		AppendPart(&buf, GETNUM(12, NUM_DUTCH_4), dutch_4);
	}
	AppendPart(&buf, GETNUM(15, NUM_DUTCH_5), dutch_5);

	return 0;
}

#define NUM_FINNISH_1 25
static const char finnish_1[] = 
	MK("Aijala")
	MK("Kisko")
	MK("Espoo")
	MK("Helsinki")
	MK("Tapiola")
	MK("Järvelä")
	MK("Lahti")
	MK("Kotka")
	MK("Hamina")
	MK("Loviisa")
	MK("Kouvola")
	MK("Tampere")
	MK("Kokkola")
	MK("Oulu")
	MK("Salo")
	MK("Malmi")
	MK("Pelto")
	MK("Koski")
	MK("Iisalmi")
	MK("Raisio")
	MK("Taavetti")
	MK("Joensuu")
	MK("Imatra")
	MK("Tapanila")
	MK("Pasila");
 
#define NUM_FINNISH_2a 26
static const char finnish_2a[] = 
	MK("Hiekka")
	MK("Haapa")
	MK("Mylly")
	MK("Kivi")
	MK("Lappeen")
	MK("Lohjan")
	MK("Savon")
	MK("Sauna")
	MK("Keri")
	MK("Uusi")
	MK("Vanha")
	MK("Lapin")
	MK("Kesä")
	MK("Kuusi")
	MK("Pelto")
	MK("Tuomi")
	MK("Pitäjän")
	MK("Terva")
	MK("Olki")
	MK("Heinä")
	MK("Kuusan")
	MK("Seinä")
	MK("Kemi")
	MK("Rova")
	MK("Martin")
	MK("Koivu");

#define NUM_FINNISH_2b 18
static const char finnish_2b[] = 
	MK("harju")
	MK("linna")
	MK("järvi")
	MK("kallio")
	MK("mäki")
	MK("nummi")
	MK("joki")
	MK("kylä")
	MK("lampi")
	MK("lahti")
	MK("metsä")
	MK("suo")
	MK("laakso")
	MK("niitty")
	MK("luoto")
	MK("hovi")
	MK("ranta")
	MK("koski");

static byte MakeFinnishCityName(byte *buf, uint32 seed)
{
// Select randomly if city name should consists of one or two parts.
	if (GETNUM(0, 15) >= 10)
		AppendPart(&buf, GETNUM(2, NUM_FINNISH_1), finnish_1); // One part
	else {
		AppendPart(&buf, GETNUM(2, NUM_FINNISH_2a), finnish_2a); // Two parts
		AppendPart(&buf, GETNUM(10, NUM_FINNISH_2b), finnish_2b);
	}
	return 0;
}

CityNameGenerator * const _city_name_generators[] = {
	MakeEnglishCityName,
	MakeFrenchCityName,
	MakeGermanCityName,
	MakeAmericanCityName,
	MakeSpanishCityName,
	MakeSillyCityName,
	MakeSwedishCityName,
	MakeDutchCityName,
	MakeFinnishCityName
};
