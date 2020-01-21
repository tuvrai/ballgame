#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <SDL_ttf.h>
#include <string>
#include <cmath>
#include <iostream>
#include "LTexture.h"
#include <fstream>

using namespace std;


namespace ballgame
{
	void splitStringLevels(string text, char ch, int row, int id);
	void splitStringData(string text, char ch, int id);
	bool loadLevelInfo(int levelnumber);
	bool loadLevelPattern(int levelnumber);
	bool loadLevel(int levelid);
	void defineBlocks(int levelid);
	bool renderBlocks();


	void levelEndText(bool isWin);
	void levelBeginText(int levelid);
	void renderHud();

	void checkBlocksHit();
	void handleEndLevel();

	void drawFrame();

	//Run the game
	bool run();
	//Initialize all sdl components
	bool init();
	//Loads media files needed
	bool loadMedia();
	//Closes all sdl components
	void close();
	//Sets drawing color
	void setDrawColor(int r, int g, int b);

	//Main window of the game, where everything appears.
	SDL_Window* screen = NULL;
	//Fixed width of the screen
	const int screen_width = 1024;
	//Fixed height of the screen
	const int screen_height = 768;
	//Rendering object, generating images on canvas.
	SDL_Renderer* gameRend = NULL;
	
	//Texture for ball rendering
	LTexture ballTex;

	//Main font
	TTF_Font* mainFont = NULL;

	//Rendered texture
	LTexture gTextTexture;

	//Loads texture using existing file image.
	bool LTexture::loadFromFile(std::string path)
	{
		//Destroys existing texture
		free();

		//The final texture
		SDL_Texture* newTexture = NULL;

		//Load image at specified path
		SDL_Surface* loadedSurface = IMG_Load(path.c_str());
		if (loadedSurface == NULL)
		{
			printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
		}
		else
		{
			//Color key image
			SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

			//Create texture from surface pixels
			newTexture = SDL_CreateTextureFromSurface(gameRend, loadedSurface);
			if (newTexture == NULL)
			{
				printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
			}
			else
			{
				//Get image dimensions
				mWidth = loadedSurface->w;
				mHeight = loadedSurface->h;
			}

			//Get rid of old loaded surface
			SDL_FreeSurface(loadedSurface);
		}

		//Return success
		mTexture = newTexture;
		return mTexture != NULL;
	}

	//renders texture to screen in specified conditions.
	void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
	{
		//Set rendering space and render to screen
		SDL_Rect renderQuad = { x, y, mWidth, mHeight };
		if (clip != NULL)
		{
			renderQuad.w = clip->w;
			renderQuad.h = clip->h;
		}
		SDL_RenderCopyEx(gameRend, mTexture, clip, &renderQuad, angle, center, flip);
	}

	//structure containing rgb color values. 
	struct color
	{
		int red = 255;
		int green = 255;
		int blue = 255;
		int alfa = 255;
	};

	//Structure defining state of the ongoing gameplay.
	struct GameState
	{
		//Points got in the game.
		int points = 0;
		//Player's health.
		int health = 3;
		//Defines whether the racket is focused on changing x-speed (true) or y-speed (false).
		bool speedChangeX = false;
		//Defines whether the HUD of the game is visible.
		bool hudVisible = false;
		//Returns what the current level is.
		int currentLevel = 1;
		//Flag defining whether the game is paused or not.
		bool pause = false;
		//Defines color of the text; default is white.
		SDL_Color textColor = { 255, 255, 255 };
		/**
		Since array of levels starts from 0, but naturally level counter starts from 1, we need to substract 1 to synchronize.
		Usage as array's element id.
		**/
		int getLevel()
		{
			return currentLevel - 1;
		}
	};

	//Main gameplay state object.
	GameState gamestate;

	//Structure defining level block patterns.
	struct Level
	{
		//Number id of level.
		int id;
		//Number of rows starting from the top straight to the last not empty row.
		int rowsHeight;
		//Defines starting width of the racket.
		int racketWidthIni;
		//Defines starting x-velocity of the ball.
		int vxIni;
		//Defines starting y-velocity of the ball.
		int vyIni;
		//Defines maximum any velocity in the level.
		int vMax;

		int row1[10];
		int row2[10];
		int row3[10];
		int row4[10];
		int row5[10];
	};

	//Main array of levels, containing patterns of blocks for each level.
	Level levels[5];

