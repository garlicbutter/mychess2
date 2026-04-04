#include "string.h"
#include "main.h"
#include "engine.h"
#include "render.h"

#define BYTES_PER_PIXEL (LV_COLOR_DEPTH / 8)
#define SQUARE_SIZE 30 // Assuming your resized 30x30 pieces

lv_obj_t *visual_pieces[64] = { NULL };
lv_obj_t *move_markers[64] = { NULL };

static uint8_t imgbuf1[ILI9341_LCD_PIXEL_WIDTH * ILI9341_LCD_PIXEL_HEIGHT / 10
		* BYTES_PER_PIXEL];

// local stuff
static void test_button_callback(lv_event_t *e);

void render_init(void) {
//	LVGL init
	lv_init();
	lv_tick_set_cb(HAL_GetTick);
//	LVGL display hook
	lv_display_t *display1 = lv_display_create(ILI9341_LCD_PIXEL_WIDTH,
	ILI9341_LCD_PIXEL_HEIGHT);
	lv_display_set_buffers(display1, imgbuf1, NULL, sizeof(imgbuf1),
			LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(display1, my_flush_cb);

//	LVGL touchscreen hook (need to call it after display init)
	lv_indev_t *indev = lv_indev_create();
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_input_read);

//	ui (build from eez studio)
	ui_init();

//	add component callbacks
	if (objects.test_button != NULL) {
		lv_obj_add_event_cb(objects.test_button, test_button_callback,
				LV_EVENT_ALL, NULL);
	}
}

void render_board_state(void) {
	lv_obj_t *parent_panel = objects.chessboard;

	for (int sq64 = 0; sq64 < 64; sq64++) {

		/* Ask VICE what piece is on this square */
		int sq120 = SQ120(sq64);
		int engine_piece = engine_board.pieces[sq120];

		/* Calculate pixel coordinates. (Inverting Y so Rank 1 is at the bottom) */
		int file = sq64 % 8;
		int rank = sq64 / 8;
		int pixel_x = file * SQUARE_SIZE;
		int pixel_y = (7 - rank) * SQUARE_SIZE;

		/* If there is a piece here according to VICE */
		if (engine_piece != EMPTY && engine_piece != OFFBOARD) {

			/* If the LVGL object doesn't exist yet, spawn it */
			if (visual_pieces[sq64] == NULL) {
				visual_pieces[sq64] = lv_img_create(parent_panel);
				if (visual_pieces[sq64] == NULL) {
					printf("FATAL: LVGL Out of Memory!\n");
					return;
				}
				lv_obj_add_flag(visual_pieces[sq64], LV_OBJ_FLAG_CLICKABLE);
				/* Attach the drag callback, and pass its starting square as user_data */
				lv_obj_add_event_cb(visual_pieces[sq64], drag_event_cb,
						LV_EVENT_ALL, (void*) (intptr_t) sq64);
			}

			/* Update the sprite image (handles pawn promotions automatically!) */
			lv_img_set_src(visual_pieces[sq64], get_sprite(engine_piece));

			/* Snap it to the correct pixel grid location */
			lv_obj_set_pos(visual_pieces[sq64], pixel_x, pixel_y);
		}
		/* If VICE says this square is empty */
		else {
			/* If an LVGL piece is still lingering here, delete it from RAM */
			if (visual_pieces[sq64] != NULL) {
				/* CRITICAL: Use the async version to prevent Use-After-Free crashes! */
				lv_obj_del_async(visual_pieces[sq64]);
				visual_pieces[sq64] = NULL;
			}
		}
	}
}

