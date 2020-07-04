// flags for options, etc.

enum {
	IMPORT_FROM_NETLIST_FILE	= 0x1,
	IMPORT_PARTS				= 0x2,
	IMPORT_NETS					= 0x4,
	KEEP_PARTS_AND_CON			= 0x8,
	KEEP_PARTS_NO_CON			= 0x10,
	KEEP_FP						= 0x20,
	KEEP_NETS					= 0x40,
	KEEP_TRACES					= 0x80,
	KEEP_STUBS					= 0x100,
	KEEP_AREAS					= 0x200,
	SAVE_BEFORE_IMPORT			= 0x400
};

enum {
	EXPORT_PARTS		= 0x1,
	EXPORT_NETS			= 0x2,
	EXPORT_VALUES		= 0x4
};

// netlist return flags
enum {
	FOOTPRINTS_NOT_FOUND = 1,
	NOT_PADSPCB_FILE
};
// undo record types
enum {
};
