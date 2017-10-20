#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cab202_graphics.h"
#include "cab202_timers.h"
#include "cab202_sprites.h"

//--------------------------------------------------------------------------
// Declare constants

#define MAX_BOMBS (4)

//--------------------------------------------------------------------------
// Declare functions.
//--------------------------------------------------------------------------

typedef struct
{
	sprite_id bombs[5];
	sprite_id aliens[13];
	sprite_id player;
	sprite_id bullet;
	timer_id bomb_clock;
	timer_id alien_clock;
	timer_id bomb_move_clock;
	timer_id bullet_clock;
	int num_bombs;
	int last_bomb;
	int key;
	int score;
	int lives;
	int level;
	int aliens_dead;
	int jerk_alien;
	int jerk_target_x;
	int jerk_target_y;
	double timescale;
	double fake_timescale;
	double scale_factor;
	timer_id jerk_timer;
	bool show_info;
	bool over;
	bool jerk_moving;
	bool jerk_halfway;	
}Game;

void draw_everything(Game * game);
bool check_adjacent(Game * game, int alien, int vector);
void move_jerk(Game * game);

void setup_aliens(Game * game)
{
	game->jerk_alien = 10;
	game->jerk_halfway = false;
	for (int i = 0; i < 7; i++)
	{
		int alien_setup_x = (40 + (3*i*2));
		int alien_setup_y =((6)- (i % 2) * 2);
		game->aliens[i] = create_sprite(alien_setup_x, alien_setup_y, 1, 1, "@");
	}
	for (int i = 0; i < 3; i++)
	{
		int alien_setup_x = (46 + (6 * i * 2));
		int alien_setup_y = ((6) + 2);
		game->aliens[i+7] = create_sprite(alien_setup_x, alien_setup_y, 1, 1, "@");
	}
	game->aliens[10] = create_sprite(0, 0, 1, 1,  "X");
	game->aliens[11] = create_sprite(0,0,1,1, "O");
	game->aliens[10]->is_visible = false;
	game->aliens[11]->is_visible = false;
	for (int i = 0; i < 11; i++)
	{
		game->aliens[i]->dx = 1;
		if(game->level == 2){
			game->aliens[i]->dy = 1;
		}
	}
}

void setup_timers(Game * game)
{
	game->bomb_clock = create_timer(3000);
	game->alien_clock = create_timer(400); // 600
	game->bomb_move_clock = create_timer(200);
	game->bullet_clock = create_timer(50);
	game->jerk_timer = create_timer(3000 + ((rand() % 1500) -(rand() % 1500)));
}

void setup_bombs(Game * game)
{
	for (int i = 0; i < MAX_BOMBS; i++){
		game->bombs[i] = create_sprite(0,0, 1,1, "*");
		game->bombs[i]->dy = 1;
		game->bombs[i]->is_visible = false;
	}
}

void setup_bullet(Game * game){
	game->bullet = create_sprite(0, 0, 1,1, "|");
	game->bullet->dy = -1;
	game->bullet->is_visible = false;
}

void setup_player(Game * game){
	game->player = create_sprite(screen_width() / 2, screen_height() - 4, 1, 1, "^");
}

void setup_panel(Game * game){
	//game->score = 0;
}

void setup_megamaniac(Game * game)
{
	setup_aliens(game);
	setup_timers(game);
	setup_bombs(game);
	setup_player(game);
	setup_bullet(game);
	setup_panel(game);
	game->num_bombs = 0;
	game->last_bomb = 0;
	game->aliens_dead = 0;
	game->timescale = 1;
	game->fake_timescale = 1;
	game->over = false;
	game->jerk_moving = false;
	game->show_info = true;
}

void draw_player(Game * game)
{
	draw_sprite(game->player);
}

void draw_aliens(Game * game)
{
	for (int i = 0; i < 11; i++)
	{
		draw_sprite(game->aliens[i]);
	}
}

void draw_bombs(Game * game)
{
	for (int i = 0; i < MAX_BOMBS; i++)
		draw_sprite(game->bombs[i]);
}

