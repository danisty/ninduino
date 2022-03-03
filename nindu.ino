#include <TFT_ST7735.h>
#include <SPI.h>
#include "MemoryFree.h"

TFT_ST7735 screen = TFT_ST7735();

#define TFT_W 160
#define TFT_H 128

class Button {
	public:
		boolean pressed;
		boolean HasBeenPressed() {
			if (!digitalRead(this->_pin) && !this->pressed) {
				this->pressed = true;
				return true;
			} else if (digitalRead(this->_pin)) {
				this->pressed = false;
			}
			return false;
		};
		Button(uint8_t pin) {
			this->_pin = pin;
		};
	private:
		uint8_t _pin;
};

int8_t selectedGame = 0;
char* games[3] = {"Pong", "Snake", "Color Switch"};
boolean playing = false;

Button upArrow = Button(2);
Button downArrow = Button(3);
Button acceptButton = Button(4);

void setup() {
	Serial.begin(9600);

	screen.init();
	screen.setRotation(1);

	initScreen();
	gameSelector();
}

void loop() {
	if (!playing) {
		if (downArrow.HasBeenPressed()) {
			navigateGames(1);
		} else if (upArrow.HasBeenPressed()) {
			navigateGames(-1);
		} else if (acceptButton.HasBeenPressed()) {
			playing = true;

			// Setup selected game
			char* game = games[selectedGame];
			if (game == "Pong") {
				screen.fillScreen(TFT_BLACK);
				Serial.print("freeMemory()=");
    			Serial.println(freeMemory());
			}
		}
	} else {
		char* game = games[selectedGame];
		if (game == "Pong") {
			Pong();
		}
	}
}

void navigateGames(uint8_t dir) {
	int8_t oldSelection = selectedGame;
	selectedGame += dir;

	if (selectedGame > 2) selectedGame = 0;
	if (selectedGame < 0) selectedGame = 2;

	char oldGame[10];
	sprintf(oldGame, "  %s", games[oldSelection]);
	screen.setTextColor(TFT_WHITE, 0x0862);
	screen.drawString(oldGame, 30, 52 + oldSelection*15, 2);

	char newGame[10];
	sprintf(newGame, "> %s", games[selectedGame]);
	screen.setTextColor(0x54FF, 0x0862);
	screen.drawString(newGame, 30, 52 + selectedGame*15, 2);
}

void initScreen() {
	screen.fillScreen(TFT_WHITE);

	screen.setTextSize(2);
	screen.setTextDatum(TC_DATUM);

	for (uint8_t i=0; i < 3; i++) {
		screen.setTextColor(i==0 ? 0xbdd7 : i==1 ? 0x6b4d : TFT_BLACK, TFT_WHITE);
		screen.drawString("NINDUINO", TFT_W/2, TFT_H/2-20, 2);
		delay(200);
	}

	for (uint8_t i=0; i < 4; i++) {
		screen.setTextColor(random(0x0000, 0xFFFF), TFT_WHITE);
		screen.drawString("NINDUINO", TFT_W/2, TFT_H/2-20, 2);
		delay(500);
	}

	for (uint8_t i=0; i < 3; i++) {
		uint16_t color = i==0 ? 0xbdd7 : i==1 ? 0x6b4d : TFT_BLACK;
		screen.fillScreen(color);
		screen.setTextColor(TFT_BLACK, color);
		screen.drawString("NINDUINO", TFT_W/2, TFT_H/2-20, 2);
		delay(100);
	}
}

void gameSelector() {
	for (uint8_t i=0; i < 3; i++) { // Fade to background color
		screen.fillScreen(i==0 ? TFT_BLACK : i==1 ? 0x0841 : 0x0862);
		delay(100);
	}

	screen.drawRect(6, 6, TFT_W-12, TFT_H-12, TFT_WHITE);

	screen.setTextSize(1);
	screen.setTextColor(TFT_WHITE);
	screen.setTextDatum(TC_DATUM);

	screen.drawString("JUEGOS", TFT_W/2, 25, 2);
	screen.setTextDatum(0);

	for (uint8_t i=0; i < 3; i++) {
		char name[10];
		sprintf(name, i == selectedGame ? "> %s" : "  %s", games[i]);
		screen.setTextColor(i == selectedGame ? 0x54FF : TFT_WHITE, 0x0862);
		screen.drawString(name, 30, 52 + i*15, 2);
	}
}

// Pong
uint16_t pong_paddle_w = 4;
uint16_t pong_paddle_h = 20;

uint16_t pong_lpaddle_x = 0;
uint16_t pong_lpaddle_y = TFT_H/2 + pong_paddle_h/2;

uint16_t pong_rpaddle_x = TFT_W - pong_paddle_w;
uint16_t pong_rpaddle_y = pong_lpaddle_y;

int16_t pong_ball_x = 0;
int16_t pong_ball_y = 0;
uint16_t pong_ball_r = 2;

int16_t pong_ball_dx = 1;
int16_t pong_ball_dy = 1;

void Pong() {
	screen.fillCircle(pong_ball_x, pong_ball_y, pong_ball_r, TFT_BLACK);

	pong_ball_x += pong_ball_dx;
	pong_ball_y += pong_ball_dy;

	if (pong_ball_x < 0 || pong_ball_x > TFT_W - pong_ball_r*2) pong_ball_dx *= -1;
	if (pong_ball_y < 0 || pong_ball_y > TFT_H - pong_ball_r*2) pong_ball_dy *= -1;

	screen.fillCircle(pong_ball_x, pong_ball_y, pong_ball_r, TFT_WHITE);
	delay(5);
}