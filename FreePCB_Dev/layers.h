// define standard drawing layers
//
#pragma once

#define MAX_LAYERS 32

enum
{
	// layout layers
	LAY_SELECTION = 0,
	LAY_BACKGND,
	LAY_VISIBLE_GRID,
	LAY_HILITE,
	LAY_DRC_ERROR,
	LAY_BOARD_OUTLINE,
	LAY_SCRIBING,
	LAY_REFINE_TOP,
	LAY_REFINE_BOT,
	LAY_RAT_LINE,
	LAY_SILK_TOP,
	LAY_SILK_BOTTOM,
	LAY_PAD_THRU,
	LAY_SM_TOP,
	LAY_SM_BOTTOM,
	LAY_TOP_COPPER,
	LAY_BOTTOM_COPPER,
	// invisible layers
	LAY_MASK_TOP = -100,	
	LAY_MASK_BOTTOM = -101,
	LAY_PASTE_TOP = -102,
	LAY_PASTE_BOTTOM = -103
};

enum
{
	// footprint layers
	LAY_FP_SELECTION = 0,
	LAY_FP_BACKGND,
	LAY_FP_VISIBLE_GRID,
	LAY_FP_HILITE,
	LAY_FP_SILK_TOP,
	LAY_FP_SILK_BOTTOM,
	LAY_FP_CENTROID,
	LAY_FP_DOT,
	LAY_FP_PAD_THRU,
	LAY_FP_TOP_MASK,
	LAY_FP_TOP_PASTE,
	LAY_FP_BOTTOM_MASK,
	LAY_FP_BOTTOM_PASTE,
	LAY_FP_TOP_COPPER,
	LAY_FP_INNER_COPPER,
	LAY_FP_BOTTOM_COPPER,
	NUM_FP_LAYERS
};

static char layer_str[32][64] = 
{ 
	"dragging",
	"background",
	"visible grid",
	"highlight",
	"DRC error",
	"board outline",
	"scribing",
	"top notes",
	"bot notes",
	"rat line",
	"top silk",
	"bottom silk",
	"thru pad",
	"top view",
	"bot view",
	"top copper",
	"bot copper",
	"inner 1",
	"inner 2",
	"inner 3",
	"inner 4",
	"inner 5",
	"inner 6",
	"inner 7",
	"inner 8",
	"inner 9",
	"inner 10",
	"inner 11",
	"inner 12",
	"inner 13",
	"inner 14",
	"undefined"
};

static char fp_layer_str[NUM_FP_LAYERS][64] = 
{ 
	"selection",
	"background",
	"visible grid",
	"highlight",
	"top silk",
	"bottom silk",
	"centroid",
	"adhesive",
	"thru pad",
	"top mask",
	"top paste",
	"bottom mask",
	"bottom paste",
	"top copper",
	"inner",
	"bottom"
};

static char layer_char[19] = "12345678QWERTYUIOP";
