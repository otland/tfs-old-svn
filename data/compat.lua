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

table.getPos = table.find
doSetCreatureDropLoot = doCreatureSetDropLoot
doPlayerSay = doCreatureSay
doPlayerAddMana = doCreatureAddMana
playerLearnInstantSpell = doPlayerLearnInstantSpell
doPlayerRemOutfit = doPlayerRemoveOutfit
pay = doPlayerRemoveMoney
broadcastMessage = doBroadcastMessage
getPlayerName = getCreatureName
getPlayerPosition = getCreaturePosition
getCreaturePos = getCreaturePosition
creatureGetPosition = getCreaturePosition
getPlayerMana = getCreatureMana
getPlayerMaxMana = getCreatureMaxMana
hasCondition = getCreatureCondition
isMoveable = isMovable
isItemMoveable = isItemMovable
saveData = saveServer
savePlayers = saveServer
getPlayerSkill = getPlayerSkillLevel
getPlayerSkullType = getCreatureSkullType
getAccountNumberByName = getAccountIdByName
getIPByName = getIpByName
getPlayersByIP = getPlayersByIp
getThingfromPos = getThingFromPos
getPlayersByAccountNumber = getPlayersByAccountId
getIPByPlayerName = getIpByName
getPlayersByIPNumber = getPlayersByIp
getAccountNumberByPlayerName = getAccountIdByName

function getPlayerVocationName(cid)
	return getVocationInfo(getPlayerVocation(cid)).name
end

function getPromotedVocation(vid)
	return getVocationInfo(vid).promotedVocation
end

function doPlayerRemovePremiumDays(cid, days)
	return doPlayerAddPremiumDays(cid, -days)
end

function getPlayerMasterPos(cid)
	return getTownTemplePosition(getPlayerTown(cid))
end

function getOnlinePlayers()
	local tmp = getPlayersOnline()
	local players = {}
	for i, cid in ipairs(tmp) do
		table.insert(players, getCreatureName(cid))
	end
	return players
end

function getPlayerByName(name)
	local cid = getCreatureByName(name)
	return isPlayer(cid) == TRUE and cid or nil
end
