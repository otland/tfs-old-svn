local config = {
	daily = "no",
	level = 100,
	storage = 30015
}

local playerPosition =
{
	{x = 247, y = 659, z = 13},
	{x = 247, y = 660, z = 13},
	{x = 247, y = 661, z = 13},
	{x = 247, y = 662, z = 13}
}

local newPosition =
{
	{x = 189, y = 650, z = 13},
 	{x = 189, y = 651, z = 13},
	{x = 189, y = 652, z = 13},
	{x = 189, y = 653, z = 13}
}

-- Do not modify the declaration lines below.
local players = {}
local failed = true
config.daily = getBooleanFromString(config.daily)

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.itemid == 1946) then
		if(config.daily) then
			doPlayerSendCancel(cid, "Sorry, not possible.")
		else
			doTransformItem(item.uid, item.itemid - 1)
		end

		return true
	end

	if(item.itemid ~= 1945) then
		return true
	end

	for i, pos in ipairs(playerPosition) do
		pos.stackpos = STACKPOS_TOP_CREATURE
		players[i] = getThingFromPos(playerPosition[i]).uid
		if(players[i] > 0 and
			isPlayer(players[i]) and
			getPlayerStorageValue(players[i].uid, config.storage) == -1 and
			getPlayerLevel(players[i].uid) >= config.level)
		then
			failed = false
		end

		if(failed) then
			doPlayerSendCancel(cid, "Sorry, not possible.")
			return true
		end

		failed = true
	end

	for i, pid in ipairs(players) do
		doSendMagicEffect(playerPosition[i], CONST_ME_POFF)
		doTeleportThing(pid, newPosition[i], false)
		doSendMagicEffect(newPosition[i], CONST_ME_ENERGYAREA)
	end

	doTransformItem(item.uid, item.itemid + 1)
	return true
end
