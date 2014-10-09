#include "LevelEditStage.h"

#include "GUISystem.h"
#include "GameInterface.h"
#include "RenderSystem.h"
#include "TextureManager.h"

#include "Level.h"
#include "Light.h"
#include "Trigger.h"

#include "GlobalVariable.h"
#include "DataPath.h"
#include "RenderUtility.h"

#include "FixString.h"
#include <fstream>

bool LevelEditStage::onInit()
{
	if( !BaseClass::onInit() )
		return false;


	mEditTileMeta = 0;
	mEditTileType = TID_FLAT;

	postavljaLight = false;
	mStepEdit=0;
	sr=1.0; sg=10; sb=1.0; si=8.0; srad=128.0;

	{
		GFrame* frame = new GFrame( UI_MAP_TOOL , Vec2i(32,32), Vec2i(320, 240) , NULL );
		//"Tools"
		GUISystem::getInstance().addWidget( frame );

		GImageButton* button;

		button = new GImageButton( UI_CREATE_LIGHT , Vec2i(16,32),Vec2i(32,32) , frame );
		button->texImag = getGame()->getTextureMgr()->getTexture("button_light.tga");

		button = new GImageButton( UI_CREATE_TRIGGER ,Vec2i(64,32),Vec2i(32,32) , frame );
		button->texImag = getGame()->getTextureMgr()->getTexture("button_light.tga");

		button = new GImageButton( UI_EMPTY_MAP  ,Vec2i(16,72),Vec2i(32,32) , frame );
		button->texImag = getGame()->getTextureMgr()->getTexture("button_gen.tga");

		button = new GImageButton( UI_SAVE_MAP ,Vec2i(64,72),Vec2i(32,32) , frame );
		button->texImag = getGame()->getTextureMgr()->getTexture("button_save.tga");
	}


	return true;
}

void LevelEditStage::onExit()
{
	BaseClass::onExit();
}

void LevelEditStage::onUpdate( float deltaT )
{	
	float speed=250;

	if( Platform::isKeyPressed( Keyboard::eLSHIFT ) )
		speed=750;
	if( Platform::isKeyPressed( Keyboard::eLEFT ) || Platform::isKeyPressed( Keyboard::eA ) )
		mCamera->setPos(mCamera->getPos()+Vec2f(-speed*deltaT,0));
	if( Platform::isKeyPressed( Keyboard::eRIGHT ) || Platform::isKeyPressed( Keyboard::eD) )
		mCamera->setPos(mCamera->getPos()+Vec2f(speed*deltaT,0));
	if( Platform::isKeyPressed( Keyboard::eUP ) || Platform::isKeyPressed( Keyboard::eW ) )
		mCamera->setPos(mCamera->getPos()+Vec2f(0, -speed*deltaT));
	if( Platform::isKeyPressed( Keyboard::eDOWN ) || Platform::isKeyPressed( Keyboard::eS ) )
		mCamera->setPos(mCamera->getPos()+Vec2f(0, speed*deltaT));

	if(Platform::isKeyPressed(Keyboard::eNUMPAD7))
		sr+=0.5*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD4))
		sr-=0.5*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD8))
		sg+=0.5*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD5))
		sg-=0.5*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD9))
		sb+=0.5*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD6))
		sb-=0.5*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD2))
		si+=50*deltaT;
	if(Platform::isKeyPressed(Keyboard::eNUMPAD1))
		si-=50*deltaT;
	if(Platform::isKeyPressed(Keyboard::eI))
		srad+=50*deltaT;
	if(Platform::isKeyPressed(Keyboard::eK))
		srad-=50*deltaT;
	if(sr<0.0)
		sr=0.0;
	if(sr>1.0)
		sr=1.0;
	if(sg<0.0)
		sg=0.0;
	if(sg>1.0)
		sg=1.0;
	if(sb<0.0)
		sb=0.0;
	if(sb>1.0)
		sb=1.0;
	if(si<0.0)
		si=0.0;

	getGame()->procSystemEvent();

	if(postavljaLight==true)
	{
		mEditLight->radius=srad;
		mEditLight->setColorParam(Vec3f(sr,sg,sb),si);
		mEditLight->setPos( convertToWorldPos( getGame()->getMousePos() ) );
	}
}



