#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_random.h>

#include "engine.hpp"

#include "graphics.h"

#define TASKNAME "mijakspad"
#define BLOCKSIZE 16

typedef enum {
	I = 0,
	J,
	L,
	O,
	S,
	T,
	Z,
	NONE
} piece;

uint16_t bag[7],
         bag_size = 7,
         usedhold = 0;

int score = 0;
piece next = NONE;

static int board[15][10];

class Tetromino {
	public:
		Tetromino(int x, int y, piece type, airboy::Display *display);
		void draw();
		bool fall();
		void hardfall();
		void setx(int x);
		void sety(int y);
		void setrotation(int rotation);
		int gety();
		piece gettype();
		void rotate(bool left);
		void move(bool left);
	private:
		int x, y, //TODO: make the piece fall from y = -32
			bx, by, bw, bh, // bounding box
			rotation,
			sprite,
			block_offsets[4][2];
		piece type;
		airboy::Display *display = nullptr;

		void moveblocks();
		bool checkcollision();
		void setblocks();
};

class Bag
{
	public:
		Bag();
		piece generatepiece();
	private:
		bool bag[7];
		int bag_size = 0;
		void fillbag();
};

class Mijakspad : public airboy::Engine
{
	public:
		Tetromino *current = nullptr,
				  *next = nullptr,
				  *hold = nullptr;

		Bag *bag = nullptr;
		
		bool usedhold = false;
		
		int score = 0,
			frame = 0,
			lefttimeout = 0,
			leftcounter = 0,
			righttimeout = 0,
			rightcounter = 0,
			downtimeout = 0,
			downcounter = 0,
			speedup = 1;

		enum {
			PLAY,
			PAUSE,
			DEAD,
			MENU,
			HELP
		} state = MENU;

		enum {
			EASY = 100,
			MEDIUM = 80,
			HARD = 50,
		} difficulty = EASY;

		void setup() override
		{
			for (int i = 0; i < 15; i++)
				for (int j = 0; j < 10; j++)
					board[i][j] = -1;
		}

		void update(float delta) override
		{
			this->display->clear_buffer();
			drawui();
			handleinput();

			if (state == PLAY) {
				current->draw();
				if (hold != nullptr)
					hold->draw();
				
				if (frame % (difficulty / speedup) == 0) {
					if(current->fall()) {
						if (current->gety() == 0) {
							state = DEAD;
						}

						checklines();

						free(current);

						current = next;

						next = new Tetromino(10 * BLOCKSIZE + 50, 25, bag->generatepiece(), this->display);
						usedhold = false;

						current->setx((current->gettype() == O ? 4 : 3) * BLOCKSIZE);
						current->sety(0);

						if (state == DEAD) {
							free(next);
							next = nullptr;

							if (hold != nullptr) {
								free(hold);
								hold = nullptr;
							}
						}
					}
				}

				drawboard();
			}


			frame++;
			if (frame >= 360000)
				frame = 0;
		}

	private:
		void drawui();
		void changedifficulty(bool up);
		void drawboard();
		void newgame();
		void checklines();
		void handleinput();
		void drawsprite(uint16_t x, uint16_t y, int index);
};

const uint16_t block_lookup[7][4][4][2] = {
	{ // I
		{{0,1},{1,1},{2,1},{3,1}},
		{{2,0},{2,1},{2,2},{2,3}},
		{{0,2},{1,2},{2,2},{3,2}},
		{{1,0},{1,1},{1,2},{1,3}},
	},
	{ // J
		{{0,0},{0,1},{1,1},{2,1}},
		{{1,0},{2,0},{1,1},{1,2}},
		{{0,1},{1,1},{2,1},{2,2}},
		{{1,0},{1,1},{1,2},{0,2}},
	},
	{ // L
		{{2,0},{2,1},{1,1},{0,1}},
		{{1,0},{1,1},{1,2},{2,2}},
		{{0,1},{1,1},{2,1},{0,2}},
		{{0,0},{1,0},{1,1},{1,2}},
	},
	{ // O
		{{0,0},{1,0},{0,1},{1,1}},
		{{0,0},{1,0},{0,1},{1,1}},
		{{0,0},{1,0},{0,1},{1,1}},
		{{0,0},{1,0},{0,1},{1,1}},
	},
	{ // S
		{{1,0},{2,0},{0,1},{1,1}},
		{{1,0},{1,1},{2,1},{2,2}},
		{{1,1},{2,1},{0,2},{1,2}},
		{{0,0},{0,1},{1,1},{1,2}},
	},
	{ // T
		{{1,0},{0,1},{1,1},{2,1}},
		{{1,0},{1,1},{2,1},{1,2}},
		{{0,1},{1,1},{2,1},{1,2}},
		{{1,0},{0,1},{1,1},{1,2}},
	},
	{ // Z
		{{0,0},{1,0},{1,1},{2,1}},
		{{2,0},{1,1},{2,1},{1,2}},
		{{0,1},{1,1},{1,2},{2,2}},
		{{1,0},{0,1},{1,1},{0,2}},
	},
};


