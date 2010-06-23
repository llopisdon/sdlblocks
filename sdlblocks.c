/*
SDLBlocks
 
Description:
An implementation of tetris using libsdl.
 
Donald E. Llopis 2005 (machinezilla@gmail.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 480

#define TETRIS_WIDTH  10
#define TETRIS_HEIGHT 20

#define TETRAD_WIDTH 20
#define TETRAD_HEIGHT 20

#define MAX_TETRAD 7

#define TETRIS_MIN_X (TETRAD_WIDTH-1)
#define TETRIS_MAX_X (TETRIS_MIN_X+(TETRIS_WIDTH*TETRAD_HEIGHT)+1)
#define TETRIS_MIN_Y 0
#define TETRIS_MAX_Y (TETRIS_MIN_Y+(TETRAD_HEIGHT*TETRIS_HEIGHT))

#define START_X (TETRIS_MIN_X+1)+(TETRAD_WIDTH*4)
#define START_Y TETRIS_MIN_Y

/* tetrad patterns */

struct TetradMask {
	int w, h;
	Uint32 mask_arr[6];
};


/* ### */
/* #   */

struct TetradMask tetrad_0_mask[4] = {
	
	{ 3, 2, { 
			1, 1, 1, 
			1, 0, 0, 
			} },

	{ 2, 3, { 
			1, 1, 
			0, 1, 
			0, 1,
			} },
	
	{ 3, 2, { 
			0, 0, 1, 
			1, 1, 1, 
			} },
	
	{ 2, 3, { 
			1, 0, 
			1, 0, 
			1, 1,
			} }
};

/* ### */
/*   # */

struct TetradMask tetrad_1_mask[4] = {
	
	{ 3, 2, { 
			1, 1, 1, 
			0, 0, 1, 
			} },

	{ 2, 3, { 
			0, 1, 
			0, 1, 
			1, 1,
			} },
	
	{ 3, 2, { 
			1, 0, 0, 
			1, 1, 1, 
			} },
	
	{ 2, 3, { 
			1, 1,
			1, 0,
			1, 0,
			} }
};

/* ### */
/*  #  */

struct TetradMask tetrad_2_mask[4] = {
	
	{ 3, 2, { 
			1, 1, 1, 
			0, 1, 0, 
			} },

	{ 2, 3, { 
			0, 1,
			1, 1,
			0, 1,
			} },
	
	{ 3, 2, { 
			0, 1, 0, 
			1, 1, 1, 
			} },
	
	{ 2, 3, { 
			1, 0,
			1, 1,
			1, 0,
			} }
};

/*  ## */
/* ##  */

struct TetradMask tetrad_3_mask[2] = {
	
	{ 3, 2, { 
			0, 1, 1, 
			1, 1, 0, 
			} },

	{ 2, 3, { 
			1, 0,
			1, 1,
			0, 1,
			} }
};

/* ##  */
/*  ## */

struct TetradMask tetrad_4_mask[2] = {
	
	{ 3, 2, { 
			1, 1, 0, 
			0, 1, 1, 
			} },

	{ 2, 3, { 
			0, 1, 
			1, 1, 
			1, 0,
			} }
};

/* #### */

struct TetradMask tetrad_5_mask[2] = {
	
	{ 4, 1, { 
			1, 1, 1, 1, 0, 0
			} },
	{ 1, 4, { 
			1, 
			1,
			1,
			1,
			0, 0
			} }
};

/* ## */
/* ## */

struct TetradMask tetrad_6_mask[1] = {
	
	{ 2, 2, { 
			1, 1,
			1, 1,
			0, 0
			} }
};

/* tetrad type */

struct Tetrad {
	int num_patterns;
	Uint32 color;
	struct TetradMask * mask; 
};

struct Tetrad tetrad[MAX_TETRAD] = {
	{4, 0xff00ff, &tetrad_0_mask[0]},
	{4, 0xffffff, &tetrad_1_mask[0]},
	{4, 0xffff00, &tetrad_2_mask[0]},
	{2, 0x00ff00, &tetrad_3_mask[0]},
	{2, 0x00ffff, &tetrad_4_mask[0]},
	{2, 0xff0000, &tetrad_5_mask[0]},
	{1, 0x0000ff, &tetrad_6_mask[0]}
};

