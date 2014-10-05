#include "Collision.h"

#include "Object.h"


ColBody::ColBody()
	:mOffset( 0,0 )
	,halfSize( 0,0 )
{
	idxCell = -1;
	colMask = 0;
	colMaskCheck = 0;
}

void ColBody::updateCache()
{
	cachePos = mOffset + object->getPos();
	boundBox.min = cachePos - halfSize;
	boundBox.max = cachePos + halfSize;
}

CollisionManager::CollisionManager() 
	:mTerrain( NULL )
	,mCellLength( 0 )
{

}

void CollisionManager::setup( float width , float height  , float cellLength  )
{
	int cx = int ( width / cellLength - 1 ) + 1;
	int cy = int ( height / cellLength - 1 ) + 1;
	mCellMap.resize( cx , cy );
	mCellLength = cellLength;
}

bool CollisionManager::testCollision( ColInfo& info , Vec2f const& offset , ColBody& body , unsigned maskCheckReplace )
{
	unsigned maskCheck = ( maskCheckReplace ) ? maskCheckReplace : body.colMaskCheck;

	assert( body.halfSize.x < mCellLength / 2 && body.halfSize.y < mCellLength / 2 );

	Rect bBox;
	bBox.min = body.boundBox.min + offset;
	bBox.max = body.boundBox.max + offset;

	if ( maskCheck & COL_TERRAIN )
	{
		Tile* tile = testTerrainCollision( bBox , body.colMask );
		if ( tile )
		{
			info.isTerrain = true;
			info.tile = tile;
			return true;
		}
	}

	if ( maskCheck & COL_OBJECT )
	{
		int cx , cy;
		calcCellPos( body.cachePos + offset , cx , cy );

		for( int oy = -1 ; oy <= 1 ; ++oy )
		{
			for( int ox = -1 ; ox <= 1 ; ++ox )
			{
				if ( !mCellMap.checkRange( cx + ox , cy + oy ) )
					continue;

				int idxCell = mCellMap.toIndex( cx + ox , cy + oy );

				Cell& cell = mCellMap[ idxCell ];


				for ( CellBodyList::iterator iter = cell.bodies.begin() , itEnd = cell.bodies.end();
					iter != itEnd ; ++iter )
				{
					ColBody& bodyTest = *iter;

					if ( &bodyTest == &body )
						continue;

					if ( ( maskCheck & bodyTest.colMask ) == 0 )
						continue;

					if ( !bBox.intersect( bodyTest.boundBox ) )
						continue;

					info.isTerrain = false;
					info.body = &bodyTest;

					return true;
				}
			}
		}
	}


	return false;
}

bool CollisionManager::checkCollision( ColBody& body )
{
	int cx , cy;
	assert( body.halfSize.x < mCellLength / 2 && body.halfSize.y < mCellLength / 2 );
	calcCellPos( body.cachePos , cx , cy );

	bool result = false;

	if ( body.colMaskCheck & COL_TERRAIN )
	{
		Tile* tile = testTerrainCollision( body.boundBox , body.colMask );

		if ( tile )
		{
			body.object->onTileCollision( body , *tile );
			result = true;
		}
	}

	if ( body.colMaskCheck & COL_OBJECT )
	{
		for( int oy = -1 ; oy <= 1 ; ++oy )
		{
			for( int ox = -1 ; ox <= 1 ; ++ox )
			{
				if ( !mCellMap.checkRange( cx + ox , cy + oy ) )
					continue;

				int idxCell = mCellMap.toIndex( cx + ox , cy + oy );

				Cell& cell = mCellMap[ idxCell ];


				for ( CellBodyList::iterator iter = cell.bodies.begin() , itEnd = cell.bodies.end();
					iter != itEnd ; ++iter )
				{
					ColBody& bodyTest = *iter;

					if ( &bodyTest == &body )
						continue;

					unsigned mask = body.colMaskCheck & bodyTest.colMask;

					if ( mask == 0 )
						continue;

					if ( !body.boundBox.intersect( bodyTest.boundBox ) )
						continue;

					body.object->onBodyCollision( body , bodyTest );
					result = true;
				}
			}
		}
	}

	return result;
}

