#ifndef Game_h__
#define Game_h__

#include "GameInterface.h"
#include "Base.h"
#include "Dependence.h"
#include "GameStage.h"
#include "IntegerType.h"


#include <vector>

class Game : public IGame
{
public:
	Game();

	bool init(char* configFile);
	void run();
	void exit();

	virtual void  addStage( GameStage* stage, bool removePrev );
	virtual void  stopPlay(){ mNeedEnd = true; }
	virtual void  procWidgetEvent( int event , int id , GWidget* sender );
	virtual void  procSystemEvent();

	sf::RenderWindow* getWindow();
	sf::Font*         getFont( int idx ){  return mFonts[idx]; }

private:

	float    mFPS;
	unsigned mMouseState;
	bool     mNeedEnd;
	std::vector<GameStage*> mStageStack;
	std::vector<sf::Font*>  mFonts;
	sf::RenderWindow mWindow;

};




#endif // Game_h__