	//Defines racket objects.
	class Racket
	{
	public:

		Racket()
		{
			width = 200;
			height = 20;
			pos = static_cast<int>(screen_width / 2) - static_cast<int>(width / 2);
		}

		//width of the racket
		int width;
		//height of the racket
		int height;
		//x-position of the racket, left edge of it.
		int pos;
		//Defines direction in which the racket is moving. False = left ; True = right;
		bool dir = false;
		//Defines whether the racket is moving (true) or no (false)
		bool isMoving = false;
		//Defines speed (x change per frame)
		double speed = 10;
		/**
		Structure defining the color of the racket.
		Properties respectively: red,green,blue,alfa
		**/
		color mColor = {
		255,255,0,255
		};
		//Texture where racket is drawn
		SDL_Texture* mTex = SDL_CreateTexture(gameRend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, screen_width, screen_height);

		/**
		Changes direction of the racket movement
		'l' - left
		'r' - right
		'n' - no movement
		**/
		void setDir(char direction)
		{
			if (direction == 'l') {
				dir = false;
				isMoving = true;
			}
			else if (direction == 'r') {
				dir = true;
				isMoving = true;
			}
			else if (direction == 'n') {
				isMoving = false;
			}
		}

		//Changes position of racket if the direction is set to left or right
		void move()
		{
			if (isMoving)
			{
				if ((!dir) && (pos - 10 >= 0)) pos -= speed;
				else if (dir && (pos + width + 10 <= screen_width)) pos += speed;

			}
		};

		//Renders the racket on its texture and draws it on canvas
		void render()
		{
			SDL_Rect borderRect = { pos, screen_height - height - 10, width, height };
			SDL_SetRenderTarget(gameRend, mTex);
			setDrawColor(mColor.red, mColor.green, mColor.blue);
			SDL_RenderFillRect(gameRend, &borderRect);
			SDL_RenderCopy(gameRend, mTex, NULL, &borderRect);
			SDL_SetRenderTarget(gameRend, NULL);
		};
	};

	//Main racket steered by the player.
	Racket racket = Racket();

	//Defines the ball objects.
	class Ball {
	public:
		Ball()
		{
			radius = 10;
			posX = static_cast<int>(screen_width / 2);
			posY = static_cast<int>(screen_height / 1.75);
		}
		//The radius of the ball.
		int radius;
		//x-position of the ball.
		int posX;
		//y-position of the ball.
		int posY;
		/**
		Structure defining the color of the racket.
		Properties respectively: red,green,blue,alfa
		**/
		color mColor = {
		255,255,0,255
		};
		//Texture where ball is drawn
		//SDL_Texture* mTex = SDL_CreateTexture(gameRend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, screen_width, screen_height);

		//Defines whether ball is moving
		bool isMoving = false;
		//velocity in x-direction
		float vx = 0;
		//velocity in y-direction
		float vy = 5;

		/**
		Counts frames from last racket bounce, to block it from multiple bouncing in a few frames straight.
		0 - means ready for next bounce.
		**/
		int justBounced = 0;
		//Defines how many frames ball will not be able to bounce from the racket.
		int bounceBlock = 30;

		//Changes position of the ball
		void move()
		{
			if (isMoving) {
				if (justBounced) justBounced++;
				if (justBounced >= bounceBlock) {
					justBounced = 0;
				}
				if (posX + radius > screen_width - 10) //RIGHT EDGE CHECK
				{
					vx = -vx;
					//posX -= radius;
				}
				if (posX - radius < 0) //LEFT EDGE CHECK
				{
					vx = -vx;
					//posX += radius;
				}
				posX += vx;

				if (posY + radius * 2 > screen_height - 15 - racket.height && posX >= racket.pos && posX <= racket.pos + racket.width && justBounced == 0) //RACKET BOUNCE CHECK
				{
					justBounced = 1;
					int avy = abs(vy);
					int avx = abs(vx);
					if (!gamestate.speedChangeX)
					{
						if (avy < levels[gamestate.getLevel()].vMax) vy++;
						else vy -= 4;

						//if (avx >= 11) vx--;
						//if (avx < 4) vx++;
					}
					else
					{
						if (avx < levels[gamestate.getLevel()].vMax) {
							if (vx >= 0) vx++;
							if (vx < 0) vx--;
						}
						else {
							if (vx >= 0) vx -= 4;
							if (vx < 0) vx += 4;
						}

						//if (avy >= 11) vy--;
						//if (avy < 4) vy++;
					}
					vy = -vy;
					posY += vy;
				}
				else if (posY < 0) //TOP EDGE CHECK
				{
					vy = -vy;
				}
				else if (posY > screen_height) //BALL FALLS CHECK
				{
					posX = int(screen_width / 2);
					posY = int(screen_height / 1.5);
					vx = levels[gamestate.getLevel()].vxIni;
					vy = levels[gamestate.getLevel()].vyIni;

					racket.pos = static_cast<int>(screen_width / 2) - static_cast<int>(racket.width / 2);
					racket.isMoving = false;

					gamestate.health--;
					gamestate.points -= 10;
					if (gamestate.health <= 0)
					{
						handleEndLevel();
					}

				}
				posY += vy;
			}

		}

