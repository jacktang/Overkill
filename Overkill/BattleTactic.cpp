#pragma once
#include "BattleTactic.h"



bool BattleTactic::reGroup(BWAPI::Position & regroupPosition) const
{
	typedef std::pair<BWAPI::UnitType, BattleArmy*> mapType;

	bool isGroup = true;
	BOOST_FOREACH(mapType p, tacticArmy)
	{
		if (p.second->getUnits().size() == 0)
			continue;
		if (!p.second->reGroup(regroupPosition))
			isGroup = false;
	}
	return isGroup;
}


bool BattleTactic::hasEnemy()
{
	if (!BWAPI::Broodwar->isVisible(BWAPI::TilePosition(attackPosition)))
		return true;

	// if current attack position has target, do not change original attack position
	BWAPI::Unitset& units = BWAPI::Broodwar->getUnitsInRadius(attackPosition, 8 * 32);
	bool needAttack = false;
	BOOST_FOREACH(BWAPI::Unit u, units)
	{
		if (u->getPlayer() == BWAPI::Broodwar->enemy() && BWTA::getRegion(u->getPosition()) == BWTA::getRegion(attackPosition) && u->getType() != BWAPI::UnitTypes::Protoss_Observer)
		{
			if (u->getType().isFlyer() && tacticArmy[BWAPI::UnitTypes::Zerg_Mutalisk]->getUnits().size() == 0
				&& tacticArmy[BWAPI::UnitTypes::Zerg_Hydralisk]->getUnits().size() == 0)
				continue;
			needAttack = true;
			break;
		}
	}

	// no enemy in attackPosition circle, looking for remaining building in the region
	if (needAttack == false)
	{
		std::map<BWTA::Region*, std::map<BWAPI::Unit, buildingInfo>>& occupiedDetail = InformationManager::Instance().getEnemyOccupiedDetail();
		if (occupiedDetail.find(BWTA::getRegion(attackPosition)) != occupiedDetail.end())
		{
			std::map<BWAPI::Unit, buildingInfo >& tmp = occupiedDetail[BWTA::getRegion(attackPosition)];

			for (std::map<BWAPI::Unit, buildingInfo >::iterator it = tmp.begin(); it != tmp.end(); it++)
			{
				if ((!it->second.unitType.isRefinery() && tmp.size() > 1) ||
					(tmp.size() == 1))
				{
					attackPosition = BWAPI::Position(it->second.initPosition);
					return true;
				}
			}
		}
	}

	return needAttack;
}



bool BattleTactic::unitNearChokepoint(BWAPI::Unit unit) const
{
	BOOST_FOREACH(BWTA::Chokepoint * choke, BWTA::getChokepoints())
	{
		if (unit->getDistance(choke->getCenter()) < 80)
		{
			return true;
		}
	}

	return false;
}

void BattleTactic::newArmyRally()
{
	for (std::map<BWAPI::Unit, std::vector<BWAPI::Position>>::iterator it = newAddArmy.begin(); it != newAddArmy.end();)
	{
		if (state != RETREAT)
		{
			if (it->second.size() == 0)
			{
				tacticArmy[it->first->getType()]->addUnit(it->first);
				newAddArmy.erase(it++);
			}
			else
			{
				it->first->move((*it->second.begin()));
				if (it->first->getDistance((*it->second.begin())) < 32 * 4)
				{
					it->second.erase(it->second.begin());
				}
				it++;
			}
		}
		else
			it++;
	}
}


BattleTactic::BattleTactic()
{
	tacticArmy[BWAPI::UnitTypes::Zerg_Zergling] = new ZerglingArmy();
	tacticArmy[BWAPI::UnitTypes::Zerg_Mutalisk] = new MutaliskArmy();
	tacticArmy[BWAPI::UnitTypes::Zerg_Hydralisk] = new HydraliskArmy();
	tacticArmy[BWAPI::UnitTypes::Zerg_Overlord] = new OverLordArmy();

	endByDefend = false;
}

BattleTactic::~BattleTactic()
{
	typedef std::pair<BWAPI::UnitType, BattleArmy*> mapType;
	BOOST_FOREACH(mapType p, tacticArmy)
	{
		delete p.second;
	}
	//tactic end
}

void BattleTactic::addAllNewArmy()
{
	for (std::map<BWAPI::Unit, std::vector<BWAPI::Position>>::iterator it = newAddArmy.begin(); it != newAddArmy.end();)
	{
		it->first->stop();
		tacticArmy[it->first->getType()]->addUnit(it->first);
		newAddArmy.erase(it++);
	}
}

void BattleTactic::onUnitDestroy(BWAPI::Unit unit)
{
	if (unit == NULL || tacticArmy.find(unit->getType()) == tacticArmy.end())
		return;

	std::vector<UnitState>& army = tacticArmy[unit->getType()]->getUnits();
	for (std::vector<UnitState>::iterator it = army.begin(); it != army.end(); it++)
	{
		if (it->unit == unit)
		{
			army.erase(it);
			break;
		}
	}

	for (std::map<BWAPI::Unit, std::vector<BWAPI::Position>>::iterator it = newAddArmy.begin(); it != newAddArmy.end(); it++)
	{
		if (it->first == unit)
		{
			newAddArmy.erase(it);
			break;
		}
	}
}



