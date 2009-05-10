local increasingItems = {[416] = 417, [426] = 425, [446] = 447, [3216] = 3217, [3202] = 3215}
local decreasingItems = {[417] = 416, [425] = 426, [447] = 446, [3217] = 3217, [3215] = 3202}
local depots = {2589, 2590, 2591, 2592}

function onStepIn(cid, item, position, fromPosition)
	if(not increasingItems[item.itemid]) then
		return TRUE
	end

	if(isPlayer(cid) ~= TRUE or isPlayerGhost(cid) ~= TRUE) then
		doTransformItem(item.uid, increasingItems[item.itemid])
	end

	if(isPlayer(cid) ~= TRUE) then
		return TRUE
	end

	if(item.actionid > 1000 and item.actionid < 3000) then
		if(getPlayerLevel(cid) < item.actionid - 1000) then
			local destPos = getCreaturePosition(cid)
			destPos.z = destPos.z + 1
			doTeleportThing(cid, destPos, FALSE)
			doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
		end
	elseif(getTileInfo(position).protection) then
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
	end

	return TRUE
end

function onStepOut(cid, item, position, fromPosition)
	if(not decreasingItems[item.itemid]) then
		return TRUE
	end

	if(isPlayer(cid) ~= TRUE or isPlayerGhost(cid) ~= TRUE) then
		doTransformItem(item.uid, decreasingItems[item.itemid])
	end
	return TRUE
end