void CollisionManager::addBody( LevelObject& obj , ColBody& body )
{
	assert( body.idxCell = -1 );
	body.object = &obj;

	Vec2f posBody = body.object->getPos() + body.getOffset();

	int cx , cy;
	calcCellPos( posBody , cx , cy );
	int idxCell = mCellMap.toIndex( cx , cy );

	body.idxCell = idxCell;
	mCellMap[ idxCell ].bodies.push_back( body );

	mBodies.push_back( body );
}

void CollisionManager::removeBody( ColBody& body )
{
	body.idxCell = -1;
	body.cellHook.unlink();
	body.managerHook.unlink();
}

void CollisionManager::updateBody( ColBody& body )
{
	body.updateCache();
	int cx , cy;
	calcCellPos( body.cachePos , cx , cy );

	int idxCell = mCellMap.toIndex( cx , cy );
	if ( idxCell != body.idxCell )
	{
		body.cellHook.unlink();
		mCellMap[ idxCell ].bodies.push_back( body );
		body.idxCell = idxCell;
	}
}

void CollisionManager::update()
{
	for( BodyList::iterator iter = mBodies.begin() , itEnd = mBodies.end(); 
		iter != itEnd ; ++iter )
	{
		ColBody& body = *iter;
		updateBody( body );
	}

	for( BodyList::iterator iter = mBodies.begin() , itEnd = mBodies.end(); 
		iter != itEnd ; ++iter )
	{
		ColBody& body = *iter;
		checkCollision( body );
	}
}

void CollisionManager::calcCellPos( Vec2f const& pos , int& cx , int& cy )
{
	cx = Math::clamp( int( pos.x / mCellLength ) , 0 , mCellMap.getSizeX() - 1 );
	cy = Math::clamp( int( pos.y / mCellLength ) , 0 , mCellMap.getSizeY() - 1 );
}

Tile* CollisionManager::rayTerrainTest( Vec2f const& from , Vec2f const& to , unsigned colMask  )
{
	Vec2i tpFrom = Vec2i( Math::floor( from.x / BLOCK_SIZE ) , Math::floor( from.y / BLOCK_SIZE ) );


	Tile* out;

	if ( out = rayBlockTest( tpFrom , from , to , colMask ) )
		return out;

	Vec2i tpCur  = tpFrom;
	Vec2i tpTo   = Vec2i( Math::floor( to.x / BLOCK_SIZE ) , Math::floor( to.y / BLOCK_SIZE ) );
	Vec2i tpDif  = tpTo - tpFrom;

	if ( tpDif.x == 0 )
	{
		int delta = tpDif.y > 0 ? 1 : -1;
		while( tpCur.y != tpTo.y )
		{
			tpCur.y += delta;
			if ( out = rayBlockTest( tpCur , from , to , colMask ) )
				return out;	
		}
	}
	else if ( tpDif.y == 0 )
	{
		int delta = tpDif.x > 0 ? 1 : -1;
		while( tpCur.x != tpTo.x )
		{
			tpCur.x += delta;
			if ( out = rayBlockTest( tpCur , from , to , colMask ) )
				return out;	
		}
	}
	else
	{
		Vec2f flac = from / float( BLOCK_SIZE ) - Vec2f( tpFrom );
		Vec2f dif = to - from;
		float slopeFactor = dif.y / dif.x;
		if ( slopeFactor < 0 )
			slopeFactor = -slopeFactor;

		int deltaX = -1;
		if ( tpDif.x > 0 )
		{
			flac.x = 1 - flac.x;
			deltaX = 1;
		}

		int deltaY = -1;
		if ( tpDif.y > 0 )
		{
			flac.y = 1 - flac.y;
			deltaY = 1;
		}

		for(;;)
		{
			float yOff = flac.x * slopeFactor;

			if ( flac.y > yOff )
			{
				flac.y -= yOff;
				flac.x = 1;
				tpCur.x += deltaX;
			}
			else
			{
				flac.x -= flac.y / slopeFactor;
				flac.y = 1;
				tpCur.y += deltaY;
			}

			if ( out = rayBlockTest( tpCur , from , to , colMask ) )
				return out;

			if ( tpCur.x == tpTo.x && tpCur.y == tpTo.y )
				break;
		}
	}
	return NULL;
}