void Mijakspad::drawsprite(uint16_t x, uint16_t y, int index)
{
	int width = x + 16 < 320 ? 16 : 320 - x;
	int height = y + 16 < 240 ? 16 : 240 - y;

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			this->display->set_pixel_fast(x + j, y + i, blocks[index][i * 16 + j]);
		}
}


void Mijakspad::drawui()
{
	//Rectangle ui = {10*BLOCKSIZE, 0, LCD_WIDTH - 10*BLOCKSIZE, LCD_HEIGHT, 0x4d6b};
	//drawRect(ui, framebuffer);
	//airboy::Vector2i pos = airboy::Vector2i(10 * 16, 0);
	//airboy::Vector2i size = airboy::Vector2i(160, 240);
    this->renderer->draw_fill_rect(airboy::Vector2i(10 * 16, 0), airboy::Vector2i(160, 240), 0x4d6b);

	//Rectangle scorerect = {10*BLOCKSIZE + 30, LCD_HEIGHT - 60, 100, 40, 0x0};
	//drawRect(scorerect, framebuffer);
	//pos.x = 190;
	//pos.y = 180;
	//size.x = 100;
	//size.y = 40;
    this->renderer->draw_fill_rect(airboy::Vector2i(190, 180), airboy::Vector2i(100, 40), 0x0);

	char scorestr[20] = "";
	sprintf(scorestr, "%06d", score);
	//drawText(10*BLOCKSIZE + 47, LCD_HEIGHT - 58, 2, 0xFBDE, scorestr, framebuffer);
	//drawText(10*BLOCKSIZE + 52, LCD_HEIGHT - 38, 2, 0xFBDE, "Score", framebuffer);
	//pos.x = 207;
	//pos.y = 182;
	this->renderer->draw_text(airboy::Vector2i(207, 182), 2, 0xFBDE, scorestr);
	//pos.x = 212;
	//pos.y = 202;
	this->renderer->draw_text(airboy::Vector2i(212, 202), 2, 0xFBDE, "Score");

	//Rectangle nextrect = {10*BLOCKSIZE + 30, 20, 100, 60, 0x0};
	//drawRect(nextrect, framebuffer);
	//pos.x = 190;
	//pos.y = 20;
	//size.x = 100;
	//size.y = 60;
    //this->renderer->draw_fill_rect(pos, size, 0x0);
    this->renderer->draw_fill_rect(airboy::Vector2i(190, 20), airboy::Vector2i(100, 60), 0x0);

	//if (next != -1) 
		//for (int i = 0; i < 4; i++) {
			//Sprite nextsprite =	{10*BLOCKSIZE + 50 + block_lookup[next][0][i][0]*BLOCKSIZE, 25 + block_lookup[next][0][i][1]*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE, blocks[next], 0,0,0,0,0,0,};
			//drawSprite(nextsprite, framebuffer);
		//}
	//if (state == PLAY) {
	if (next != nullptr) {
		next->draw();
	}
	//drawText(10*BLOCKSIZE + 58, 62, 2, 0xFBDE, "Next", framebuffer);
	//pos.x = 218;
	//pos.y = 62;
	//this->renderer->draw_text(pos, 2, 0xFBDE, "Next");
	this->renderer->draw_text(airboy::Vector2i(218, 62), 2, 0xFBDE, "Next");

	//Rectangle holdrect = {10*BLOCKSIZE + 30, 100, 100, 60, 0x0};
	//drawRect(holdrect, framebuffer);
	//pos.x = 190;
	//pos.y = 100;
	//size.x = 100;
	//size.y = 60;
    //this->renderer->draw_fill_rect(pos, size, 0x0);
    this->renderer->draw_fill_rect(airboy::Vector2i(190, 100), airboy::Vector2i(100, 60), 0x0);

	//if (hold != -1)
		//for (int i = 0; i < 4; i++) {
			//Sprite holdsprite =	{10*BLOCKSIZE + 50 + block_lookup[hold][0][i][0]*BLOCKSIZE, 105 + block_lookup[hold][0][i][1]*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE, blocks[hold], 0,0,0,0,0,0,};
			//drawSprite(holdsprite, framebuffer);
		//}
	if (hold != nullptr) {
		hold->draw();
	}
	//drawText(10*BLOCKSIZE + 58, 142, 2, 0xFBDE, "Hold", framebuffer);
	//pos.x = 218;
	//pos.y = 142;
	//this->renderer->draw_text(pos, 2, 0xFBDE, "Hold");
	this->renderer->draw_text(airboy::Vector2i(218, 142), 2, 0xFBDE, "Hold");

	if (state == PAUSE) {
		//drawText(30, 80, 3, 0x9EF7, "Paused", framebuffer);
		//pos.x = 30;
		//pos.y = 80;
		//this->renderer->draw_text(pos, 3, 0x9ef7, "Paused");
		this->renderer->draw_text(airboy::Vector2i(30, 80), 3, 0xFBDE, "Paused");
		//drawText(35, 150, 2, 0x9EF7, "Press", framebuffer);
		//pos.x = 35;
		//pos.y = 150;
		//this->renderer->draw_text(pos, 2, 0x9ef7, "Press");
		this->renderer->draw_text(airboy::Vector2i(35, 150), 2, 0xFBDE, "Press");
		//drawText(35, 164, 2, 0x9EF7, "start", framebuffer);
		//pos.x = 35;
		//pos.y = 164;
		//this->renderer->draw_text(pos, 2, 0x9ef7, "start");
		this->renderer->draw_text(airboy::Vector2i(35, 164), 2, 0xFBDE, "start");
	} else if (state == DEAD) {
		//drawText(30, 80, 4, 0x9EF7, "Game", framebuffer);
		//pos.x = 30;
		//pos.y = 80;
		//this->renderer->draw_text(pos, 4, 0x9ef7, "Game");
		this->renderer->draw_text(airboy::Vector2i(30, 80), 4, 0xFBDE, "Game");
		//drawText(30, 108, 4, 0x9EF7, "over", framebuffer);
		//pos.x = 30;
		//pos.y = 108;
		//this->renderer->draw_text(pos, 4, 0x9ef7, "over");
		this->renderer->draw_text(airboy::Vector2i(30, 108), 4, 0xFBDE, "over");
		//drawText(35, 150, 2, 0x9EF7, "Press", framebuffer);
		//pos.x = 35;
		//pos.y = 150;
		//this->renderer->draw_text(pos, 2, 0x9ef7, "Press");
		this->renderer->draw_text(airboy::Vector2i(35, 150), 2, 0xFBDE, "Press");
		//drawText(35, 164, 2, 0x9EF7, "start", framebuffer);
		//pos.x = 35;
		//pos.y = 164;
		//this->renderer->draw_text(pos, 2, 0x9ef7, "start");
		this->renderer->draw_text(airboy::Vector2i(35, 164), 2, 0xFBDE, "start");
	} else if (state == MENU) {
		//drawText(0, 80, 3, 0x9EF7, "MIJAKSPAD", framebuffer);
		//pos.x = 0;
		//pos.y = 80;
		//this->renderer->draw_text(pos, 3, 0x9ef7, "MIJAKSPAD");
		this->renderer->draw_text(airboy::Vector2i(0, 80), 3, 0xFBDE, "MIJAKSPAD");
		if (difficulty == EASY) {
			//drawText(35, 140, 2, 0x9EF7, "> Easy <", framebuffer);
			//pos.x = 35;
			//pos.y = 140;
			//this->renderer->draw_text(pos, 2, 0x9ef7, "> Easy <");
			this->renderer->draw_text(airboy::Vector2i(35, 140), 2, 0xFBDE, "> Easy <");
		} else if (difficulty == MEDIUM) {
			//drawText(25, 140, 2, 0x9EF7, "> Medium <", framebuffer);
			//pos.x = 25;
			//pos.y = 140;
			//this->renderer->draw_text(pos, 2, 0x9ef7, "> Medium <");
			this->renderer->draw_text(airboy::Vector2i(25, 140), 2, 0xFBDE, "> Medium <");
		} else {
			//drawText(35, 140, 2, 0x9EF7, "> Hard <", framebuffer);
			//pos.x = 35;
			//pos.y = 140;
			//this->renderer->draw_text(pos, 2, 0x9ef7, "> Hard <");
			this->renderer->draw_text(airboy::Vector2i(35, 140), 2, 0xFBDE, "> Hard <");
		}
		//drawText(54, 172, 2, 0x9EF7, "Press", framebuffer);
		//pos.x = 54;
		//pos.y = 172;
		//this->renderer->draw_text(pos, 2, 0x9ef7, "Press");
		this->renderer->draw_text(airboy::Vector2i(54, 172), 2, 0xFBDE, "Press");
		//drawText(54, 186, 2, 0x9EF7, "start", framebuffer);
		//pos.x = 54;
		//pos.y = 186;
		//this->renderer->draw_text(pos, 2, 0x9ef7, "start");
		this->renderer->draw_text(airboy::Vector2i(54, 186), 2, 0xFBDE, "start");
		this->renderer->draw_text(airboy::Vector2i(23, 220), 1, 0x9ef7, "Press menu for help");
	} else if (state == HELP) {
		this->renderer->draw_text(airboy::Vector2i(30, 40), 1, 0x9ef7, "A - ROTATE RIGHT");
		this->renderer->draw_text(airboy::Vector2i(30, 50), 1, 0x9ef7, "B - ROTATE LEFT");
		this->renderer->draw_text(airboy::Vector2i(30, 60), 1, 0x9ef7, "X - HARDFALL");
		this->renderer->draw_text(airboy::Vector2i(30, 70), 1, 0x9ef7, "Y - HOLD PIECE");
		this->renderer->draw_text(airboy::Vector2i(30, 80), 1, 0x9ef7, "DOWN - SPEEDUP");
		this->renderer->draw_text(airboy::Vector2i(54, 172), 2, 0x9ef7, "Press");
		this->renderer->draw_text(airboy::Vector2i(54, 186), 2, 0x9ef7, "start");
		//this->renderer->draw_text(airboy::Vector2i(5, 40), 2, 0x9ef7, "A - ROTATE RIGHT");
		//this->renderer->draw_text(airboy::Vector2i(5, 58), 2, 0x9ef7, "B - ROTATE LEFT");
		//this->renderer->draw_text(airboy::Vector2i(5, 76), 2, 0x9ef7, "X - HARDFALL");
		//this->renderer->draw_text(airboy::Vector2i(5, 94), 2, 0x9ef7, "Y - HOLD PIECE");
		//this->renderer->draw_text(airboy::Vector2i(5, 112), 2, 0x9ef7, "DOWN - SPEEDUP");
	}
}


