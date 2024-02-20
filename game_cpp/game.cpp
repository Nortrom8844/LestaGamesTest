
#include <cassert>
#include <cmath>
#include <array>

#include "../framework/scene.hpp"
#include "../framework/game.hpp"
#include "../framework/engine.hpp"


//-------------------------------------------------------
//	Basic Vector2 class
//-------------------------------------------------------

class Vector2
{
public:
	float x = 0.f;
	float y = 0.f;

	constexpr Vector2() = default;
	constexpr Vector2( float vx, float vy );
	constexpr Vector2( Vector2 const &other ) = default;
};


constexpr Vector2::Vector2( float vx, float vy ) :
	x( vx ),
	y( vy )
{
}

Vector2 normalizedVector(Vector2 v)
{
    float vectorLength = sqrt(v.x * v.x + v.y * v.y);
    Vector2 newVector(v.x / vectorLength, v.y / vectorLength);

    return newVector;
}

//-------------------------------------------------------
//	game parameters
//-------------------------------------------------------

namespace Params
{
	namespace System
	{
		constexpr int targetFPS = 60;
	}

	namespace Table
	{
		constexpr float width = 15.f;
		constexpr float height = 8.f;
		constexpr float pocketRadius = 0.4f;

		static constexpr std::array< Vector2, 6 > pocketsPositions =
		{
			Vector2{ -0.5f * width, -0.5f * height },
			Vector2{ 0.f, -0.5f * height },
			Vector2{ 0.5f * width, -0.5f * height },
			Vector2{ -0.5f * width, 0.5f * height },
			Vector2{ 0.f, 0.5f * height },
			Vector2{ 0.5f * width, 0.5f * height }
		};

		static constexpr std::array< Vector2, 7 > ballsPositions =
		{
			// player ball
			Vector2( -0.3f * width, 0.f ),
			// other balls
			Vector2( 0.2f * width, 0.f ),
			Vector2( 0.25f * width, 0.05f * height ),
			Vector2( 0.25f * width, -0.05f * height ),
			Vector2( 0.3f * width, 0.1f * height ),
			Vector2( 0.3f * width, 0.f ),
			Vector2( 0.3f * width, -0.1f * height )
		};

	}

	namespace Physics
	{
	    float frictionDeceleration = 0.003;
	    float strikePower = 1.f;
	}

	namespace Ball
	{
		constexpr float radius = 0.3f;
	}

	namespace Shot
	{
		constexpr float chargeTime = 1.f;
	}
}


class BillBall{
    private:
        Vector2 position;
        Vector2 speed = Vector2(0, 0);

    public:
        Vector2 getPosition();
        Vector2 getSpeed();
        void setPosition(Vector2 newPosition);
        void setSpeed(Vector2 newSpeed);
        Vector2 getNextPosition(float dt);

        void strike(Vector2 direction, float force);

        Scene::Mesh* gameMesh;
        Scene::Mesh* getMesh();

        BillBall(Vector2 ballPosition, Scene::Mesh* mesh);
};


Vector2 BillBall::getPosition()
{
    return position;
}

Vector2 BillBall::getSpeed()
{
    return speed;
}

Scene::Mesh* BillBall::getMesh(){
    return gameMesh;
}

Vector2 BillBall::getNextPosition(float dt)
{
    return Vector2(position.x + speed.x, position.y + speed.y);
}

void BillBall::setPosition(Vector2 newPosition){
    position = Vector2(newPosition.x, newPosition.y);
}

void BillBall::setSpeed(Vector2 newSpeed){
    speed.x = newSpeed.x;
    speed.y = newSpeed.y;
}

BillBall::BillBall(Vector2 ballPosition, Scene::Mesh* mesh){
    position = Vector2 ( ballPosition.x, ballPosition.y);
    gameMesh = mesh;
}

void BillBall::strike(Vector2 direction, float power)
{
    direction = normalizedVector(direction);
    speed.x +=  direction.x * power;
    speed.y += direction.y * power;
}


//-------------------------------------------------------
//	Table logic
//-------------------------------------------------------

class Table
{
public:
	Table() = default;
	Table( Table const& ) = delete;