/* game super type - holds all of the game state variables */

struct Tetris {
	
	/* game state flags */
	int tetrad_drop;
	int tetrad_move;
	int tetrad_skip_move;
	int tetrad_new;
	int tetrad_wait;
	int tetrad_check_fill;
	Uint32 tetrad_drop_rate;

	int cur_tetrad;
	int cur_pattern;
	int prev_pattern;
	int game_over;
	int game_run;
	int game_pause;
	int game_start;
	int game_audio;

	Uint32 game_score;
	Uint32 game_level;
	Uint32 game_total_num_lines_cleared;
	Uint32 game_cur_num_lines_cleared;

	/* tetrad_max_patterns - max patterns of the currently active tetrad */
	int tetrad_max_patterns;

	/* tx, ty -- upper-left position of currently active tetrad relative to the game board */
	int tx;
	int ty;
	int prev_tx;
	int prev_ty;

	/* max_x,max_y -- maximum width and height of currently active tetrad relative to the game board */
	int max_x;
	int max_y;

	/* game clock-tick time variables */
	Uint32 now, next_time;
	
	/* abstract representation of the tetris game board */
	Uint32 board[TETRIS_HEIGHT][TETRIS_WIDTH];
	Uint32 score;

	/* pointer to currently active tetrad */
	struct Tetrad *t;
};

/* function prototypes */

void hline(SDL_Surface *surface, int x, int y, int width, Uint32 pixel );
void vline(SDL_Surface *surface, int x, int y, int height, Uint32 pixel );
void tetris_initialize( struct Tetris * t );
void tetris_draw_board( SDL_Surface *surface, Uint32 *board );
void tetris_update( struct Tetris *t );
Uint32 tetris_score( Uint32 level, Uint32 lines );
void tetris_level_up( struct Tetris *t );
void tetris_draw_text( TTF_Font *font, SDL_Surface *dest, Uint32 x, Uint32 y, char *text );
void tetrad_draw(SDL_Surface *surface, int x, int y, struct Tetrad *t, int pattern );
void tetrad_put( Uint32 *board, struct Tetrad *t, int pattern, int tx, int ty );
int  tetrad_move( Uint32 *board, struct Tetrad *t, int pattern, int tx, int ty );

/*
 * main
 *
 *
 */