void Mijakspad::changedifficulty(bool up)
{
	if (up) {
		if (difficulty == EASY)
			difficulty = HARD;
		else if (difficulty == MEDIUM)
			difficulty = EASY;
		else if (difficulty == HARD)
			difficulty = MEDIUM;
	} else {
		if (difficulty == EASY)
			difficulty = MEDIUM;
		else if (difficulty == MEDIUM)
			difficulty = HARD;
		else if (difficulty == HARD)
			difficulty = EASY;
	}
}


void Mijakspad::drawboard()
{
	for (int ix = 0; ix < 10; ix++) {
		for (int iy = 0; iy < 15; iy++) {
			if (board[iy][ix] != -1)
				drawsprite(ix * BLOCKSIZE, iy * BLOCKSIZE, board[iy][ix]);
		}
	}
}


void Mijakspad::newgame()
{
	state = PLAY;

	for (int i = 0; i < 15; i++)
		for (int j = 0; j < 10; j++)
			board[i][j] = -1;

	bag = new Bag();
	piece c = bag->generatepiece();
	current = new Tetromino((c == O ? 4 : 3) * BLOCKSIZE, 0, c, this->display);
	next = new Tetromino(10 * BLOCKSIZE + 50, 25, bag->generatepiece(), this->display);
}