void drag_event_cb(lv_event_t *e) {
	lv_obj_t *obj = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_PRESSED) {
		/* --- FINGER TOUCH DOWN --- */

		/* 1. Zoom the piece to 150% (45x45) */
		lv_img_set_zoom(obj, 384);

		/* 2. Bring the piece to the absolute foreground so it overlaps adjacent pieces */
		lv_obj_move_foreground(obj);

		/* 3. Get the starting square and show the available moves */
		int from_sq64 = (int) (intptr_t) lv_event_get_user_data(e);
		show_move_markers(SQ120(from_sq64));

	} else if (code == LV_EVENT_PRESSING) {
		/* --- FINGER DRAGGING --- */
		lv_indev_t *indev = lv_event_get_indev(e);
		if (indev == NULL)
			return;

		lv_point_t vect;
		lv_indev_get_vect(indev, &vect);
		lv_coord_t x = lv_obj_get_x(obj) + vect.x;
		lv_coord_t y = lv_obj_get_y(obj) + vect.y;
		lv_obj_set_pos(obj, x, y);
	}

	else if (code == LV_EVENT_RELEASED) {
		/* 1. Shrink piece back to normal 100% size */
		lv_img_set_zoom(obj, 256);
		clear_move_markers();

		/* 1. Retrieve the original starting square from the object's user_data */
		int from_sq64 = (int) (intptr_t) lv_event_get_user_data(e);
		int from_sq120 = SQ120(from_sq64);

		/* 2. Calculate where the finger dropped the piece */
		lv_coord_t drop_x = lv_obj_get_x(obj) + (SQUARE_SIZE / 2);
		lv_coord_t drop_y = lv_obj_get_y(obj) + (SQUARE_SIZE / 2);

		/* Convert pixel coordinates to Chess File (0-7) and Rank (0-7) */
		/* Note: LVGL Y=0 is the TOP of the screen (Rank 8), so we invert Y */
		int target_file = drop_x / SQUARE_SIZE;
		int target_rank = 7 - (drop_y / SQUARE_SIZE);

		/* Keep the drop within the 8x8 boundaries */
		if (target_file < 0)
			target_file = 0;
		if (target_file > 7)
			target_file = 7;
		if (target_rank < 0)
			target_rank = 0;
		if (target_rank > 7)
			target_rank = 7;

		/* 3. Convert target to VICE format */
		int to_sq64 = (target_rank * 8) + target_file;
		int to_sq120 = SQ120(to_sq64);

		/* 4. Ask the Engine to process the move */
		if (attempt_human_move(from_sq120, to_sq120)) {
			char from_str[3], to_str[3];
			printf("%s to %s\n", sq64_to_str(from_sq64, from_str),
					sq64_to_str(to_sq64, to_str));

			/* Did the human just checkmate the AI? */
			if (check_game_over(&engine_board)) {
				// Optional: You could disable LVGL touches here so the user can't keep playing
				printf("Game Over!\n");
			} else {
				/* AI's Turn */
				engine_make_move();

				/* Did the AI just checkmate the human? */
				if (check_game_over(&engine_board)) {
					printf("Game Over!\n");
				}
			}
		} else {
			printf("Illigal Move\n");
		}
		render_board_state();
	}
}

void my_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
	uint32_t flush_width = area->x2 - area->x1 + 1;
	uint32_t flush_height = area->y2 - area->y1 + 1;

	uint32_t screen_width = BSP_LCD_GetXSize();
	uint32_t fb_start_address = LtdcHandler.LayerCfg[0].FBStartAdress;
	uint8_t *dest_address = (uint8_t*) fb_start_address
			+ (((area->y1 * screen_width) + area->x1) * BYTES_PER_PIXEL);

	uint8_t *source_address = px_map;

	for (uint32_t y = 0; y < flush_height; y++) {
		memcpy(dest_address, source_address, (flush_width * BYTES_PER_PIXEL));
		source_address += (flush_width * BYTES_PER_PIXEL);
		dest_address += (screen_width * BYTES_PER_PIXEL);
	}

	// CRITICAL: You MUST tell LVGL that you have finished flushing!
	// If you forget this line, LVGL will draw one frame and freeze forever.
	lv_display_flush_ready(display);
}

const void* get_sprite(int vice_piece) {
	switch (vice_piece) {
	case wP:
		return &img_pawn_w;
	case wN:
		return &img_knight_w;
	case wB:
		return &img_bishop_w;
	case wR:
		return &img_rook_w;
	case wQ:
		return &img_queen_w;
	case wK:
		return &img_king_w;

	case bP:
		return &img_pawn_b;
	case bN:
		return &img_knight_b;
	case bB:
		return &img_bishop_b;
	case bR:
		return &img_rook_b;
	case bQ:
		return &img_queen_b;
	case bK:
		return &img_king_b;

	default:
		return NULL;
	}
}

void my_input_read(lv_indev_t *indev, lv_indev_data_t *data) {
	static TS_StateTypeDef TsState;
	BSP_TS_GetState(&TsState);

	if (TsState.TouchDetected) {
		data->state = LV_INDEV_STATE_PRESSED;
		data->point.x = TsState.X;
		data->point.y = TsState.Y;
	} else {
		data->state = LV_INDEV_STATE_RELEASED;
		/* Note: You do not need to update data->point.x or y here.
		 LVGL is smart enough to remember the last known coordinates
		 internally to handle "click and drag" release events. */
	}
}