void draw_bullet(Game * game){
	draw_sprite(game->bullet);
}

void draw_panel(Game * game){
	for (int i = 0; i < screen_width(); i++){
		draw_char(i, screen_height() - 3, '-');
	}
	draw_string(screen_width() - 21, screen_height()-2, "Score: ");
	draw_int(screen_width() - 14, screen_height()-2, game->score);
	draw_string(screen_width() - 9, screen_height()-2, "Lives: ");
	draw_int(screen_width() - 2 , screen_height() - 2, game->lives);
	draw_string(screen_width()/2 - 8, screen_height() - 2, "Timescale:");
	draw_double(screen_width()/2 + 3, screen_height() - 2, game->fake_timescale);
	draw_string(screen_width()/2 - 6, screen_height()-1, "Level");
	draw_int (screen_width()/2, screen_height() - 1, game->level);
	draw_string (1, screen_height()-2, "James Flannery: n8326631");
	if (game->show_info){
		draw_string(screen_width()/2 - 37, screen_height()/2, "For marking purposes, press '+' or '-' to speed up or slow down the aliens");
	}
	//draw_int (20, screen_height()-1, game->jerk_target_y);
}

void draw_game_over(Game * game){
	if (game->lives <= 0){
		draw_string(screen_width()/2 - 17, screen_height()/2, "GAME OVER - THE EARTH IS DESTROYED");
		draw_string(screen_width()/2 - 15, screen_height()/2 + 1, "Press R to restart or Q to quit");
	}
}

bool update_bombs(Game * game)
{
	int r = rand() % 10;
	if (game->num_bombs < MAX_BOMBS && timer_expired(game->bomb_clock))
	{
		while (!game->aliens[r]->is_visible){
			r = rand() % 10;
		}
		game->bombs[game->last_bomb]->x = game->aliens[r]->x;
		game->bombs[game->last_bomb]->y = game->aliens[r]->y;
		game->bombs[game->last_bomb]->is_visible = true;
		game->num_bombs++;
		game->last_bomb = (game->last_bomb+1)%MAX_BOMBS;
	}

	if (timer_expired(game->bomb_move_clock))
	{
		for (int i = 0; i < MAX_BOMBS; i++)
		{
			game->bombs[i]->y += game->bombs[i]->dy;
			if(game->bombs[i]->y > screen_height() - 4 && game->bombs[i]->is_visible == true)
			{
				game->bombs[i]->is_visible = false;
				game->last_bomb = i;
				game->num_bombs--;
			}
			if(game->bombs[i]->x == game->player->x && game->bombs[i]->y == game->player->y){
				game->lives--;
				game->bombs[i]->is_visible = false;
				game->last_bomb = i;
				game->num_bombs--;
			}
		}
		return true;
	}
	return false;
}

void level_two_move(Game * game, int alien){
		if (game->aliens[alien]->dy ==1){
			game->aliens[alien]->dy -= 2;
		}
		else if (game->aliens[alien]->dy == -1){
			game->aliens[alien]->dy += 2;
	}
}

void level_three_move(Game * game, int alien){
if (alien != game->jerk_alien){
		game->aliens[alien]->dy = 1;
		if(game->aliens[alien]->y == game->player->y && game->aliens[alien]->x == game->player->x){
			game->aliens[alien]->is_visible = false;
			game->aliens_dead++;
			game->lives--;
			
		}
		if (game->aliens[alien]->y > screen_height() - 5){
			game->aliens[alien]->y = -1;
		}
	}
}

void level_four_move(Game * game, int alien){
	level_three_move(game, alien);
	int rand_vector = (rand() % 3) - 1;
	if(check_adjacent(game, alien, rand_vector)){
		game->aliens[alien]->x += rand_vector;
	}
}