void Mijakspad::checklines()
{
	int numlines = 0;
	for (int i = 1; i < 15; i++) {
		bool line = true;
		for (int j = 0; j < 10; j++) {
			if (board[i][j] == -1) {
				line = false;
				break;
			}
		}
		if (line) {
			numlines++;
			for (int j = 0; j < 10; j++) {
				board[i][j] = -1;
			}
			for (int j = i; j > 0; j--)
				for (int k = 0; k < 10; k++) {
					board[j][k] = board[j-1][k];
				}
			for (int j = 0; j < 10; j++) {
				board[0][j] = -1;
			}
		}
	}
	switch(numlines){
		case 1:
			score += 100;
			break;
		case 2:
			score += 300;
			break;
		case 3:
			score += 500;
			break;
		case 4:
			score += 800;
			break;
		default:
			break;
	}
}


void Mijakspad::handleinput()
{
	if (state == DEAD) {
		// if (input->is_just_pressed(airboy::Buttons::BUTTON_START))
		if (input->is_just_pressed(airboy::Buttons::BUTTON_SELECT))
			state = MENU;
	} else if (state == PAUSE) {
		// if (input->is_just_pressed(airboy::Buttons::BUTTON_START))
		if (input->is_just_pressed(airboy::Buttons::BUTTON_SELECT))
			state = PLAY;
	} else if (state == HELP) {
		// if (input->is_just_pressed(airboy::Buttons::BUTTON_START))
		if (input->is_just_pressed(airboy::Buttons::BUTTON_SELECT))
			state = MENU;
	} else if (state == MENU) {
		// if (input->is_just_pressed(airboy::Buttons::BUTTON_START))
		if (input->is_just_pressed(airboy::Buttons::BUTTON_SELECT))
			newgame();
		else if (input->is_just_pressed(airboy::Buttons::BUTTON_AIRBOY))
			state = HELP;
		else {
			// if (input->is_just_pressed(airboy::Buttons::BUTTON_SELECT)) {
			if (input->is_just_pressed(airboy::Buttons::BUTTON_START)) {
				changedifficulty(false);
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_DPAD_DOWN)) {
				changedifficulty(false);
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_DPAD_UP)) {
				changedifficulty(true);
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_DPAD_LEFT)) {
				changedifficulty(false);
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_DPAD_RIGHT)) {
				changedifficulty(true);
			}
		}
	} else if (state == PLAY) {
		// if (input->is_just_pressed(airboy::Buttons::BUTTON_START))
		if (input->is_just_pressed(airboy::Buttons::BUTTON_SELECT))
			state = PAUSE;
		else {
			if (input->is_just_pressed(airboy::Buttons::BUTTON_ACTION_A)) {
				current->rotate(false);
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_ACTION_B)) {
				current->rotate(true);
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_ACTION_Y)) {
				if (!usedhold) {
					if (hold == nullptr) {
						hold = next;
						next = new Tetromino(10 * BLOCKSIZE + 50, 25, bag->generatepiece(), this->display);
					}

					Tetromino *temp = current;

					current = hold;
					current->setx((current->gettype() == O ? 4 : 3) * BLOCKSIZE);
					current->sety(0);

					hold = temp;
					hold->setx(10 * BLOCKSIZE + 50);
					hold->sety(105);
					hold->setrotation(0);

					usedhold = true;
				}
			}
			if (input->is_just_pressed(airboy::Buttons::BUTTON_ACTION_X)) {
				current->hardfall();

				checklines();

				free(current);

				current = next;
				current->setx((current->gettype() == O ? 4 : 3) * BLOCKSIZE);
				current->sety(0);

				next = new Tetromino(10 * BLOCKSIZE + 50, 25, bag->generatepiece(), this->display);
				usedhold = false;
			}
			// if (input->is_pressed(airboy::Buttons::BUTTON_DPAD_LEFT)) {
			if (input->is_pressed(airboy::Buttons::BUTTON_DPAD_RIGHT)) {
				if (leftcounter == 0 || lefttimeout == 0) {
					current->move(true);
					leftcounter = leftcounter == 0 ? 20 : 10;
					lefttimeout = leftcounter;
				} else
					lefttimeout--;
			} else
				lefttimeout = 0;
			// if (input->is_pressed(airboy::Buttons::BUTTON_DPAD_RIGHT)) {
			if (input->is_pressed(airboy::Buttons::BUTTON_DPAD_LEFT)) {
				if (rightcounter == 0 || righttimeout == 0) {
					current->move(false);
					rightcounter = rightcounter == 0 ? 20 : 10;
					righttimeout = rightcounter;
				} else
					righttimeout--;
			} else
				righttimeout = 0;
			if (input->is_pressed(airboy::Buttons::BUTTON_DPAD_DOWN)) {
				if (downcounter == 0 || downtimeout == 0) {
					speedup = 10;
					downcounter = downcounter == 0 ? 10 : 2;
					downtimeout = downcounter;
				} else
					downtimeout--;
			} else {
				downtimeout = 0;
				speedup = 1;
			}
		}
	}
}


