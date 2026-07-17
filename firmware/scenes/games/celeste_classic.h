#ifndef CELESTE_H_
#define CELESTE_H_

#ifdef __cplusplus
#define _Bool bool
extern "C" {
#endif

typedef enum {
	CELESTE_P8_MUSIC, CELESTE_P8_SPR, CELESTE_P8_BTN, CELESTE_P8_SFX,
	CELESTE_P8_PAL, CELESTE_P8_PAL_RESET, CELESTE_P8_CIRCFILL, CELESTE_P8_PRINT,
	CELESTE_P8_RECTFILL, CELESTE_P8_LINE, CELESTE_P8_MGET, CELESTE_P8_CAMERA,
	CELESTE_P8_FGET, CELESTE_P8_MAP
} CELESTE_P8_CALLBACK_TYPE;

typedef _Bool Celeste_P8_bool_t;
typedef int (*Celeste_P8_cb_func_t) (CELESTE_P8_CALLBACK_TYPE calltype, ...);

extern void Celeste_P8_set_call_func(Celeste_P8_cb_func_t func);
extern void Celeste_P8_set_rndseed(unsigned seed);
extern void Celeste_P8_init(void);
extern void Celeste_P8_update(void);
extern void Celeste_P8_draw(void);

extern void Celeste_P8__DEBUG(void); //debug functionality

// Expose player state for AI/attract mode
void get_celeste_player_state(float *x, float *y, float *vx, float *vy, int *djump, _Bool *on_ground, _Bool *wall_left, _Bool *wall_right, int *room_x, int *room_y);
void Celeste_P8_debug_load_room(int x, int y);


//state functionality
size_t Celeste_P8_get_state_size(void);
void Celeste_P8_save_state(void* st);
void Celeste_P8_load_state(const void* st);

#ifdef __cplusplus
} //extern "C"
#endif

#endif
