local increasingItems = {[416] = 417, [426] = 425, [446] = 447, [3216] = 3217, [3202] = 3215}
local decreasingItems = {[417] = 416, [425] = 426, [447] = 446, [3217] = 3217, [3215] = 3202}
local depots = {2589, 2590, 2591, 2592}

function onStepIn(cid, item, position, fromPosition)
	if(increasingItems[item.itemid] ~= nil) then
		if(isPlayer(cid) ~= TRUE or isPlayerGhost(cid) ~= TRUE) then
			doTransformItem(item.uid, increasingItems[item.itemid])
		end

		if(isPlayer(cid) == TRUE) then
			if(item.actionid > 1000 and item.actionid < 3000) then
				if(getPlayerLevel(cid) < item.actionid - 1000) then
					local destPos = getCreaturePosition(cid)
					destPos.z = destPos.z + 1
					doTeleportThing(cid, destPos, FALSE)
					doSendMagicEffect(position, CONST_ME_MAGIC_BLUE)
				end
			elseif(getTileInfo(position).protection) then
				local depotPos = getPlayerLookPos(cid)
				depotPos.stackpos = 2 -- ground = 0, table = 1, depot should be 2
				local depot = getThingFromPos(depotPos)
				if(depot.uid > 0 and isInArray(depots, depot.itemid) == TRUE) then
					local depotItems = getPlayerDepotItems(cid, getDepotId(depot.uid))
					if(depotItems < 2) then
						doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, "Your depot contains 1 item.")
					else
						doPlayerSendTextMessage(cid, MESSAGE_STATUS_DEFAULT, "Your depot contains " .. depotItems .. " items.")
					end
				end
			end
		end
	end
	return TRUE
end

function onStepOut(cid, item, position, fromPosition)
	if(decreasingItems[item.itemid] ~= nil) then
		if(isPlayer(cid) ~= TRUE or isPlayerGhost(cid) ~= TRUE) then
			doTransformItem(item.uid, decreasingItems[item.itemid])
		end
	end
	return TRUE
end