int main( int argc, char *argv[] )
{
	const SDL_VideoInfo *video;
	SDL_Surface *screen;
	SDL_Event event;
	SDL_Rect rect;
	TTF_Font *font;
	Mix_Music *music;

	int i, j;
	int x, y;

	struct Tetris tetris;

	char text[256];

	/*
	 * Initialize the game variables
	 *
	 */

	tetris_initialize( &tetris );
	srand( (unsigned int) time( (time_t *)NULL ) );
	
	/*
	 * Initialize Font Engine and load the Bitstream Vera Sans Mono font
	 *
	 */

	if( TTF_Init() == -1 ) {
		fprintf( stderr, "Unable to initialize SDL_ttf: %s\n", TTF_GetError() );
		exit( 1 );
	}

	font = TTF_OpenFont( "Bitstream-Vera-Sans-Mono.ttf", 18 );

	if( font == NULL ) {
		fprintf( stderr, "Unable to load font file: Bitstream-Vera-Sans-Mono.ttf %s\n", TTF_GetError() );
		exit( 1 );
	}

	/*
	 * Initialize SDL Video and Audio
	 *
	 */

	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 ) {
		fprintf( stderr, "Unable to init SDL: %s\n", SDL_GetError() );
		exit( 1 );
	}

	video = SDL_GetVideoInfo();

	if( video == NULL ) {
		fprintf( stderr, "Unable to get video information: %s\n", SDL_GetError() );
		exit( 1 );
	}

	screen = SDL_SetVideoMode ( SCREEN_WIDTH, SCREEN_HEIGHT, video->vfmt->BitsPerPixel, SDL_HWSURFACE | SDL_DOUBLEBUF );

	if( screen == NULL ) {
		fprintf( stderr, "Unable to set up video: %s\n", SDL_GetError() );
		exit( 1 );
	}

	SDL_WM_SetCaption( "SDLBlocks", NULL );
	SDL_WM_SetIcon( SDL_LoadBMP( "sdlblocks.bmp" ), NULL );

	/* setup sound and load game music */

	if( Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024 ) != 0 ) {
		fprintf( stderr,  "Unable to initialize audio: %s\n", Mix_GetError() );
		tetris.game_audio = 0;
	}
	else
		tetris.game_audio = 1;

	if( tetris.game_audio ) {
		music = Mix_LoadMUS( "korobeiniki.mp3" );
		if( music == NULL ) {
			fprintf( stderr, "Unable to load Mp3 file: %s\n", Mix_GetError() );
			exit( 1 );
		}
	}

	/* map tetrad RGB colors to actual colors */
	tetrad[0].color = SDL_MapRGB( screen->format, 0xff, 0x00, 0xff );
	tetrad[1].color = SDL_MapRGB( screen->format, 0xff, 0xff, 0xff );
	tetrad[2].color = SDL_MapRGB( screen->format, 0xff, 0xff, 0x00 );
	tetrad[3].color = SDL_MapRGB( screen->format, 0x00, 0xff, 0x00 );
	tetrad[4].color = SDL_MapRGB( screen->format, 0x00, 0xff, 0xff );
	tetrad[5].color = SDL_MapRGB( screen->format, 0xff, 0x00, 0x00 );
	tetrad[6].color = SDL_MapRGB( screen->format, 0x00, 0x00, 0xff );

	/* setup cleanup callbacks */
	atexit( TTF_Quit );
	atexit( SDL_Quit );
	atexit( Mix_CloseAudio );

	/*
	 * Main Loop
	 *
	 */

    tetris.next_time = SDL_GetTicks();

	while ( tetris.game_run ) {
		/*
		 * Event Handler Section
		 *
		 */

		while ( SDL_PollEvent( &event ) ) {

			switch ( event.type ) {
				case SDL_QUIT:
					tetris.game_run = 0;
					break;

				case SDL_KEYDOWN:
					switch ( event.key.keysym.sym ) {

						case SDLK_LEFT:
							if ( tetris.game_start == 0 ) {
								if( tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
									tetris.prev_tx = tetris.tx;
									
									tetris.tx -= TETRAD_WIDTH;
									
									if( tetris.tx < (TETRIS_MIN_X+1) )
										tetris.tx = (TETRIS_MIN_X+1);

									/* make sure we can move into this position */
									if( !tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
										/* move the tetrad back */
										tetris.tx = tetris.prev_tx;
									}
								}
								tetris.tetrad_wait = 1;
							}
							break;

						case SDLK_RIGHT:
							if ( tetris.game_start == 0 ) {
								if( tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {

									tetris.prev_tx = tetris.tx;

									tetris.tx += TETRAD_WIDTH;

									if( tetris.tx > tetris.max_x )
										tetris.tx = tetris.max_x;

									/* make sure we can move into this position */
									if( !tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
										/* move the tetrad back */
										tetris.tx = tetris.prev_tx;
									}
								}
								tetris.tetrad_wait = 1;
							}
							break;

						case SDLK_UP:
							if ( tetris.game_start == 0 ) {
								tetris.prev_pattern = tetris.cur_pattern;
								tetris.cur_pattern++;

								if( tetris.cur_pattern == tetris.tetrad_max_patterns ) 
									tetris.cur_pattern = 0;

								tetris.max_x = TETRIS_MAX_X - ( tetris.t->mask[tetris.cur_pattern].w * TETRAD_WIDTH );
								tetris.max_y = TETRIS_MAX_Y - ( tetris.t->mask[tetris.cur_pattern].h * TETRAD_HEIGHT );

								/* 
								 * check the position of the new tetrad
								 * make sure that it can be placed along
								 * the right edge of the game board
								 */

								if( tetris.tx > tetris.max_x ) {
									tetris.cur_pattern = tetris.prev_pattern;
									tetris.max_x = TETRIS_MAX_X - ( tetris.t->mask[tetris.cur_pattern].w * TETRAD_WIDTH );
									tetris.max_y = TETRIS_MAX_Y - ( tetris.t->mask[tetris.cur_pattern].h * TETRAD_HEIGHT );
								}

								if( tetris.ty > tetris.max_y ) {
									tetris.cur_pattern = tetris.prev_pattern;
									tetris.max_x = TETRIS_MAX_X - ( tetris.t->mask[tetris.cur_pattern].w * TETRAD_WIDTH );
									tetris.max_y = TETRIS_MAX_Y - ( tetris.t->mask[tetris.cur_pattern].h * TETRAD_HEIGHT );
								}

								if( !tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
									/* move the tetrad back */
									tetris.cur_pattern = tetris.prev_pattern;
									tetris.max_x = TETRIS_MAX_X - ( tetris.t->mask[tetris.cur_pattern].w * TETRAD_WIDTH );
									tetris.max_y = TETRIS_MAX_Y - ( tetris.t->mask[tetris.cur_pattern].h * TETRAD_HEIGHT );
								}
							}
							break;

						case SDLK_DOWN:
							if ( tetris.game_start == 0 ) {
								tetris.prev_ty = tetris.ty;
								tetris.ty += TETRAD_HEIGHT;
								if( tetris.ty > tetris.max_y )
									tetris.ty = tetris.prev_ty;
								/* make sure we can move into this position */
								if( !tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
									/* move the tetrad back */
									tetris.ty = tetris.prev_ty;
								}
							}
							break;

						case SDLK_SPACE:
							if( tetris.game_over ) {
								tetris_initialize( &tetris );
							}
							else if( tetris.game_start ) {
								tetris.game_start = 0;
								tetris.tetrad_skip_move = 1;
								tetris.tetrad_new = 1;
								if( tetris.game_audio ) 
									Mix_PlayMusic( music, -1 );
							}
							else {
								tetris.tetrad_drop = 1;
								tetris.tetrad_move = 0;
							}
#ifdef DEBUG_TETRIS
							fprintf( stderr, "drop...\n");
#endif
							break;

						case SDLK_ESCAPE:
							tetris.game_run = 0;
							break;
					}
					break;

				case SDL_KEYUP:
					switch ( event.key.keysym.sym )
					{
						case SDLK_LEFT:
							break;
						case SDLK_RIGHT:
							break;
						case SDLK_UP:
							break;
						case SDLK_DOWN:
							break;
					}
					break;
			}
		}

		/*
		 * Game Logic Section
		 *
		 */

		/* move the currently active tetrad down one row */

		if ( tetris.tetrad_drop || tetris.tetrad_move ) { 

			tetris.prev_ty = tetris.ty; 

			if ( tetris.tetrad_move ) {
				tetris.now = SDL_GetTicks();

				if ( tetris.tetrad_wait ) {
					tetris.now = 0;
					tetris.tetrad_wait = 0;
				}

				else if ( ( tetris.now - tetris.next_time ) >= tetris.tetrad_drop_rate ) {
					tetris.next_time = SDL_GetTicks();
					tetris.ty += TETRAD_HEIGHT;
				}
			}
			else 
				tetris.ty += TETRAD_HEIGHT; 

			if( tetris.ty > tetris.max_y ) {
				tetris.ty = tetris.prev_ty;
				tetris.tetrad_drop = 0;
				tetris.tetrad_move = 0;
				tetris.tetrad_new = 1;
				tetris.tetrad_check_fill = 1;
			}
			
			/* make sure we can move into this position */
			if( !tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
				if( tetris.ty < 0 ) {
					tetris.game_over = 1;
					tetris.tetrad_new = 0;
					tetris.tetrad_drop = 0;
					tetris.tetrad_move = 0;
					tetris.tetrad_skip_move = 0;

					if( tetris.game_audio )
						Mix_HaltMusic();
				}
				else {
					/* move the tetrad back */
					tetris.ty = tetris.prev_ty;
					tetris.tetrad_drop = 0;
					tetris.tetrad_move = 0;
					tetris.tetrad_new = 1;
					tetris.tetrad_check_fill = 1;
				}
			}
		}

		/* don't move the tetrad for 1 game-loop */

		if ( tetris.tetrad_skip_move ) {
			tetris.tetrad_skip_move = 0;
			tetris.tetrad_move = 1;
			tetris.tetrad_wait = 1;
		}

		/* spawn a new tetrad */

		if( tetris.tetrad_new ) {
		
			/* place the current tetrad on the board */
			tetrad_put( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty );

			/* generate a new tetrad */
			tetris.cur_tetrad = rand() % MAX_TETRAD;
			tetris.cur_tetrad = rand() % MAX_TETRAD;
			tetris.cur_tetrad = rand() % MAX_TETRAD; 
			tetris.cur_tetrad = rand() % MAX_TETRAD;
			tetris.cur_tetrad = rand() % MAX_TETRAD;

			tetris.cur_pattern = 0;

			tetris.t = &tetrad[tetris.cur_tetrad];

			tetris.tx = START_X;
			tetris.ty = 0;

			tetris.tetrad_max_patterns = tetris.t->num_patterns;
			tetris.max_x = TETRIS_MAX_X - ( tetris.t->mask[tetris.cur_pattern].w * TETRAD_WIDTH );
			tetris.max_y = TETRIS_MAX_Y - ( tetris.t->mask[tetris.cur_pattern].h * TETRAD_HEIGHT );
			
			/* check for game over */
			if( !tetrad_move( &(tetris.board[0][0]), tetris.t, tetris.cur_pattern, tetris.tx, tetris.ty ) ) {
#ifdef DEBUG_TETRIS
				fprintf( stderr, "game over...\n" );
#endif
				tetris.game_over = 1;
				tetris.tetrad_new = 0;
				tetris.tetrad_drop = 0;
				tetris.tetrad_move = 0;
				tetris.tetrad_skip_move = 0;

				if( tetris.game_audio )
					Mix_HaltMusic();
			}
			else {
				tetris.tetrad_new = 0;
				tetris.tetrad_drop = 0;
				tetris.tetrad_move = 0;
				tetris.tetrad_skip_move = 1;
			}
		}

		/* check for filled rows and update the score and level */

		if ( tetris.tetrad_check_fill ) { 
#ifdef DEBUG_TETRIS
			fprintf( stderr, "checking board..\n" );
#endif
			tetris_update( &tetris );
			tetris_level_up( &tetris );
			tetris.tetrad_check_fill = 0;

#ifdef DEBUG_TETRIS
	fprintf( stderr, "\n" );
	fprintf( stderr, "cur level: %d\n", tetris.game_level );
	fprintf( stderr, "cur score: %d\n", tetris.game_score );
	fprintf( stderr, "total num lines cleared: %d\n", tetris.game_total_num_lines_cleared );
	fprintf( stderr, "cur num lines cleared: %d\n", tetris.game_cur_num_lines_cleared );
	fprintf( stderr, "\n" );
#endif

		}
		
		/*
		 * Rendering Section
		 *
		 */

		if( SDL_MUSTLOCK( screen ) )
			SDL_LockSurface( screen );

		/* clear the buffer */
		SDL_FillRect( screen, NULL, 0x000000 );

		/* draw the walls: left, right, bottom */
		vline( screen, TETRIS_MIN_X-1, 0, 401, 0xffffff );
		vline( screen, TETRIS_MAX_X+1, 0, 401, 0xffffff );
		hline( screen, TETRIS_MIN_X-1, TETRIS_MAX_Y+1, 204, 0xffffff );

		/* draw the grid */
		for( i=0;i<TETRIS_HEIGHT;i++ ) {
			y = (i * TETRAD_HEIGHT) + (TETRAD_HEIGHT/2);
			for(j=0; j<TETRIS_WIDTH; j++ ) {
				x = (TETRIS_MIN_X-1) + (j*TETRAD_WIDTH) + (TETRAD_WIDTH/2);
				rect.x = x;
				rect.y = y;
				rect.h = 2;
				rect.w = 2;
				SDL_FillRect( screen, &rect, 0x0000ff );
			}
		}

		/* draw the tetrominoes already on the matrix */
		tetris_draw_board( screen, &(tetris.board[0][0]) );

		/* draw the currently active tetrominoe */
		tetrad_draw( screen, tetris.tx, tetris.ty, tetris.t, tetris.cur_pattern );

		/* draw game text */
		sprintf( &text[0], "SDLBlocks" );
		tetris_draw_text( font, screen, 260, 32, &text[0] );
		sprintf( &text[0], "level: %d", tetris.game_level );
		tetris_draw_text( font, screen, 260, 64, &text[0] );
		sprintf( &text[0], "lines: %d", tetris.game_total_num_lines_cleared );
		tetris_draw_text( font, screen, 260, 96, &text[0] );
		sprintf( &text[0], "score: %d", tetris.game_score );
		tetris_draw_text( font, screen, 260, 128, &text[0] );
		
		if( tetris.game_pause ) {
			sprintf( &text[0], "PAUSE" );
			tetris_draw_text( font, screen, 260, 192, &text[0] );
		}
		else if( tetris.game_start ) {
			sprintf( &text[0], "PRESS SPACE..." );
			tetris_draw_text( font, screen, 260, 192, &text[0] );
		}
		else if( tetris.game_over ) {
			sprintf( &text[0], "GAME OVER" );
			tetris_draw_text( font, screen, 260, 192, &text[0] );
		}


		if( SDL_MUSTLOCK( screen ) )
			SDL_UnlockSurface( screen );

		SDL_Flip( screen );
	}

	/* clean up */

	if( tetris.game_audio)
		Mix_FreeMusic( music );

	SDL_FreeSurface( screen );

	return 0;
}