Bag::Bag()
{
	fillbag();
}


void Bag::fillbag()
{
	memset(bag, false, sizeof(bag));
	bag_size = sizeof(bag);
}


piece Bag::generatepiece()
{
	if (bag_size == 0)
		fillbag();

	int dice_roll = (int) (1.0 * bag_size / 4294967295 * esp_random());
	bag_size--;

	while(bag[dice_roll]) {
		dice_roll--;
		if (dice_roll < 0)
			dice_roll = 6;
	}

	bag[dice_roll] = true;

	return (piece) dice_roll;
}


Tetromino::Tetromino(int x, int y, piece type, airboy::Display *display)
{
	rotation = 0;
	this->x = x;
	this->y = y;
	this->type = type;
	sprite = (int) type;
	this->display = display;

	moveblocks();
}


void Tetromino::moveblocks()
{
	bx = by = 9999;
	bw = bh = 0;

	for (int i = 0; i < 4; i++) {
		block_offsets[i][0] = block_lookup[(int) type][rotation][i][0] * BLOCKSIZE;

		if (block_offsets[i][0] < bx)
			bx = block_offsets[i][0];
		if (block_offsets[i][0] + BLOCKSIZE > bw)
			bw = block_offsets[i][0];

		block_offsets[i][1] = block_lookup[(int) type][rotation][i][1] * BLOCKSIZE;

		if (block_offsets[i][1] < by)
			by = block_offsets[i][1];
		if (block_offsets[i][1] + BLOCKSIZE > bh)
			bh = block_offsets[i][1];
	}
}