	void init();
	void deinit();

	std::array<BillBall*, 7> billBalls = {};
	BillBall* ballToHit;

private:
	std::array< Scene::Mesh*, 6 > pockets = {};
};


void Table::init()
{
	for ( int i = 0; i < 6; i++ )
	{
		assert( !pockets[ i ] );
		pockets[ i ] = Scene::createPocketMesh( Params::Table::pocketRadius );
		Scene::placeMesh( pockets[ i ], Params::Table::pocketsPositions[ i ].x, Params::Table::pocketsPositions[ i ].y, 0.f );
	}

	for ( int i = 0; i < 7; i++ )
	{
        Scene::Mesh* ballMesh = Scene::createBallMesh( Params::Ball::radius );
        Scene::placeMesh(ballMesh, Params::Table::ballsPositions[ i ].x, Params::Table::ballsPositions[ i ].y, 0.f );

        BillBall* newBall = new BillBall( Vector2(Params::Table::ballsPositions[ i ].x  , Params::Table::ballsPositions[ i ].y), ballMesh);

		billBalls[ i ] = newBall;

	}

	ballToHit = billBalls[ 0 ];
}


void Table::deinit()
{
	for ( Scene::Mesh* mesh : pockets )
		Scene::destroyMesh( mesh );
	pockets = {};
}

//-------------------------------------------------------
//	physical calculations
//-------------------------------------------------------

namespace PhysicEvents
{
    Vector2 vectorProjecction(Vector2 a, Vector2 b)
    {
        float pr = (a.x*b.x+a.y*b.y)/(sqrt(b.x*b.x+b.y*b.y));

        Vector2 t = normalizedVector(b);

        t.x = t.x * pr;
        t.y = t.y * pr;

        return t;
    }


    void ricochet(BillBall*curBall)
    {
        float x = curBall->getPosition().x, y = curBall->getPosition().y;

        if (x + Params::Ball::radius > 0.5f * Params::Table::width)
        {
            float difference = x + Params::Ball::radius - 0.5f * Params::Table::width;
            curBall->setPosition(Vector2(x - difference * 2, y));
            curBall->setSpeed(Vector2( -curBall->getSpeed().x, curBall->getSpeed().y ));
        }

        if (curBall->getPosition().x - Params::Ball::radius < -0.5f * Params::Table::width)
        {
            float difference = - 0.5f * Params::Table::width - x + Params::Ball::radius;
            curBall->setPosition(Vector2(x + difference * 2, y));
            curBall->setSpeed(Vector2( -curBall->getSpeed().x, curBall->getSpeed().y ));
        }

        if (curBall->getPosition().y + Params::Ball::radius > 0.5f * Params::Table::height)
        {
            float difference = y + Params::Ball::radius - 0.5f * Params::Table::height;
            curBall->setPosition(Vector2(x, y - difference * 2));
            curBall->setSpeed(Vector2( curBall->getSpeed().x, -curBall->getSpeed().y ));
        }

        if (curBall->getPosition().y - Params::Ball::radius < -0.5f * Params::Table::height)
        {
            float difference = - 0.5f * Params::Table::height - y + Params::Ball::radius;
            curBall->setPosition(Vector2(x, y + difference * 2));
            curBall->setSpeed(Vector2( curBall->getSpeed().x, -curBall->getSpeed().y ));
        }
    }

    void collide(BillBall* ball1, BillBall* ball2)
    {
        float x1 = ball1->getPosition().x, x2 = ball2->getPosition().x;
        float y1 = ball1->getPosition().y, y2 = ball2->getPosition().y;

        Vector2 guideVector = Vector2( x2 - x1, y2 - y1 );

        Vector2 g1 = vectorProjecction(ball1->getSpeed(), guideVector);
        Vector2 g2 = vectorProjecction(ball2->getSpeed(), guideVector);

        Vector2 newSpeed1 = Vector2(ball1->getSpeed().x - g1.x + g2.x, ball1->getSpeed().y - g1.y + g2.y );
        Vector2 newSpeed2 = Vector2(ball2->getSpeed().x + g1.x - g2.x, ball2->getSpeed().y + g1.y - g2.y );

        ball1->setSpeed(newSpeed1);
        ball2->setSpeed(newSpeed2);
    }
}