/*
 * tetris_initialize
 *
 * initialize tetris game variables
 *
 * input:
 *
 * struct Tetris * tetris - point to type struct Tetris
 *
 * output: none
 *
 */

void tetris_initialize( struct Tetris *tetris )
{
	if( tetris != NULL ) {

		Uint32 *board = &(tetris->board[0][0]);
		memset( board, 0, (TETRIS_HEIGHT*TETRIS_WIDTH)*sizeof(Uint32) );
		
		tetris->tetrad_drop = 0;
		tetris->tetrad_new = 0;
		tetris->tetrad_wait = 0;
		tetris->tetrad_check_fill = 0;
		tetris->cur_tetrad = 0;
		/* set cur_pattern to -1 so that the tetrad isn't drawn while
		 * the game hasn't started. */
		tetris->cur_pattern = -1;
		tetris->prev_pattern = 0;
		tetris->tetrad_max_patterns = 0;
		tetris->max_x = 0;
		tetris->max_y = 0;
		tetris->prev_tx = 0;
		tetris->prev_ty = 0;
		tetris->score = 0;
		tetris->game_over = 0;
		tetris->game_pause = 0;
		tetris->game_score = 0;
		tetris->game_level = 0;
		tetris->game_total_num_lines_cleared = 0;
		tetris->game_cur_num_lines_cleared = 0;
		tetris->tetrad_move = 0;
		tetris->tetrad_skip_move = 0;
		tetris->game_run = 1;
		tetris->tx = START_X;
		tetris->ty = START_Y;

		tetris->game_start = 1;
		tetris->game_pause = 0;
		tetris->game_over = 0;

		/* set the first tetrad */
		tetris->tetrad_drop_rate = 500;

		tetris->t = &tetrad[tetris->cur_tetrad];
		tetris->tetrad_max_patterns = tetris->t->num_patterns;
		tetris->max_x = TETRIS_MAX_X - ( tetris->t->mask[tetris->cur_pattern].w * TETRAD_WIDTH );
		tetris->max_y = TETRIS_MAX_Y - ( tetris->t->mask[tetris->cur_pattern].h * TETRAD_HEIGHT );
	}
}

