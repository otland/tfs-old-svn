local config = {
	maxLevel = getConfigInfo('maximumDoorLevel')
}

local increasingItems = {[416] = 417, [426] = 425, [446] = 447, [3216] = 3217, [3202] = 3215}
local decreasingItems = {[417] = 416, [425] = 426, [447] = 446, [3217] = 3216, [3215] = 3202}
local depots = {2589, 2590, 2591, 2592}

local checkCreature = {isPlayer, isMonster, isNpc}
function onStepIn(cid, item, position, fromPosition)
	if(not increasingItems[item.itemid]) then
		return FALSE
	end

	if(isPlayer(cid) ~= TRUE or isPlayerGhost(cid) ~= TRUE) then
		doTransformItem(item.uid, increasingItems[item.itemid])
	end

	if(item.actionid >= 194 and item.actionid <= 196) then
		local f = checkCreature[item.actionid - 193]
		if(f(cid) == TRUE) then
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	if(item.actionid >= 191 and item.actionid <= 193) then
		local f = checkCreature[item.actionid - 190]
		if(f(cid) ~= TRUE) then
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	if(isPlayer(cid) ~= TRUE) then
		return TRUE
	end

	if(item.actionid == 189 and isPremium(cid) ~= TRUE) then
		doTeleportThing(cid, fromPosition, FALSE)
		doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
		return TRUE
	end

	local gender = item.actionid - 186
	if(isInArray({PLAYERSEX_FEMALE,  PLAYERSEX_MALE, PLAYERSEX_GAMEMASTER}, gender) == TRUE) then
		local playerGender = getPlayerSex(cid)
		if(playerGender ~= gender) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	local skull = item.actionid - 180
	if(skull >= 0 and skull < 6) then
		local playerSkull = getCreatureSkullType(cid)
		if(playerSkull ~= skull) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	local group = item.actionid - 150
	if(group >= 0 and group < 30) then
		local playerGroup = getPlayerGroupId(cid)
		if(playerGroup < group) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	local vocation = item.actionid - 100
	if(vocation >= 0 and vocation < 50) then
		local playerVocationInfo = getVocationInfo(getPlayerVocation(cid))
		if(playerVocationInfo.id ~= vocation and playerVocationInfo.fromVocation ~= vocation) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	if(item.actionid >= 1000 and item.actionid <= config.maxLevel) then
		if(getPlayerLevel(cid) < item.actionid - 1000) then
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
			doTeleportThing(cid, fromPosition, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end

		return TRUE
	end

	if(item.actionid ~= 0 and getPlayerStorageValue(cid, item.actionid) <= 0) then
		doTeleportThing(cid, fromPosition, FALSE)
		doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)

		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
		return TRUE
	end

	if(getTileInfo(position).protection) then
		local depotPos, depot = getPlayerLookPos(cid), {}
		depotPos.stackpos = STACKPOS_GROUND
		while(true) do
			depotPos.stackpos = depotPos.stackpos + 1
			depot = getThingFromPos(depotPos)
			if(depot.uid == 0) then
				break
			end

			if(isInArray(depots, depot.itemid) == TRUE) then
				local depotItems = getPlayerDepotItems(cid, getDepotId(depot.uid))
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, "Your depot contains " .. depotItems .. " item" .. (depotItems > 1 and "s" or "") .. ".")
				break
			end
		end

		return TRUE
	end

	return FALSE
end

function onStepOut(cid, item, position, fromPosition)
	if(not decreasingItems[item.itemid]) then
		return FALSE
	end

	if(isPlayer(cid) ~= TRUE or isPlayerGhost(cid) ~= TRUE) then
		doTransformItem(item.uid, decreasingItems[item.itemid])
		return TRUE
	end

	return FALSE
end
