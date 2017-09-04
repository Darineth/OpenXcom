#pragma once
/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <vector>
#include "Position.h"
#include "BattlescapeGame.h"

namespace OpenXcom
{

class BattleItem;
class SavedBattleGame;
class Surface;
class Tile;
class Mod;
class TileEngine;
class BattleUnit;

/**
 * 3d double-based vector math class
 */
struct DVec3
{
public:
	double x, y, z;

	/// Null position constructor.
	DVec3() : x(0), y(0), z(0) {};
	/// X Y Z position constructor.
	DVec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {};
	/// Copy constructor.
	DVec3(const DVec3& vec) : x(vec.x), y(vec.y), z(vec.z) {};

	DVec3& operator=(const DVec3& vec) { x = vec.x; y = vec.y; z = vec.z; return *this; }

	DVec3 operator+(const DVec3& vec) const { return DVec3(x + vec.x, y + vec.y, z + vec.z); }
	DVec3& operator+=(const DVec3& vec) { x+=vec.x; y+=vec.y; z+=vec.z; return *this; }

	DVec3 operator-(const DVec3& vec) const { return DVec3(x - vec.x, y - vec.y, z - vec.z); }
	DVec3& operator-=(const DVec3& vec) { x-=vec.x; y-=vec.y; z-=vec.z; return *this; }

	DVec3 operator*(const DVec3& vec) const { return DVec3(x * vec.x, y * vec.y, z * vec.z); }
	DVec3& operator*=(const DVec3& vec) { x*=vec.x; y*=vec.y; z*=vec.z; return *this; }
	DVec3 operator*(const double v) const { return DVec3(x * v, y * v, z * v); }
	DVec3& operator*=(const double v) { x*=v; y*=v; z*=v; return *this; }

	DVec3 operator/(const DVec3& vec) const { return DVec3(x / vec.x, y / vec.y, z / vec.z); }
	DVec3& operator/=(const DVec3& vec) { x/=vec.x; y/=vec.y; z/=vec.z; return *this; }

    DVec3 operator/(const double v) const { return DVec3(x / v, y / v, z / v); }

	double dot(const DVec3& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

	DVec3 cross(const DVec3& vec) const { return DVec3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, x * vec.y - y * vec.x); }

	void normalize() 
	{
		double mag = sqrt(x * x + y * y + z * z);

		x /= mag;
		y /= mag;
		z /= mag;
	}

	double distance(const DVec3& vec)
	{
		return sqrt(vec.x * x + vec.y * y + vec.z * z);
	}

	/// == operator
    bool operator== (const DVec3& vec) const
	{
		return x == vec.x && y == vec.y && z == vec.z;
	}
	/// != operator
    bool operator!= (const DVec3& vec) const
	{
		return x != vec.x || y != vec.y || z != vec.z;
	}
};

/**
 * A class that represents a projectile. Map is the owner of an instance of this class during its short life.
 * It calculates its own trajectory and then moves along this precalculated trajectory in voxel space.
 */
class Projectile
{
private:
	Mod *_mod;
	SavedBattleGame *_save;
	BattleAction _action;
	Position _origin, _targetVoxel;
	std::vector<Position> _trajectory;
	size_t _position;
	Surface *_sprite;
	int _speed;
	int _bulletSprite;
	bool _reversed;
	int _vaporColor, _vaporDensity, _vaporProbability;
	int _impact;
	double _calculatedAccuracy;
	bool _shotgun;
	BattleItem *_ammo;
	Position _originVoxel;
	bool _trajectoryVector;
	Position _trajectoryFinalVector;
	BattleUnit *_trajectorySkipUnit;

	//void applyAccuracy(const Position& origin, Position *target, double accuracy, bool keepRange, Tile *targetTile, bool extendLine);
	//void applyAccuracy(const Position& origin, Position *target, double accuracy, bool keepRange, bool extendLine);
	void applyAccuracy(Position origin, Position *target, double accuracy, bool keepRange, bool extendLine);
public:
	/// Creates a new Projectile.
	//Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position target, BattleItem *ammo);
	Projectile(Mod *mod, SavedBattleGame *save, BattleAction action, Position origin, Position target, BattleItem *ammo, int bulletSprite, bool shotgun);
	/// Cleans up the Projectile.
	~Projectile();
	/// Calculates the trajectory for a straight path.
	int calculateTrajectory(double accuracy, bool force = false, bool ignoreAccuracy = false);
	int calculateTrajectory(double accuracy, const Position& originVoxel, bool excludeUnit = true, bool force = false, bool ignoreAccuracy = false);
	/// Rotates the input vector around a uniformly random axis by the specified angle.
	DVec3 rotateVectorRandomly(const DVec3& vec, const double angle);
	/// Calculates the trajectory for a straight path from a direction vector.
	int calculateTrajectoryFromVector(bool force = false, bool ignoreAccuracy = false, bool dualFiring = false);
	int calculateTrajectoryFromVector(Position originVoxel, bool force = false, bool ignoreAccuracy = false, bool dualFiring = false);
	/// Calculates the chance to hit for a shot.
	void calculateChanceToHit(double& chance, double& coverChance);// Position origin, Position directionVector, double weaponAcc, double soldierAcc, BattleUnit* targetUnit);
	/// Calculates the trajectory for a curved path.
	int calculateThrow(double accuracy, bool ignoreAccuracy = false);
	/// Moves the projectile one step in its trajectory.
	bool move();
	// Reset the projectile to the beginning of its trajectory.
	void resetTrajectory();
	/// Gets the current position in voxel space.
	Position getPosition(int offset = 0) const;
	/// Gets a particle from the particle array.
	int getParticle(int i) const;
	/// Gets the item.
	BattleItem *getItem() const;
	/// Gets the sprite.
	Surface *getSprite() const;
	/// Skips the bullet flight.
	void skipTrajectory();
	/// Gets the Position of origin for the projectile.
	Position getOrigin() const;
	/// Gets the Position of the origin voxel for the projectile.
	Position getOriginVoxel() const;
	/// Gets the targetted tile for the projectile.
	Position getTarget() const;
	/// Returns the last calculated impact for the projectile.
	int getImpact() const;
	/// Is this projectile being drawn back-to-front or front-to-back?
	bool isReversed() const;
	/// adds a cloud of particles at the projectile's location
	void addVaporCloud();
	double getCalculatedAccuracy() const;
	BattleItem *getAmmo() const;
	int recalculateTrajectoryFromVector();
};

}