//-------------------------------------------------------
//	game public interface
//-------------------------------------------------------

namespace Game
{
	Table table;

	bool isChargingShot = false;
	float shotChargeProgress = 0.f;


	void init()
	{
		Engine::setTargetFPS( Params::System::targetFPS );
		Scene::setupBackground( Params::Table::width, Params::Table::height );
		table.init();
	}


	void deinit()
	{
		table.deinit();
	}

    float distance(Vector2 v1, Vector2 v2)
    {
        return sqrt( (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y) );
    }

    void score(int ballIndex)
    {
        Scene::destroyMesh(table.billBalls[ ballIndex ]->getMesh());
        table.billBalls[ ballIndex ] = NULL;
    }

	void update( float dt )
	{
		if ( isChargingShot )
			shotChargeProgress = std::min( shotChargeProgress + dt / Params::Shot::chargeTime, 1.f );
		Scene::updateProgressBar( shotChargeProgress );

            for (int i = 0; i < table.billBalls.size(); i++)
            {
                BillBall* curBall = table.billBalls[ i ];
                if (curBall != NULL)
                {
                    bool scored = false;
                    Vector2 positionToMove = curBall->getNextPosition(dt);

                    for (int j = 0; j < 6; j++)
                    {
                        Vector2 pocketVector = Vector2(Params::Table::pocketsPositions[ j ].x, Params::Table::pocketsPositions[ j ].y);
                        if ( distance(positionToMove, pocketVector) < Params::Table::pocketRadius )
                        {
                            scored = true;
                            score(i);
                        }
                    }

                    if (!scored)
                    {
                        float vx = curBall->getSpeed().x;
                        float vy = curBall->getSpeed().y;

                        if ( ( vx != 0 ) || (vy != 0 ) )
                            if ( ( vx * vx + vy * vy ) <= Params::Physics::frictionDeceleration * Params::Physics::frictionDeceleration * 1.1f)
                                curBall->setSpeed( Vector2(0,0) );
                            else
                            {
                                Vector2 deceleration = normalizedVector( curBall->getSpeed() );
                                deceleration.x = -deceleration.x * Params::Physics::frictionDeceleration;
                                deceleration.y = -deceleration.y * Params::Physics::frictionDeceleration;

                                curBall->setSpeed(Vector2( vx + deceleration.x, vy + deceleration.y )  );
                            }

                        if ((positionToMove.x + Params::Ball::radius > 0.5f * Params::Table::width) ||
                            (positionToMove.x - Params::Ball::radius < -0.5f * Params::Table::width) ||
                            (positionToMove.y + Params::Ball::radius > 0.5f * Params::Table::height) ||
                            (positionToMove.y - Params::Ball::radius < -0.5f * Params::Table::height))
                            {
                                curBall->setPosition(positionToMove);
                                PhysicEvents::ricochet(curBall);
                                positionToMove = curBall->getPosition();
                            }

                        for (int l = i+1; l < table.billBalls.size(); l++)
                        {
                            if ((l != i) && (table.billBalls[ l ] != NULL))
                                if ( distance(positionToMove, table.billBalls[ l ]->getPosition() ) <= 2 * Params::Ball::radius )
                                {
                                    positionToMove = curBall->getPosition();
                                    PhysicEvents::collide(curBall, table.billBalls[ l ]);
                                }
                        }

                        curBall->setPosition(positionToMove);
                        Scene::placeMesh(table.billBalls[i]->getMesh(), curBall->getPosition().x, curBall->getPosition().y, 0.f);
                    }

                }
            }

	}


	void mouseButtonPressed( float x, float y )
	{
		isChargingShot = true;
	}


	void mouseButtonReleased( float x, float y )
	{

		Vector2 direction(x - table.ballToHit->getPosition().x, y - table.ballToHit->getPosition().y);
        table.ballToHit->strike(direction, shotChargeProgress * Params::Physics::strikePower);

		isChargingShot = false;
		shotChargeProgress = 0.f;
	}
}