/*
 * hline
 *
 * draw a horizontal line
 *
 */
void hline(SDL_Surface *surface, int x, int y, int width, Uint32 pixel )
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.h = 1;
	rect.w = width;

	SDL_FillRect( surface, &rect, pixel );
}

/*
 * vline
 *
 * draw a vertical line
 *
 */
void vline(SDL_Surface *surface, int x, int y, int height, Uint32 pixel )
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.h = height;
	rect.w = 1;
	
	SDL_FillRect( surface, &rect, pixel );
}


/*
 * tetris_update
 *
 * check the tetris game board for filled rows and remove them if they are found
 * and accumulate points.
 *
 */
void tetris_update( struct Tetris *t )
{
	Uint32 board_tmp[TETRIS_HEIGHT][TETRIS_WIDTH];
	Uint32 *bptr;
	Uint32 num_lines_cleared;
	int i, j;
	int n;
	int m;

	memset( &board_tmp[0][0], 0, (TETRIS_HEIGHT*TETRIS_WIDTH)*sizeof(Uint32) );

	num_lines_cleared = 0;

	/* check the board for filled rows from the bottom-up */
	i = TETRIS_HEIGHT - 1;

	for( i=TETRIS_HEIGHT-1; i>-1; i-- ) {
		bptr = (i * TETRIS_WIDTH) + &(t->board[0][0]);
		n = 0;
		for ( j=0; j<TETRIS_WIDTH; j++ ) {
				if ( *bptr++ )
						n++;
		}

#ifdef DEBUG_TETRIS
		fprintf( stderr, "n: %d\n", n );
#endif

		/* is there a fully filled row? */
		if( n == TETRIS_WIDTH ) { 

			/* yes, then shift the board down to the current level */

			num_lines_cleared++;

			bptr = &(t->board[0][0]);

			m = i * TETRIS_WIDTH;
			memcpy( &board_tmp[0][0], bptr, m*sizeof(Uint32) );
			memcpy( bptr+TETRIS_WIDTH, &board_tmp[0][0], m*sizeof(Uint32) );

			/* push the row counter back up to the previous line */
			i++;
		}
	}

	/* update game score */
	t->game_score += tetris_score( t->game_level, num_lines_cleared );
	t->game_total_num_lines_cleared += num_lines_cleared;
	t->game_cur_num_lines_cleared += num_lines_cleared;

	/* check for tilt, if so then reset the score */
	if( t->game_score > 9999999 ) {
		t->game_score = 0;
	}
}