Tile* CollisionManager::rayBlockTest( Vec2i const& tPos , Vec2f const& from , Vec2f const& to , unsigned colMask )
{
	if ( !mTerrain->checkRange( tPos.x , tPos.y ) )
		return NULL;

	Tile& tile = mTerrain->getData( tPos.x , tPos.y );
	Block* block = Block::FromType( tile.type );

	if ( ( block->getColMask() & colMask ) == 0 )
		return NULL;

	if ( block->checkFlag( BF_NONSIMPLE ) )
	{
		if ( !block->rayTest( tile , from , to ) )
			return NULL;
	}

	return &tile;
}

Tile* CollisionManager::testTerrainCollision( Rect const& bBox , unsigned colMask )
{
	TileMap& terrain = *mTerrain;

	int xMin = Math::clamp( Math::floor( bBox.min.x / BLOCK_SIZE ) , 0 , terrain.getSizeX() - 1 );
	int xMax = Math::clamp( Math::floor( bBox.max.x / BLOCK_SIZE ) , 0 , terrain.getSizeX() - 1 );
	int yMin = Math::clamp( Math::floor( bBox.min.y / BLOCK_SIZE ) , 0 , terrain.getSizeY() - 1 );
	int yMax = Math::clamp( Math::floor( bBox.max.y / BLOCK_SIZE ) , 0 , terrain.getSizeY() - 1 );

	for( int x = xMin; x <= xMax ; ++x )
	{
		for(int y = yMin; y <= yMax; ++y  )
		{
			Tile& tile = terrain.getData( x , y );
			Block* block = Block::FromType( tile.type );

			if ( ( block->getColMask() & colMask ) == 0 )
				continue;

			Rect bBoxOther;
			bBoxOther.min = tile.pos;
			bBoxOther.max = tile.pos + Vec2f(BLOCK_SIZE,BLOCK_SIZE);

			if( !bBox.intersect(bBoxOther) )
				continue;

			if ( block->checkFlag( BF_NONSIMPLE ) )
			{
				if ( !block->testIntersect( tile , bBox ) )
					continue;
			}

			return &tile;
		}
	}
	return NULL;
}

void CollisionManager::findBody( Rect const& bBox , unsigned colMask , ColBodyVec& out )
{
	Vec2i cMin , cMax;
	calcCellPos( bBox.min , cMin.x , cMin.y );
	calcCellPos( bBox.max , cMax.x , cMax.y );

	for( int i = cMin.x ; i <= cMax.x ; ++i )
	{
		for( int j = cMin.y ; j <= cMax.y ; ++j )
		{
			Cell& cell = mCellMap.getData( i , j );

			for ( CellBodyList::iterator iter = cell.bodies.begin() , itEnd = cell.bodies.end();
				iter != itEnd ; ++iter )
			{
				ColBody& bodyTest = *iter;

				if ( ( colMask & bodyTest.colMask ) == 0 )
					continue;
	
				if ( i == cMin.x || i == cMax.x || 
					 i == cMin.x || i == cMax.x )
				{
					if ( !bBox.hitTest( bodyTest.cachePos ) )
						continue;
				}

				out.push_back( &bodyTest );
			}
		}
	}

}