bool check_adjacent(Game * game, int alien, int vector){
	bool can_move = true;
	for (int i = 0; i < 10; i++){
		if (alien != i){
			if(game->aliens[alien]->x+vector == game->aliens[i]->x|| game->aliens[alien]->x+vector == game->aliens[i]->x+1|| game->aliens[alien]->x+vector == game->aliens[i]->x -1){
				can_move = false;
			}
		}
	}
	return can_move;
}

void level_five_move(Game * game, int alien){
	game->aliens[alien]->dx = 1;
	level_three_move(game, alien);
	if (alien != game->jerk_alien)
		game->aliens[alien]->dy = 0;
	if (timer_expired(game->jerk_timer)){	
		if (!game->jerk_moving){
			game->jerk_timer = create_timer(4000 + ((rand() % 500) -(rand() % 500)));
			while (!game->aliens[game->jerk_alien]->is_visible){
					game->jerk_alien = rand() % 10;
			}			
			game->aliens[game->jerk_alien]->dy = 1;
			game->aliens[10]->x = game->aliens[game->jerk_alien]->x;
			game->aliens[10]->y = game->aliens[game->jerk_alien]->y;
			game->aliens[10]->is_visible = true;
			game->jerk_moving = true;
			game->jerk_target_x = game->player->x;
			//game->jerk_target_y = (((screen_height() - 4) - game->aliens[game->jerk_alien]->y) - game->player->x);// - (game->aliens[game->jerk_alien]->x - game->player->x));
			game->jerk_target_y =(screen_height()/2);
		}
	}
	if(game->jerk_moving){
		move_jerk(game);
		if (game->aliens[game->jerk_alien]->y > screen_height() - 4){
			game->aliens[game->jerk_alien]->x = game->aliens[10]->x;
			game->aliens[game->jerk_alien]->y = game->aliens[10]->y;
			game->jerk_moving = false;
			game->jerk_halfway = false;
			game->jerk_alien = 11;
		}
	}

}

void move_jerk(Game * game){
	if (abs((game->player->x - game->aliens[game->jerk_alien]->x))+1 > abs((game->player->y - game->aliens[game->jerk_alien]->y))){
		game->jerk_halfway = true;
	}
	
	if(!game->jerk_halfway){
		if (game->aliens[game->jerk_alien]->x > game->player->x + 1){
			game->aliens[game->jerk_alien]->dx = 1;
		}else{
			game->aliens[game->jerk_alien]->dx = -1;
		}
	}
	if (game->jerk_halfway){
		if (game->aliens[game->jerk_alien]->x > game->player->x){
					game->aliens[game->jerk_alien]->dx = -1;
		}else{
			game->aliens[game->jerk_alien]->dx = 1;
		}
	}
}

bool update_aliens(Game * game)
{
	if (game->key == '+' && game->timescale > 0.2){
		game->timescale -= 0.1;
		game->fake_timescale += 0.1;
		game->alien_clock = create_timer (400 * game->timescale);
		game->show_info = false;
	}
	if (game->key == '-' && game->timescale < 1.5){
		game->timescale += 0.1;
		game->fake_timescale -= 0.1;
		game->alien_clock = create_timer (400 * game->timescale);
		game->show_info = false;
	}
	if(timer_expired(game->alien_clock))
	{
		for (int i = 0; i < 12; i++)
		{		
				if (game->level == 2){
					level_two_move(game, i);
				}
				
				if (game->level == 3){
					level_three_move(game, i);
				}	
				
				if (game->level == 4){
					level_four_move(game, i);
				}
				if (game->level == 5){
					level_five_move(game, i);
				}
				if (game->aliens[i]->x <= screen_width()-1) //&& !game->aliens[i]->x <= 0)
				{
					game->aliens[i]->x = game->aliens[i]->x + (game->aliens[i]->dx);
					game->aliens[i]->y = game->aliens[i]->y + (game->aliens[i]->dy);
				}else if (game->aliens[i]->x > screen_width()-1){
					game->aliens[i]->x = 0;
				}
				if (game->aliens[i]->x < 0){
					game->aliens[i]->x = screen_width()-1;
				}
		}
		if (game->aliens_dead == 10){
			game->aliens_dead = 0;
			game->level = ((game->level + 1) % 6);
			draw_everything(game);
			setup_megamaniac(game);
			game->score += 500;
		}
		return true;
	}
	return false;
}