/*
 * tetris_score
 *
 * calculate game score based on NES scoring algorithm
 * see wikipedia.org entry for tetris for an explaination.
 *
 */
Uint32 tetris_score( Uint32 level, Uint32 lines )
{
	if( lines == 0 )
		return 0;
	else if( lines == 1 ) {
		return ( level + 1 ) * 40;
	}
	else if( lines == 2 ) {
		return ( level + 1 ) * 100;
	}
	else if( lines == 3 ) {
		return ( level + 1 ) * 300;
	}
	else {
		return ( level + 1 ) * 1200;
	}
}

/*
 * tetris_level_up
 *
 * increase the game level every 10 lines cleared
 * stop at level 20
 *
 * increase tetrad_drop_rate by 20ms per level
 *
 */
void tetris_level_up( struct Tetris *t )
{
	if( ( t->game_cur_num_lines_cleared > 9 ) && ( t->game_level < 20 ) ) {
		t->game_level++;
		t->tetrad_drop_rate -= 20;
		t->game_cur_num_lines_cleared -= 10;
	}
	else if( ( t->game_cur_num_lines_cleared >= 10 ) && ( t->game_level >= 20 ) ) {
		t->game_level++;
		t->game_cur_num_lines_cleared -= 10;
	}
}

/*
 * tetris_draw_text
 *
 * draw a text string to some surface at some (x,y)
 *
 */