		//renders ball on its position
		void render()
		{
			ballTex.render(posX, posY);
		}


	};

	//Main ball of the game, bounced by the racket.
	Ball ball = Ball();

	//Defines the block objects hit by the ball.
	class Block
	{
	public:
		Block()
		{
			width = 100;
			height = 30;

			posX = -1;
			posY = -1;

			resistanceStart = 1;
		}

		Block(int x, int y)
		{
			width = 100;
			height = 30;

			posX = x;
			posY = y;

			resistanceStart = 1;
		}

		Block(int w, int h, int x, int y)
		{
			width = w;
			height = h;

			posX = x;
			posY = y;

			resistanceStart = 1;
		}
		//width of the block
		int width;
		//height of the block
		int height;
		//x-position of the block, left edge of it.
		int posX;
		//y-position of the block
		int posY;
		//Resistance at the beginning of the level (How many times we have to hit the block)
		int resistanceStart;
		//Resistance of the block at the moment (how many hits remaning to destroy)
		int resistanceNow;

		/**
		Structure defining the color of the block.
		Properties respectively: red,green,blue,alfa
		**/
		color mColor = {
		0,255,0,255
		};
		//Texture where block is drawn
		SDL_Texture* mTex = SDL_CreateTexture(gameRend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, screen_width, screen_height);
		//Renders the block on its texture and draws it on canvas
		void render()
		{
			SDL_Rect borderRect = { posX, posY, width, height };
			SDL_SetRenderTarget(gameRend, mTex);
			setDrawColor(mColor.red, mColor.green, mColor.blue);
			SDL_RenderFillRect(gameRend, &borderRect);
			SDL_RenderCopy(gameRend, mTex, NULL, NULL);
			SDL_SetRenderTarget(gameRend, NULL);
		};
	};

	//Array containing blocks of ongoing level.
	Block gameBlocks[50];

	//Changes color of the renderer drawing
	void setDrawColor(int r, int g, int b)
	{
		SDL_SetRenderDrawColor(gameRend, r, g, b, 0);
	}

	//Array containing general data (like amount of blocks, starting ball speed) for each level. It can be load using loadLevelData()
	int rawleveldata[4][6] = { };

	//Parses data from level data file
	void splitStringData(string text, char ch, int id)
	{
		int number;
		string cur = "";
		int properties = 0;
		for (int i = 0; i < text.size(); i++)
		{
			if (text[i] != ch) cur += text[i];
			else
			{
				number = stoi(cur);
				cur = "";
				rawleveldata[id][properties] = number;
				properties++;
			}
		}
		
	}

	//Loads general data of levels to the array
	bool loadLevelData()
	{
		fstream file;
		file.open("gamedata/levels.txt", ios::in);
		if (!file.is_open()) return false;
		string line = "";
		int linecount = 0;
		while (getline(file, line))
		{
			splitStringData(line, '_', linecount);
			linecount++;
		}
		return true;
	}

	//Parses data from file and puts it to the data of level
	void splitStringLevels(string text, char ch, int row, int id)
	{
		int number;
		string cur = "";
		int properties = 0;
		for (int i = 0; i < text.size(); i++)
		{
			if (text[i] != ch) cur += text[i];
			else
			{
				number = stoi(cur);
				cur = "";
				if (row == 1) levels[id].row1[properties] = number;
				else if (row == 2) levels[id].row2[properties] = number;
				else if (row == 3) levels[id].row3[properties] = number;
				else if (row == 4) levels[id].row4[properties] = number;
				else if (row == 5) levels[id].row5[properties] = number;
				properties++;
			}
		}

	}

