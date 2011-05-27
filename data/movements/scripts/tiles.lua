local config = {
	maxLevel = getConfigInfo('maximumDoorLevel')
}

local increasingItems = {[416] = 417, [426] = 425, [446] = 447, [3216] = 3217, [3202] = 3215, [11062] = 11063}
local decreasingItems = {[417] = 416, [425] = 426, [447] = 446, [3217] = 3216, [3215] = 3202, [11063] = 11062}
local depots = {2589, 2590, 2591, 2592}

local checkCreature = {isPlayer, isMonster, isNpc}
local function pushBack(cid, position, fromPosition, displayMessage)
	doTeleportThing(cid, fromPosition, false)
	doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
	if(displayMessage) then
		doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The tile seems to be protected against unwanted intruders.")
	end
end

function onStepIn(cid, item, position, fromPosition)
	if(not increasingItems[item.itemid]) then
		return false
	end

	if(not isPlayerGhost(cid)) then
		doTransformItem(item.uid, increasingItems[item.itemid])
	end

	if(item.actionid >= 194 and item.actionid <= 196) then
		local f = checkCreature[item.actionid - 193]
		if(f(cid)) then
			pushBack(cid, position, fromPosition, false)
		end

		return true
	end

	if(item.actionid >= 191 and item.actionid <= 193) then
		local f = checkCreature[item.actionid - 190]
		if(not f(cid)) then
			pushBack(cid, position, fromPosition, false)
		end

		return true
	end

	if(not isPlayer(cid)) then
		return true
	end

	if(item.actionid == 189 and not isPremium(cid)) then
		pushBack(cid, position, fromPosition, true)
		return true
	end

	local gender = item.actionid - 186
	if(isInArray({PLAYERSEX_FEMALE,  PLAYERSEX_MALE, PLAYERSEX_GAMEMASTER}, gender)) then
		if(gender ~= getPlayerSex(cid)) then
			pushBack(cid, position, fromPosition, true)
		end

		return true
	end

	local skull = item.actionid - 180
	if(skull >= SKULL_NONE and skull <= SKULL_BLACK) then
		if(skull ~= getCreatureSkullType(cid)) then
			pushBack(cid, position, fromPosition, true)
		end

		return true
	end

	local group = item.actionid - 150
	if(group >= 0 and group < 30) then
		if(group > getPlayerGroupId(cid)) then
			pushBack(cid, position, fromPosition, true)
		end

		return true
	end

	local vocation = item.actionid - 100
	if(vocation >= 0 and vocation < 50) then
		local playerVocationInfo = getVocationInfo(getPlayerVocation(cid))
		if(playerVocationInfo.id ~= vocation and playerVocationInfo.fromVocation ~= vocation) then
			pushBack(cid, position, fromPosition, true)
		end

		return true
	end

	if(item.actionid >= 1000 and item.actionid - 1000 <= config.maxLevel) then
		if(getPlayerLevel(cid) < item.actionid - 1000) then
			pushBack(cid, position, fromPosition, true)
		end

		return true
	end

	if(item.actionid ~= 0 and getPlayerStorageValue(cid, item.actionid) <= 0) then
		pushBack(cid, position, fromPosition, true)
		return true
	end

	if(getTileInfo(position).protection) then
		local depotPos, depot = getCreatureLookPosition(cid), {}
		depotPos.stackpos = STACKPOS_GROUND
		while(true) do
			if(not getTileInfo(depotPos).depot) then
				break
			end

			depotPos.stackpos = depotPos.stackpos + 1
			depot = getThingFromPos(depotPos)
			if(depot.uid == 0) then
				break
			end

			if(isInArray(depots, depot.itemid)) then
				local depotItems = getPlayerDepotItems(cid, getDepotId(depot.uid))
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, "Your depot contains " .. depotItems .. " item" .. (depotItems > 1 and "s" or "") .. ".")
				break
			end
		end

		return true
	end

	return false
end

function onStepOut(cid, item, position, fromPosition)
	if(not decreasingItems[item.itemid]) then
		return false
	end

	if(not isPlayerGhost(cid)) then
		doTransformItem(item.uid, decreasingItems[item.itemid])
		return true
	end

	return false
end