void tetris_draw_text( TTF_Font *font, SDL_Surface *dest, Uint32 x, Uint32 y, char *text )
{
	SDL_Surface *src;
	SDL_Rect rect;
	SDL_Color white = { 0xff, 0xff, 0xff, 0x00 };

	src = TTF_RenderText_Solid( font, text, white );

	rect.x = x;
	rect.y = y;
	rect.w = src->w;
	rect.h = src->h;

	if( src != NULL ) {
		SDL_BlitSurface( src, NULL, dest, &rect );
		SDL_FreeSurface( src );
	}
}

/*
 * tetrad_draw
 *
 * draw a tetrad on the board
 *
 */
void tetrad_draw( SDL_Surface *surface, int x, int y, struct Tetrad *t, int pattern )
{
	Uint32 *mask;
	SDL_Rect rect;
	int i, j;
	int w, h;

	if ( pattern < 0 )
		return;
	
	rect.x = x;
	rect.y = y;
	rect.h = TETRAD_HEIGHT - 1;
	rect.w = TETRAD_WIDTH - 1;

	mask = t->mask[pattern].mask_arr;
	w = t->mask[pattern].w;
	h = t->mask[pattern].h;

	for( i=0; i<h; i++ ) {

		rect.y = y + (i*TETRAD_HEIGHT) + 1;

		if ( y > -1 ) {
			for( j=0; j<w; j++ ) {
				rect.x = x + (j*TETRAD_WIDTH) + 1;
				if ( *mask++ )
					SDL_FillRect( surface, &rect, t->color );
			}
		}
	}
}