	//load general information and starting values of the level
	bool loadLevelInfo(int levelnumber)
	{
		levelnumber--;

		levels[levelnumber].id = rawleveldata[levelnumber][0]; //load id of the level
		levels[levelnumber].rowsHeight = rawleveldata[levelnumber][1]; //loads number of rows
		levels[levelnumber].racketWidthIni = rawleveldata[levelnumber][2]; //loads racket width
		levels[levelnumber].vxIni = rawleveldata[levelnumber][3]; //loads x-velocity
		levels[levelnumber].vyIni = rawleveldata[levelnumber][4]; //loads y-velocity
		levels[levelnumber].vMax = rawleveldata[levelnumber][5]; //loads max velocity
		return true;
	}

	//Loads from file block pattern of chosen level - as an argument it takes the level's id.
	bool loadLevelPattern(int levelnumber)
	{
		fstream file;
		string filename = "gamedata/levels/level" + to_string(levelnumber) + ".txt";
		levelnumber--;
		file.open(filename, ios::in);
		if (!file.is_open()) return false;
		string line = "";
		int linecount = 1;
		while (getline(file, line))
		{
			splitStringLevels(line, '_', linecount, levelnumber);
			linecount++;
		}
		return true;
	}

	//Create blocks using level's pattern.
	void defineBlocks(int levelid)
	{
		levelid--;
		int resistance = 0;
		int blockid = 0;
		for (int row = 1; row <= 5; row++)
		{
			for (int column = 0; column < 10; column++)
			{
				if (row == 1) resistance = levels[levelid].row1[column];
				else if (row == 2) resistance = levels[levelid].row2[column];
				else if (row == 3) resistance = levels[levelid].row3[column];
				else if (row == 4) resistance = levels[levelid].row4[column];
				else if (row == 5) resistance = levels[levelid].row5[column];

				blockid = ((row - 1) * 10) + column;
				gameBlocks[blockid].width = 90;
				gameBlocks[blockid].height = 30;
				gameBlocks[blockid].posX = 40 + 95 * column;
				gameBlocks[blockid].posY = 60 + 35 * (row - 1);
				gameBlocks[blockid].resistanceNow = resistance;
			}
		}

	}

	//Loads all components of chosen level.
	bool loadLevel(int levelid)
	{
		loadLevelInfo(levelid);
		if (!loadLevelPattern(levelid))
		{
			printf("Failed to load %a level pattern.\n", levelid);
			return false;
		}
		defineBlocks(levelid);


		//cout << levels[levelid - 1].id << endl;
		//cout << levels[levelid - 1].rowsHeight << endl;

		levelid--;
		gamestate.health = 3;
		racket.pos = static_cast<int>(screen_width / 2) - static_cast<int>(racket.width / 2);
		racket.isMoving = false;
		racket.width = levels[levelid].racketWidthIni;

		ball.posX = int(screen_width / 2);
		ball.posY = int(screen_height / 1.5);
		ball.vx = levels[levelid].vxIni;
		ball.vy = levels[levelid].vyIni;

		return true;

	}

	//Renders all blocks of level and checks if all of them are destroyed.
	bool renderBlocks()
	{
		bool levelDone = true;
		for (int i = 0; i < 50; i++)
		{
			if (gameBlocks[i].resistanceNow == 0) continue;
			else levelDone = false;
			switch (gameBlocks[i].resistanceNow)
			{
			case 1:
				gameBlocks[i].mColor = {0,255,0,255};
				break;
			case 2:
				gameBlocks[i].mColor = { 51, 153, 102,255 };
				break;
			case 3:
				gameBlocks[i].mColor = { 0, 153, 255 , 255 };
				break;
			case 4:
				gameBlocks[i].mColor = { 51, 51, 255 , 255 };
				break;
			case 5:
				gameBlocks[i].mColor = { 204, 51, 25 , 255 };
				break;
			}
			gameBlocks[i].render();
		}
		return levelDone;
	}

