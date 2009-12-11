local SPECIAL_QUESTS = {2001}

function onStepIn(cid, item, position, lastPosition, fromPosition, toPosition, actor)
	if(not isPlayer(cid)) then
		return true
	end

	if(isContainer(item.uid)) then
		if(not isInArray(SPECIAL_QUESTS, item.actionid) and item.uid > 65535) then
			return true
		end
	elseif(getTileInfo(position).creatures <= 1) then
		return true
	end
	
	if(fromPosition.x == 0) then -- player just logged in
		fromPosition = getTownTemplePosition(getPlayerTown(cid))
		doSendMagicEffect(fromPosition, CONST_ME_TELEPORT)
	end

	doTeleportThing(cid, fromPosition, true)
	return true
end