bool update_player(Game * game)
{
		if (game->key == 'a'){
			game->show_info = false;
			if (game->player->x > 0){
				game->player->x--;
			}else{
				game->player->x = screen_width()-1;
			}

			return true;
		}
		if (game->key == 'd'){
			game->show_info = false;
			if (game->player->x < screen_width() - 1){
				game->player->x++;
				return true;
			}else{
				game->player->x = 0;
			}
		}
	return false;
}


bool update_bullet(Game * game)
{
	if (game->key == 's' && !game->bullet->is_visible)
	{
		game->bullet->x = game->player->x;
		game->bullet->y = game->player->y;
		game->bullet->is_visible = true;
		game->show_info = false;
		return true;
	}
	if (game->key == 'z' && !game->bullet->is_visible && game->level == 5){
		game->bullet->x = game->player->x;
		game->bullet->y = game->player->y;
		game->bullet->is_visible = true;
		game->show_info = false;
		return true;
	}
	if (game->key == 'c' && !game->bullet->is_visible && game->level == 5){
		game->bullet->x = game->player->x;
		game->bullet->y = game->player->y;
		game->bullet->is_visible = true;
		game->show_info = false;
		return true;
	}
	if (game->key == 'z' && game->bullet->is_visible && game->level == 5){
		game->show_info = false;
		game->bullet->dx -= 0.1;
	}
	if (game->key == 'c' && game->bullet->is_visible && game->level == 5){
		game->show_info = false;
		game->bullet->dx += 0.1;
	}
	if(game->bullet->is_visible && timer_expired(game->bullet_clock))
	{
		game->bullet->x += game->bullet->dx;
		game->bullet->y += game->bullet->dy;
		if (game->bullet->y < 0 || game->bullet->x < 0 || game->bullet->x > screen_width()){
			game->bullet->is_visible = false;
			game->bullet->x = -1;
			game->bullet->y = 0;
			game->bullet->dx = 0;
		}
		for (int i = 0; i < 10; i++)
		{
			if (game->aliens[i]->x == game->bullet->x && game->aliens[i]->y == game->bullet->y && game->aliens[i]->is_visible)
			{
				game->bullet->x = -1;
				game->bullet->y = 0;
				game->bullet->is_visible = false;
				game->aliens[i]->is_visible = false;
				game->aliens_dead++;
				game->score += 30;
			}
		}
		return true;
	}
	return false;
}

void draw_everything(Game * game){
	clear_screen();
	draw_aliens(game);
	draw_bombs(game);
	draw_player(game);
	draw_bullet(game);
	draw_panel(game);
	draw_game_over(game);
	show_screen();
}
void megamaniac()
{
	Game game;
	setup_megamaniac(&game);
	game.score = 0;
	game.lives = 3;
	game.level = 1;
	while (!game.over)
	{	
		// while (game.lives <= 0){
			// draw_game_over(&game);
		// }

		game.key = get_char();
		if (update_aliens(&game))
		{
			draw_everything(&game);
		}	
		if(update_bullet(&game))
		{
			draw_everything(&game);
		}
		if(update_bombs(&game))
		{
			draw_everything(&game);
		}

		if(update_player(&game))
		{
			draw_everything(&game);
		}
		if (game.key == 'q'){
			break;
		}
		if(game.key == 'r'){
			game.score = 0;
			game.lives = 3;
			game.level = 1;	
			game.timescale = 1;
			setup_megamaniac(&game);
		}
		if (game.key == 'l'){
			game.level = ((game.level + 1) % 6);
			if (game.level == 0)
				game.level = 1;
			draw_everything(&game);
			setup_megamaniac(&game);
		}
	}
}

int main( void ) 
{
	srand( time( NULL ) );
	setup_screen();

	megamaniac();

	cleanup_screen();
	return 0;
}