	//Creates text to renderer in given color, font, dimensions and coordinates.
	void createText(std::string textureText, SDL_Color textColor, TTF_Font* font, int w, int h, int x, int y)
	{
		SDL_Surface* texSurface = TTF_RenderText_Solid(font, textureText.c_str(), textColor);
		SDL_Texture* tex = SDL_CreateTextureFromSurface(gameRend, texSurface);
		SDL_Rect tex_rect;
		tex_rect.w = w;
		tex_rect.h = h;
		tex_rect.x = x;
			tex_rect.y = y;
		SDL_SetRenderTarget(gameRend, tex);
		
		SDL_RenderCopy(gameRend, tex, NULL, &tex_rect);
		SDL_SetRenderTarget(gameRend, NULL);

		SDL_FreeSurface(texSurface);
		SDL_DestroyTexture(tex);

	}

	//Generates level starting text
	void levelBeginText(int levelid)
	{
		string text = "level " + to_string(levelid);
		int w = 300; 
		int h = 100; 
		int x = int(screen_width / 2) - int(w / 2); 
		int y = int(screen_height / 2) - int(h / 2);
		createText(text, gamestate.textColor, mainFont, w , h , x , y);
		createText("tuvrai | ballgame v1.0", gamestate.textColor, mainFont, 150, 15, 5, screen_height-20);
		gamestate.pause = true;
		SDL_RenderPresent(gameRend);
	}

	void levelEndText(bool isWin)
	{
		string text;
		if (isWin) text = "Congratulations! You won.";
		else text = "You lost. Try again.";
		int w = 400;
		int h = 100;
		int x = int(screen_width / 2) - int(w / 2);
		int y = int(screen_height / 2) - int(h / 2);
		createText(text, gamestate.textColor, mainFont, w, h, x, y);
		text = "Points: " + to_string(gamestate.points);
		createText(text, gamestate.textColor, mainFont, w, h, x, y+120);
		SDL_RenderPresent(gameRend);
	}

	//Renders HUD if enabled.
	void renderHud()
	{
		string temptext;
		temptext = "level:    " + to_string(gamestate.currentLevel);
		createText(temptext, gamestate.textColor, mainFont, 120, 20, 5, screen_height-150);
		temptext = "x-velocity: " + to_string(static_cast<int>(ball.vx));
		createText(temptext, gamestate.textColor, mainFont, 120, 20, 5, screen_height - 130);
		temptext = "y-velocity: " + to_string(static_cast<int>(ball.vy));
		createText(temptext, gamestate.textColor, mainFont, 120, 20, 5, screen_height - 110);
		string foc = (gamestate.speedChangeX) ? "x" : "y";
		temptext = "focus: " + foc;
		createText(temptext, gamestate.textColor, mainFont, 80, 20, 5, screen_height - 90);
		temptext = "health: " + to_string(gamestate.health);
		createText(temptext, gamestate.textColor, mainFont, 80, 20, 5, screen_height - 70);
		temptext = "points: " + to_string(gamestate.points);
		createText(temptext, gamestate.textColor, mainFont, 80, 20, 5, screen_height - 50);
	}

	//Checks if the ball has hit any of the blocks and reacts to that if necessary.
	void checkBlocksHit()
	{
		bool done = false;
		for (int i = 0; i < 50; i++)
		{
			if (gameBlocks[i].resistanceNow == 0) continue;
			if (ball.posY - ball.radius <= gameBlocks[i].posY + gameBlocks[i].height &&
				ball.posY + ball.radius >= gameBlocks[i].posY &&
				ball.posX - ball.radius < gameBlocks[i].posX + gameBlocks[i].width &&
				ball.posX + ball.radius > gameBlocks[i].posX)
			{
				gameBlocks[i].resistanceNow--;
				//cout << ball.posX << " " << ball.radius << " " << gameBlocks[i].posX << " " << gameBlocks[i].width << endl;
					//ball.vy = rint(0.9 * ball.vy);
					//ball.vx = rint(0.9 * ball.vx);
				gamestate.points++;
					
					ball.vy = -ball.vy;
					ball.posY += ball.vy;
					return;
				
			}
			/*else if (ball.posY - 5 >= gameBlocks[i].posY &&
				ball.posY + 5 <= gameBlocks[i].posY + gameBlocks[i].height &&
				( (ball.posX - ball.radius < gameBlocks[i].posX + gameBlocks[i].width && ball.posX + ball.radius > gameBlocks[i].posX && ball.vx < 0 ) ||
				(ball.posX + ball.radius > gameBlocks[i].posX && ball.posX + ball.radius < gameBlocks[i].posX + gameBlocks[i].width && ball.vx>0)	))
			{
				cout << "$"<< ball.posY << " " << ball.vx << " " << i << gameBlocks[i].posX << " " << gameBlocks[i].posY << endl;
				ball.vx = -ball.vx;
				ball.posX += ball.vx;
				return;
			}*/
		}
	}

