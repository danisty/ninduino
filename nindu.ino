#include <TFT_ST7735.h>
#include <SPI.h>

TFT_ST7735 screen = TFT_ST7735();

#define TFT_W 160
#define TFT_H 128

class Button {
	public:
		bool pressed;
		bool HasBeenPressed() {
			if (!digitalRead(this->_pin) && !this->pressed && millis() - this->_ms > 200) {
				this->pressed = true;
				this->_ms = millis();
				return true;
			} else if (digitalRead(this->_pin)) {
				this->pressed = false;
			}
			return false;
		};
		bool IsPressed() {
			return !digitalRead(this->_pin);
		}
		Button(uint8_t pin) {
			this->_pin = pin;
		};
	private:
		uint8_t _pin;
		uint32_t _ms = 0;
};

int8_t selectedGame = 0;
char* games[3] = {"Pong", "Snek", "..."};
bool playing = false;

Button upArrow = Button(4);
Button rightArrow = Button(3);
Button leftArrow = Button(7);
Button downArrow = Button(2);

Button backButton = Button(8);
Button acceptButton = Button(6);

void setup() {
	// Serial.begin(9600);

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
			// Setup selected game
			char* game = games[selectedGame];

			if (game == "Pong") InitPong();
			else if (game == "Snek") InitSnek();
			else if (game == "...") InitSadness();
			
			playing = true;
		} 
	} else {
		if (backButton.HasBeenPressed()) {
			playing = false;
			gameSelector();
			return;
		}

		// Render selected game
		char* game = games[selectedGame];

		if (game == "Pong") Pong();
		else if (game == "Snek") Snek();
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
		screen.drawString("NINDUINO", TFT_W/2, TFT_H/2-17, 2);
		delay(200);
	}

	for (uint8_t i=0; i < 4; i++) {
		screen.setTextColor(random(0x0000, 0xFFFF), TFT_WHITE);
		screen.drawString("NINDUINO", TFT_W/2, TFT_H/2-17, 2);
		delay(500);
	}

	for (uint8_t i=0; i < 3; i++) {
		uint16_t color = i==0 ? 0xbdd7 : i==1 ? 0x6b4d : TFT_BLACK;
		screen.fillScreen(color);
		screen.setTextColor(TFT_BLACK, color);
		screen.drawString("NINDUINO", TFT_W/2, TFT_H/2-17, 2);
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
int16_t pong_midline_x = TFT_W/2;

uint8_t pong_paddle_w = 3;
uint8_t pong_paddle_h = 25;

int16_t pong_lpaddle_x = 0;
int16_t pong_lpaddle_y = pong_midline_x + pong_paddle_h/2;

int16_t pong_rpaddle_x = TFT_W - pong_paddle_w;
int16_t pong_rpaddle_y = pong_lpaddle_y;

int16_t pong_old_ball_x = 0;
int16_t pong_old_ball_y = 0;
int16_t pong_ball_x = 0;
int16_t pong_ball_y = 0;
uint8_t pong_ball_p = 4;

int8_t pong_ball_dx = 1;
int8_t pong_ball_dy = 1;

uint16_t pong_p1_score = 0;
uint16_t pong_p2_score = 0;

bool pong_throwing_ball = true;

uint32_t pong_last_shoot = 0;
uint8_t pong_delay = 0;
uint8_t pong_predicted_y = 0;

void InitPong() {
	screen.fillScreen(TFT_BLACK);

	pong_p1_score = 0;
	pong_p2_score = 0;

	pong_throwing_ball = true;

	pong_ball_dx = 1;
	pong_ball_dy = 1;

	pong_lpaddle_y = pong_midline_x - pong_paddle_h;
	pong_rpaddle_y = pong_lpaddle_y;
	
	screen.setTextSize(1);
	screen.setTextColor(TFT_WHITE, TFT_BLACK);

	PongDrawPaddles();
	PongDrawMiddle(true);
	PongDrawScores();
}

void Pong() {
	uint32_t ms = millis() - pong_last_shoot;
	if (ms > 10000) pong_delay = 4;
	else if (ms > 20000) pong_delay = 3;
	else pong_delay = 5;

	// User inputs
	if (upArrow.IsPressed() || rightArrow.IsPressed()) {
		screen.fillRect(0, pong_lpaddle_y + pong_paddle_h, pong_paddle_w, 1, TFT_BLACK);
		if (pong_lpaddle_y > 0) pong_lpaddle_y--;
	}
	if (downArrow.IsPressed() || leftArrow.IsPressed()) {
		screen.fillRect(0, pong_lpaddle_y, pong_paddle_w, 1, TFT_BLACK);
		if (pong_lpaddle_y < TFT_H - pong_paddle_h) pong_lpaddle_y++;
	}
	
	if (pong_throwing_ball) {
		if (acceptButton.HasBeenPressed()) {
			pong_throwing_ball = false;
			pong_last_shoot = millis();
		}

		pong_old_ball_x = pong_ball_x;
		pong_old_ball_y = pong_ball_y;
		pong_ball_y = pong_lpaddle_y + pong_paddle_h/2;
		pong_ball_x = 6;

		PongDrawPaddles();
		PongDrawBall();
	 	PongPredictBall();

		return delay(5);
	}

	// For some reason these variables randomly become 0
	if (pong_ball_dx == 0) pong_ball_dx = 1;
	if (pong_ball_dy == 0) pong_ball_dy = 1;

	pong_old_ball_x = pong_ball_x;
	pong_old_ball_y = pong_ball_y;
	pong_ball_x += pong_ball_dx;
	pong_ball_y += pong_ball_dy;

	// Basic AI
	if (pong_ball_x >= pong_midline_x - 20) {
		int8_t direction = pong_predicted_y - (pong_rpaddle_y + pong_paddle_h/2);
		
		if (direction < 0) {
			screen.fillRect(pong_rpaddle_x, pong_rpaddle_y + pong_paddle_h, pong_paddle_w, 1, TFT_BLACK);
			if (pong_rpaddle_y > 0) pong_rpaddle_y--;
		} else if (direction > 0) {
			screen.fillRect(pong_rpaddle_x, pong_rpaddle_y, pong_paddle_w, 1, TFT_BLACK);
			if (pong_rpaddle_y < TFT_H - pong_paddle_h) pong_rpaddle_y++;
		}
	}

	PongDrawPaddles();
	PongDrawMiddle(false);
	PongDrawBall();

	// Ball hit bounds detection
	if (pong_ball_x == -1 || pong_ball_x == TFT_W - pong_ball_p + 1) {
		pong_ball_dx *= -1;
		if (pong_ball_x == -1){
			pong_p2_score++;
			pong_throwing_ball = true;
		} else {
			pong_p1_score++;
			delay(1000);

			pong_old_ball_x = pong_ball_x;
			pong_old_ball_y = pong_ball_y;

			pong_ball_x = TFT_W - 6 - pong_ball_p;
			pong_ball_y = pong_rpaddle_y + pong_paddle_h/2;
		}

		PongDrawScores();
		PongDrawBall();
		PongDrawPaddles();

		delay(1000);
		pong_last_shoot = millis();

		return;
	}
	if (pong_ball_y < 0 || pong_ball_y > TFT_H - pong_ball_p) pong_ball_dy *= -1;

	// Ball hit paddle detection
	if (pong_ball_x == pong_paddle_w && pong_ball_y >= pong_lpaddle_y - 2 && pong_ball_y <= pong_lpaddle_y + pong_paddle_h) {
		pong_ball_dx = 1;
	 	PongPredictBall();
	}
	if (pong_ball_x == pong_rpaddle_x - pong_ball_p && pong_ball_y >= pong_rpaddle_y - 2 && pong_ball_y <= pong_rpaddle_y + pong_paddle_h) pong_ball_dx = -1;

	// Redraw scores if ball passes through them
	if (pong_ball_y < 40 && pong_ball_x > pong_midline_x - 40 && pong_ball_x < pong_midline_x + 80) PongDrawScores();

	delay(pong_delay);
}

void PongDrawBall() {
	screen.fillRect(pong_old_ball_x, pong_old_ball_y, pong_ball_p, pong_ball_p, TFT_BLACK);
	screen.fillRect(pong_ball_x, pong_ball_y, pong_ball_p, pong_ball_p, TFT_WHITE);
}

void PongDrawScores() {
	char score1[8];
	char score2[8];

	sprintf(score1, "%i", pong_p1_score);
	sprintf(score2, "%i", pong_p2_score);

	screen.setTextDatum(TR_DATUM);
	screen.drawString(score1, pong_midline_x - 3, 1, 2);

	screen.setTextDatum(TL_DATUM);
	screen.drawString(score2, pong_midline_x + 5, 1, 2);
}

void PongDrawMiddle(bool ignorePosition) {
	if (!ignorePosition && (pong_ball_x < pong_midline_x - pong_ball_p && pong_ball_x > pong_midline_x + 2)) return;

	screen.setAddrWindow(pong_midline_x, 0, pong_midline_x + 1, TFT_H);

	for(int16_t i = 0; i < 32; i+=2) {
		screen.pushColor(TFT_WHITE, 4*2);
		screen.pushColor(TFT_BLACK, 4*2);
	}
}

void PongDrawPaddles() {
	screen.fillRect(pong_lpaddle_x, pong_lpaddle_y, pong_paddle_w, pong_paddle_h, TFT_WHITE);
	screen.fillRect(pong_rpaddle_x, pong_rpaddle_y, pong_paddle_w, pong_paddle_h, TFT_WHITE);
}

void PongPredictBall() {
	uint8_t newY = pong_ball_y + (TFT_W - pong_ball_x - pong_paddle_w) * pong_ball_dy;

	if (newY < 0) pong_predicted_y = -newY;
	else if (newY > TFT_H) pong_predicted_y = 2 * TFT_H - newY;
	else pong_predicted_y = newY;

	bool fail = random(100) % 7 == 0;
	if (fail) pong_predicted_y += ((pong_rpaddle_y - pong_predicted_y) > 0 ? 1 : -1) * (pong_paddle_h / 2 + 4);
}

const uint8_t snek_rows = 12;
const uint8_t snek_columns = 12;

uint8_t snek_cell_size = 120 / snek_columns;

uint8_t snek_x = 0;
uint8_t snek_y = 0;
int8_t snek_direction = 0;

uint8_t snek_apple_position = 0;

uint16_t snek_score = 0;

uint8_t snek_length = 0;
uint8_t snek_body[snek_rows * snek_columns] = { 0 };

uint32_t snek_last_movement = 0;
bool hasChangedDirection = false;

void InitSnek() {
	screen.fillScreen(TFT_BLACK);
	screen.drawRect((TFT_W - snek_rows * snek_cell_size) / 2, 4, snek_rows * snek_cell_size + 2, 122, TFT_WHITE);

	snek_score = 0;

	snek_x = 2;
	snek_y = snek_rows / 2;
	snek_direction = 2;

	// Head + tail
	snek_body[0] = snek_y * snek_rows + snek_x;
	snek_body[1] = snek_y * snek_rows + snek_x - 1;
	snek_body[2] = snek_y * snek_rows + snek_x - 2;
	snek_length = 3;

	SnekDrawBodyPart(0, false);
	SnekDrawBodyPart(1, false);
	SnekDrawBodyPart(2, false);

	SnekDrawScore();
	SnekSpawnApple();

	snek_last_movement = millis();
}

void Snek() {
	uint8_t oldDir = snek_direction;

	// User inputs
	if (!hasChangedDirection) { // Avoid changing direction twice in a movement
		if (upArrow.IsPressed() && snek_direction != 3) snek_direction = 1;
		else if (rightArrow.IsPressed() && snek_direction != 4) snek_direction = 2;
		else if (downArrow.IsPressed() && snek_direction != 1) snek_direction = 3;
		else if (leftArrow.IsPressed() && snek_direction != 2) snek_direction = 4;
		
		hasChangedDirection = oldDir != snek_direction;
	}

	// Wait for next movement while listening for input
	if (millis() - snek_last_movement < 200) return delay(5);
	hasChangedDirection = false;

	switch (snek_direction)
	{
	case 1:
		snek_y--;
		break;
	case 2:
		snek_x++;
		break;
	case 3:
		snek_y++;
		break;
	case 4:
		snek_x--;
		break;
	}

	// Check if snek hit a wall or itself
	uint8_t snekRawPosition = snek_y * snek_rows + snek_x;
	if (snek_x >= snek_columns || snek_y >= snek_rows || SnekIsTouchingSnek(snekRawPosition)) {
		char score[8];
		sprintf(score, "%i", snek_score);

		screen.setTextSize(3);
		screen.setTextColor(0x54FF);
		screen.drawCentreString(score, TFT_W/2, TFT_H/2 - 25, 2);

		delay(2000);
		InitSnek();
		return;
	}

	// Draw tail for changed position
	SnekDrawBodyPart(snek_length - 1, true);
	for (uint8_t i=snek_length - 1; i > 0; i--) {
		snek_body[i] = snek_body[i - 1];
	}

	snek_body[0] = snekRawPosition;
	SnekDrawBodyPart(0, false);

	// Check if apple was collected
	if (snekRawPosition == snek_apple_position) {
		snek_score++;
		snek_body[snek_length++] = -1;
		
		SnekDrawScore();
		SnekSpawnApple();
	}

	snek_last_movement = millis();
}

bool SnekIsTouchingSnek(uint8_t position) {
	for (uint8_t i=0; i < snek_length; i++) {
		if (snek_body[i] == position) {
			return true;
		}
	}
	return false;
}

void SnekDrawBodyPart(uint8_t i, bool clear) {
	uint8_t rawPos = snek_body[i];
	uint8_t x = 21 + (rawPos % snek_rows) * snek_cell_size + 1;
	uint8_t y = 5 + (rawPos / snek_rows) * snek_cell_size + 1;

	if (clear) {
		screen.fillRect(x - 1, y - 1, snek_cell_size, snek_cell_size, TFT_BLACK);
	} else {
		uint8_t previousRawPos = snek_body[min(i + 1, snek_length - 1)];
		uint8_t pX = 21 + (previousRawPos % snek_rows) * snek_cell_size + 1;
		uint8_t pY = 5 + (previousRawPos / snek_rows) * snek_cell_size + 1;

		// Too lazy to make this part any more decent
		if (x > pX) screen.fillRect(x - 2, y, snek_cell_size, snek_cell_size - 2, TFT_WHITE);
		else if (x < pX) screen.fillRect(x, y, snek_cell_size, snek_cell_size - 2, TFT_WHITE);
		else if (y > pY) screen.fillRect(x, y - 2, snek_cell_size - 2, snek_cell_size, TFT_WHITE);
		else if (y < pY) screen.fillRect(x, y, snek_cell_size - 2, snek_cell_size, TFT_WHITE);
		else screen.fillRect(x, y, snek_cell_size - 2, snek_cell_size - 2, TFT_WHITE);
	}
}

void SnekDrawScore() {
	char score[8];
	sprintf(score, "%i", snek_score);

	screen.setTextSize(1);
	screen.setTextColor(TFT_WHITE, TFT_BLACK);
	screen.drawCentreString(score, 10, TFT_H/2 - 7, 2);
}

void SnekSpawnApple() {
	snek_apple_position = snek_body[0];
	while (SnekIsTouchingSnek(snek_apple_position)) snek_apple_position = random(0, 9) * snek_rows + random(0, 9);

	uint8_t x = 21 + (snek_apple_position % snek_rows) * snek_cell_size + snek_cell_size / 2;
	uint8_t y = 5 + (snek_apple_position / snek_rows) * snek_cell_size + snek_cell_size / 2;

	screen.fillCircle(x, y, snek_cell_size/2 - 2, TFT_RED);
}

// Lack of time to make any other game
void InitSadness() {
	screen.fillScreen(TFT_BLACK);
	
	screen.setTextColor(TFT_WHITE);
	screen.drawCentreString("Falta de tiempo :(", TFT_W / 2, TFT_H / 2 - 7, 2);
}
