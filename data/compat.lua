--[[
 * File containing deprecated functions and constants used by alot of scripts and other engines
]]--
CONDITION_PARAM_STAT_MAXHITPOINTS = CONDITION_PARAM_STAT_MAXHEALTH
CONDITION_PARAM_STAT_MAXMANAPOINTS = CONDITION_PARAM_STAT_MAXMANA
CONDITION_PARAM_STAT_SOULPOINTS = CONDITION_PARAM_STAT_SOUL
CONDITION_PARAM_STAT_MAGICPOINTS = CONDITION_PARAM_STAT_MAGICLEVEL
CONDITION_PARAM_STAT_MAXHITPOINTSPERCENT = CONDITION_PARAM_STAT_MAXHEALTHPERCENT
CONDITION_PARAM_STAT_MAXMANAPOINTSPERCENT = CONDITION_PARAM_STAT_MAXMANAPERCENT
CONDITION_PARAM_STAT_SOULPOINTSPERCENT = CONDITION_PARAM_STAT_SOULPERCENT
CONDITION_PARAM_STAT_MAGICPOINTSPERCENT = CONDITION_PARAM_STAT_MAGICLEVELPERCENT

function doSetCreatureDropLoot(cid, doDrop)
	return doCreatureSetDropLoot(cid, doDrop)
end

function doPlayerSay(cid, text, class)
	return doCreatureSay(cid, text, class)
end

function doPlayerAddMana(cid, mana)
	return doCreatureAddMana(cid, mana)
end

function playerLearnInstantSpell(cid, name)
	return doPlayerLearnInstantSpell(cid, name)
end

function doPlayerRemovePremiumDays(cid, days)
	return doPlayerAddPremiumDays(cid, -days)
end

function doPlayerRemOutfit(cid, looktype, addons)
	return doPlayerRemoveOutfit(cid, looktype, addons)
end

function pay(cid, money)
	return doPlayerRemoveMoney(cid, money)
end

function broadcastMessage(msg, msgtype)
	return doBroadcastMessage(msg, msgtype)
end

function getPlayerName(cid)
	return getCreatureName(cid)
end

function getPlayerPosition(cid)
	return getCreaturePosition(cid)
end

function getCreaturePos(cid)
	return getCreaturePosition(cid)
end

function creatureGetPosition(cid)
	return getCreaturePosition(cid)
end

function getPlayerMana(cid)
	return getCreatureMana(cid)
end

function getPlayerMaxMana(cid)
	return getCreatureMaxMana(cid)
end

function getPlayerMasterPos(cid)
	return getTemplePositionById(getPlayerTown(cid))
end

function hasCondition(cid, condition)
	return getCreatureCondition(cid, condition)
end

function isMoveable(uid)
	return isMovable(uid)
end

function isItemMoveable(id)
	return isItemMovable(id)
end

function getDataDir()
	return "./data/"
end

function saveData()
	return saveServer()
end

function savePlayers()
	return saveServer()
end