	void handleEndLevel()
	{
		gamestate.currentLevel = 1;
		gamestate.hudVisible = false;
		loadLevel(1);
		setDrawColor(0, 0, 0);
		SDL_RenderClear(gameRend);
		levelEndText(false);
		gamestate.points = 0;
		gamestate.health = 3;
		gamestate.pause = true;
	}

	//Initializes all SDL components (libraries, window, renderer, etc.).
	bool init()
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
			return false;
		}
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
			return false;
		}
		screen = SDL_CreateWindow("ball game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
		if (screen == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			return false;
		}
		gameRend = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
		if (gameRend == NULL)
		{
			printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
			return false;
		}

		int imgFlags = IMG_INIT_PNG;
		if (!(IMG_Init(imgFlags) & imgFlags))
		{
			printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
			return false;
		}
		if (TTF_Init() == -1)
		{
			printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
			return false;
		}

		return true;
	}

	//Loads  all additional files like sound or images.
	bool loadMedia()
	{
		if (!ballTex.loadFromFile("gamedata/img/ball.bmp"))
		{
			printf("Failed to load images.\n");
			return false;
		}

		mainFont = TTF_OpenFont("gamedata/fonts/JosefinSans-Regular.ttf",150);
		if (mainFont == NULL)
		{
			printf("Failed to load fonts.\n");
			return false;
		}
		return true;
	}
	
	//Shuts down all SDL components and frees the memory.
	void close()
	{
		//Free media
		ballTex.free();
		//Destroy window	
		SDL_DestroyRenderer(gameRend);
		SDL_DestroyWindow(screen);
		screen = NULL;
		gameRend = NULL;

		delete racket.mTex;

		//Quit SDL subsystems
		IMG_Quit();
		SDL_Quit();
		TTF_Quit();
	}

	//Renders the frame
	void drawFrame()
	{
		setDrawColor(0, 0, 0);
		SDL_RenderClear(gameRend);
		checkBlocksHit();

		if (!renderBlocks())
		{
			if (gamestate.hudVisible) renderHud();


			ball.move();
			ball.render();
			racket.move();
			racket.render();
		}

		else
		{
			gamestate.pause = true;
			if (gamestate.currentLevel < 4)
			{

				gamestate.currentLevel++;
				loadLevel(gamestate.currentLevel);
				levelBeginText(gamestate.currentLevel);
			}
			else
			{
				levelEndText(true);
			}
		}
		SDL_RenderPresent(gameRend);
	}

	//Runs the game
	bool run()
	{
		if (!init())
		{
			printf("Failed to initialize!\n");
			return false;
		}
		if (!loadMedia())
		{
			printf("Failed to load media!\n");
			return false;
		}
		if (!loadLevelData())
		{
			printf("Failed to load levels!\n");
			return false;
		}
		if (!loadLevel(gamestate.currentLevel))
		{
			printf("Failed to load level.\n");
			return false;
		}
		levelBeginText(gamestate.currentLevel);
		//Flag defining whether the program is running or user quitted.
		bool quit = false;
		//Flag defining whether the game is paused or not.
		bool pause = false;
		//Event handling user's input.
		SDL_Event e;
		ball.isMoving = true;
		while (!quit)
		{
			while (SDL_PollEvent(&e) != 0)
			{
				if (e.type == SDL_QUIT)
				{
					quit = true;
				}
				if (e.type == SDL_KEYDOWN)
				{
					switch (e.key.keysym.sym)
					{
					case SDLK_p:
						gamestate.pause ^= true;
						break;
					case SDLK_h:
						gamestate.hudVisible ^= true;
						break;
					case SDLK_LEFT:
						racket.setDir('l');
						break;
					case SDLK_RIGHT:
						racket.setDir('r');
						break;
					case SDLK_UP:
						gamestate.speedChangeX ^= true;
						break;
					}
				}
			}
			if (!gamestate.pause) drawFrame();
			SDL_Delay(15);
		}
		close();
		return true;
	}

};

int main()
{
	ballgame::run();
	return 0;
}