void Tetromino::setblocks()
{
	for (int i = 0; i < 4; i++) {
		int ix = x / 16 + block_offsets[i][0] / 16;
		int iy = y / 16 + block_offsets[i][1] / 16;

		board[iy][ix] = sprite;	
	}
}


bool Tetromino::checkcollision()
{
	if (x + bw >= 160 || y + bh >= 240 || x + bx < 0 || y + by < 0)
		return true;

	for (int i = 0; i < 4; i++) {
		int ix = x / 16 + block_offsets[i][0] / 16;
		int iy = y / 16 + block_offsets[i][1] / 16;

		if (board[iy][ix] != -1)
			return true;
	}

	return false;
}


void Tetromino::draw()
{
	for (int i = 0; i < 4; i++) {
		for (int ix = 0; ix < BLOCKSIZE; ix++) {
			for (int iy = 0; iy < BLOCKSIZE; iy++) {
				display->set_pixel_fast(x + ix + block_offsets[i][0], y + iy + block_offsets[i][1], blocks[(int) type][iy * 16 + ix]);
			}
		}
	}
}


bool Tetromino::fall()
{
	y += BLOCKSIZE;
	
	moveblocks();

	if (checkcollision()) {
		y -= BLOCKSIZE;
		moveblocks();
		setblocks();

		return true;
	} else {
		return false;
	}
}


void Tetromino::hardfall()
{
	while (!fall());
}


void Tetromino::setx(int x)
{
	this->x = x;
}


void Tetromino::sety(int y)
{
	this->y = y;
}


int Tetromino::gety()
{
	return y;
}


piece Tetromino::gettype()
{
	return type;
}


void Tetromino::setrotation(int rotation)
{
	this->rotation = rotation;

	moveblocks();
}


void Tetromino::rotate(bool left)
{
	int diff = left ? -1 : 1;

	int oldrot = rotation;
	rotation += diff;

	if (rotation < 0)
		rotation += 4;
	else if (rotation > 3)
		rotation -= 4;

	moveblocks();
	
	if (checkcollision()) {
		rotation = oldrot;	
		moveblocks();
	}
}


void Tetromino::move(bool left)
{
	int oldx = x;
	x += 16 - 32 * (int) left;

	moveblocks();

	if (checkcollision()) {
		moveblocks();
		x = oldx;
	}
}


extern "C" void app_main(void)
{
    Mijakspad game;
	if (game.construct(60))
		game.run();
	else
		ESP_LOGE(TASKNAME, "Construct error");
}