/*
 * tetrad_move
 *
 */
int tetrad_move( Uint32 *board, struct Tetrad *t, int pattern, int tx, int ty )
{
	Uint32 *mask;
	Uint32 *bptr;
	int i, j;
	int bx;
	int by;
	int w, h;

	bx = (tx / TETRAD_WIDTH)-1;
	by = ty / TETRAD_HEIGHT;
	w = t->mask[pattern].w;
	h = t->mask[pattern].h;

	mask = t->mask[pattern].mask_arr;

	for( i=0; i<h; i++ ) {
		if ( by > -1 ) {
			bptr = board + ( by * TETRIS_WIDTH ) + bx;
			for( j=0; j<w; j++ ) {
				if( *mask && *bptr ) {
					return 0;
				}
				mask++;
				bptr++;
			}
		}
		by++;
	}
	
	return 1;
}

/*
 * tetrad_put
 *
 */
void tetrad_put( Uint32 *board, struct Tetrad *t, int pattern, int tx, int ty )
{
	Uint32 *mask;
	Uint32 *bptr = board;
	int i, j;
	int bx;
	int by;
	int w, h;

	if ( ty < 0 )
		return;

	bx = (tx / TETRAD_WIDTH)-1;
	by = ty / TETRAD_HEIGHT;
	w = t->mask[pattern].w;
	h = t->mask[pattern].h;

	mask = t->mask[pattern].mask_arr;

	for( i=0; i<h; i++ ) {
		if ( by > -1 ) {
			bptr = board + ( by * TETRIS_WIDTH ) + bx;
			for( j=0; j<w; j++ ) {
				if( *mask ) {
					*bptr = t->color;
				}
				mask++;
				bptr++;
			}
		}
		by++;
	}

#ifdef DEBUG_TETRIS

	bptr = board;

	fprintf( stderr, "\n\n");

	for( i=0; i<TETRIS_HEIGHT; i++ ) {

		for( j=0; j<TETRIS_WIDTH; j++ ) {

			if ( *bptr++ ) {
				fprintf( stderr, "1 " );
			}
			else
				fprintf( stderr, "0 " );
		}
		fprintf( stderr, "\n");
	}

#endif 
}

/*
 * tetris_draw_board
 *
 */
void tetris_draw_board( SDL_Surface *surface, Uint32 *board ) 
{
	Uint32 *bptr = board;
	SDL_Rect rect;
	int i, j;

	rect.h = TETRAD_HEIGHT - 1;
	rect.w = TETRAD_WIDTH - 1;
	
	for( i=0; i<TETRIS_HEIGHT; i++ ) {
		rect.y = (i * TETRAD_HEIGHT) + TETRIS_MIN_Y + 1;
		rect.x = 1;
		for( j=0; j<TETRIS_WIDTH; j++ ) {
			rect.x += TETRAD_WIDTH;
			if ( *bptr ) {
				SDL_FillRect( surface, &rect, *bptr );
			}
			bptr++;
		}
	}
}

/* vim: set ci ai ts=4 sw=4: */