void LevelEditStage::onRender()
{

	RenderEngine* renderEngine = getGame()->getRenderEenine();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//mWorldScaleFactor = 0.5f;

	mRenderParam.camera      = mCamera;
	mRenderParam.level       = mLevel;
	mRenderParam.scaleFactor = mWorldScaleFactor;
	mRenderParam.mode        = RM_ALL;

	renderEngine->renderScene( mRenderParam );

	Player* player = mLevel->getPlayer();
	glLoadIdentity();

	if ( mPause )
	{
		glEnable(GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glColor4f( 0 , 0 , 0 , 0.8 );
		drawRect( Vec2f(0.0, 0.0) , Vec2f( getGame()->getScreenSize() ) );
		glDisable(GL_BLEND);
	}

	GUISystem::getInstance().render();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	drawSprite( Vec2f( getGame()->getMousePos() ) - Vec2f(16,16) ,Vec2f(32,32), mTexCursor );
	glDisable(GL_BLEND);


	Vec2f posCursor = convertToWorldPos( getGame()->getMousePos() );
	

	FixString< 256 > str;
	str.format( "x = %f , y = %f " , posCursor.x , posCursor.y );
	mDevMsg->setString( str );
	getRenderSystem()->drawText( mDevMsg , Vec2i( 10 , 10 ) , TEXT_SIDE_LEFT | TEXT_SIDE_RIGHT );

}

bool LevelEditStage::onMouse( MouseMsg const& msg )
{
	if( msg.onLeftDown() )
	{
		Vec2f wPos = convertToWorldPos( msg.getPos() );
		if(postavljaLight==true)
		{
			postavljaLight=false;
		}

		switch( mStepEdit )
		{
		case 1:
			t1= wPos;
			mStepEdit=2;
			break;
		case 2:
			t2 = wPos;
			mStepEdit=3;
			break;
		case 3:
			t3 = wPos;
			mEditTrigger->init(t1,t2);
			mLevel->addOjectInternal( mEditTrigger );
			mEditTrigger = NULL;
			mStepEdit=0;
			break;

		}

	}			
	else if( msg.onRightDown() )
	{				
		TileMap& terrain = mLevel->getTerrain();

		Vec2i tPos = convertToTilePos( getGame()->getMousePos() );

		if ( terrain.checkRange( tPos.x , tPos.y ) )
		{
			Tile& tile = terrain.getData( tPos.x , tPos.y );
			tile.type = mEditTileType;
			tile.meta = mEditTileMeta;
		}
	}
	return false;
}

bool LevelEditStage::onKey( unsigned key , bool isDown )
{
	if ( !isDown )
	{
		switch( key )
		{
		case Keyboard::eF1:
			GUISystem::getInstance().findTopWidget( UI_MAP_TOOL )->destroy();
			stop();
			break;
		case Keyboard::eF4:
			{
				RenderEngine* renderEngine = getGame()->getRenderEenine();
				if( renderEngine->getAmbientLight().x==0.1f)
					renderEngine->setAmbientLight( Vec3f(0.8f, 0.8f, 0.8f) );
				else
					renderEngine->setAmbientLight( Vec3f(0.1f, 0.1f, 0.1f) );
			}
			break;
		case Keyboard::eF6:
			{
				string path = LEVEL_DIR;
				path += gMapFileName;
				saveLevel( path.c_str() );				
			}
			break;
		case Keyboard::eG:
			{
				Vec2f wPos = convertToWorldPos( getGame()->getMousePos() );
				cout << "X: " << wPos.x << endl;
				cout << "Y: " << wPos.y << endl;
			}
			break;

		case Keyboard::eNUM1:
			mEditTileType = TID_FLAT;
			mEditTileMeta = 0;
			break;
		case Keyboard::eNUM2:		
			mEditTileType = TID_WALL;
			mEditTileMeta = 0;
			break;
		case Keyboard::eNUM3:			
			mEditTileType = TID_GAP;
			mEditTileMeta = 0;
			break;
		case Keyboard::eNUM4:
			mEditTileType = TID_DOOR;
			mEditTileMeta = DOOR_RED;
			break;
		case Keyboard::eNUM5:	
			mEditTileType = TID_DOOR;
			mEditTileMeta = DOOR_GREEN;
			break;
		case Keyboard::eNUM6:	
			mEditTileType = TID_DOOR;
			mEditTileMeta = DOOR_BLUE;
			break;
		case Keyboard::eNUM7:
			mEditTileType = TID_ROCK;
			mEditTileMeta = 2000;
			break;
		}
	}
	return BaseClass::onKey( key , isDown );
}

void LevelEditStage::onWidgetEvent( int event , int id , GWidget* sender )
{
	switch( id )
	{
	case UI_SAVE_MAP:
		{
			string path = LEVEL_DIR;
			path += gMapFileName;
			saveLevel( path.c_str() );
		}
		break;
	case UI_EMPTY_MAP:
		{
			generateEmptyLevel();
		}
		break;
	case UI_CREATE_TRIGGER:
		if( mStepEdit == 0 )
		{		
			mEditTrigger = new AreaTrigger;
			mStepEdit    = 1;
		}
		break;
	case UI_CREATE_LIGHT:
		if(postavljaLight==false)
		{
			postavljaLight = true;
			mEditLight = mLevel->createLight( getGame()->getMousePos() , 128 , true);
			mEditLight->setColorParam(Vec3f(1.0, 1.0, 1.0), 8);
		}
		break;
	}
}

bool LevelEditStage::saveLevel( char const* path )
{
	std::ofstream of( path );

	if ( !of.is_open() )
		return false;

	TileMap& terrain = mLevel->getTerrain();

	of << terrain.getSizeX() << " " << terrain.getSizeY() << "\n";

	for(int i=0; i< terrain.getSizeX(); i++)
	for(int j=0; j< terrain.getSizeY(); j++)
	{
		Tile& tile = terrain.getData( i , j );
		if ( tile.type == TID_FLAT && tile.meta == 0 )
			continue;

		of << "block" << " "  
		   << i << " "  << j << " " 
		   << (int)tile.type << " " 
		   << (int)tile.meta << "\n";		
	}


	LightList lights = mLevel->getLights();
	for( LightList::iterator iter = lights.begin() , itEnd = lights.end();
		iter != itEnd ; ++iter )
	{
		Light* light = *iter;
		if( light->isStatic )
		{
		  of << "light " 
			 << light->getPos().x << " " 
			 << light->getPos().y << " " 
			 << light->radius << " " 
			 << light->intensity << " " 
			 << light->color.x << " " 
			 << light->color.y << " " 
			 << light->color.z << "\n";
		}
	}
	of.close ();

	cout << "Map Saved." << endl;

	return true;
}

void LevelEditStage::generateEmptyLevel()
{
	TileMap& terrain = mLevel->getTerrain();
	for(int i=0; i< terrain.getSizeX() ; i++)
	{
		for(int j=0; j< terrain.getSizeY(); j++)
		{		
			Tile& tile = terrain.getData( i , j );
			tile.type = TID_FLAT;
			tile.meta = 0;
			if(i==0 || j==0 || i== terrain.getSizeX()-1 || j== terrain.getSizeY() -1 )
				tile.type = TID_WALL;
		}	
	}

	LightList& lights = mLevel->getLights();
	for(LightList::iterator iter = lights.begin();
		iter != lights.end() ; )
	{
		Light* light = *iter;
		if( light->isStatic )
		{
			++iter;
			delete light;

		}
		else
		{
			++iter;
		}
	}
}
