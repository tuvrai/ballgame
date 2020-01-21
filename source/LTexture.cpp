#include "LTexture.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
using namespace std;
using namespace ballgame;

LTexture::LTexture()
{
	//Initialize
	//std::cout << "done.";
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::LTexture(string filepath)
{
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
	filePath = filepath;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		//cout << getWidth() << endl;
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}