void update_debug_terminal(QueueHandle_t queue) {
	if (queue == NULL || objects.debug_terminal == NULL)
		return;

	char buf[64];
	char c;
	int i = 0;

	/* Pull up to 63 characters per frame out of the RTOS queue */
	while (i < 63 && xQueueReceive(queue, &c, 0) == pdTRUE) {
		buf[i++] = c;
	}

	if (i > 0) {
		buf[i] = '\0';

		/* 1. Append the new characters */
		lv_textarea_add_text(objects.debug_terminal, buf);

		/* 2. Get the current text and count the lines */
		const char *text = lv_textarea_get_text(objects.debug_terminal);
		int newline_count = 0;
		const char *ptr = text;

		while (*ptr) {
			if (*ptr == '\n') {
				newline_count++;
			}
			ptr++;
		}

		/* 3. If we exceed 10 lines, chop off the oldest ones safely */
		if (newline_count > 10) {
			int lines_to_remove = newline_count - 10;
			ptr = text;

			/* Move pointer forward until we pass the old lines */
			while (lines_to_remove > 0 && *ptr) {
				if (*ptr == '\n') {
					lines_to_remove--;
				}
				ptr++;
			}

			/* * THE FIX: Use a static buffer. It stays in global memory,
			 * so it won't crash your FreeRTOS stack, and it completely
			 * prevents LVGL from reading from memory it is trying to free.
			 */
			static char safe_buffer[500];

			/* Copy the pruned text into our safe holding area */
			strncpy(safe_buffer, ptr, sizeof(safe_buffer) - 1);
			safe_buffer[sizeof(safe_buffer) - 1] = '\0'; // Guarantee null termination

			/* Now update LVGL safely */
			lv_textarea_set_text(objects.debug_terminal, safe_buffer);
			lv_obj_scroll_to_y(objects.debug_terminal, LV_COORD_MAX,
			LV_ANIM_OFF); // scroll to end immediately (prevent cursor bouncing back to top)
		}
	}
}

// returns remaining sleeping time
uint32_t render_timer_handler() {

	uint32_t time_till_next = lv_timer_handler();

	/*If there is nothing to do now, check again a little bit later.*/
	if (time_till_next == LV_NO_TIMER_READY) {
		time_till_next = LV_DEF_REFR_PERIOD; /*33 ms by default in lv_conf.h*/
	}
	return time_till_next;
}

void delete_loading_board_spinner(void) {
	if (objects.loading_board != NULL) {
		lv_obj_del(objects.loading_board); // Deletes it from the screen and memory
		objects.loading_board = NULL; // Best practice: nullify the dead pointer
	}
}

/* Put this somewhere above your initialization code */
void test_button_callback(lv_event_t *e) {
	/* Get the specific event type that triggered this callback */
	lv_event_code_t code = lv_event_get_code(e);

	/* Usually, you want LV_EVENT_CLICKED (fires when you press AND release the button) */
	if (code == LV_EVENT_CLICKED) {
		printf("reset clicked!\n");
	}
}

/* Wipes all red dots from the board */
void clear_move_markers(void) {
	for (int i = 0; i < 64; i++) {
		if (move_markers[i] != NULL) {
			lv_obj_del(move_markers[i]); /* Safe to delete directly, they aren't the drag target */
			move_markers[i] = NULL;
		}
	}
}

/* Asks VICE for legal moves for ONE piece, and draws dots on the targets */
void show_move_markers(int from_sq120) {
	S_MOVELIST list[1];
	GenerateAllMoves(&engine_board, list);

	for (int moveNum = 0; moveNum < list->count; ++moveNum) {
		int move = list->moves[moveNum].move;

		/* If this move belongs to the piece we just touched */
		if (FROMSQ(move) == from_sq120) {

			/* Test if the move is actually legal (doesn't leave King in check) */
			if (MakeMove(&engine_board, move)) {

				int to_sq64 = SQ64(TOSQ(move));
				int file = to_sq64 % 8;
				int rank = to_sq64 / 8;

				/* Calculate pixel coordinates */
				int pixel_x = file * SQUARE_SIZE + SQUARE_SIZE / 2 - 6;
				int pixel_y = (7 - rank) * SQUARE_SIZE + SQUARE_SIZE / 2 - 4;

				/* Spawn the red dot */
				if (move_markers[to_sq64] == NULL) {
					move_markers[to_sq64] = lv_img_create(objects.chessboard);
					lv_img_set_src(move_markers[to_sq64], &img_red_dot);

					/* Center it in the square (Assuming your red dot sprite is 30x30 with a small dot in the middle.
					 If it's smaller, you may need to add an offset here to center it perfectly) */
					lv_obj_set_pos(move_markers[to_sq64], pixel_x, pixel_y);

					/* CRITICAL: Tell LVGL to ignore touches on the red dot,
					 otherwise it blocks you from dropping the piece on that square! */
					lv_obj_clear_flag(move_markers[to_sq64],
							LV_OBJ_FLAG_CLICKABLE);
				}

				/* Undo the test move! */
				TakeMove(&engine_board);
			}
		}
	}
}

