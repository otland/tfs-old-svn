local SPECIAL_QUESTS = {2001}

function onStepIn(cid, item, position, lastPosition, fromPosition, toPosition, actor)
	if(not isPlayer(cid) or (isContainer(item.uid) and not isInArray(SPECIAL_QUESTS, item.actionid)
		and item.uid > 65535) or getTileInfo(fromPosition).floorChange[9] or
		getTileInfo(position).creatures <= 1) then
		return true
	end

	if(lastPosition.x == 0) then -- player just logged in
		lastPosition = getTownTemplePosition(getPlayerTown(cid))
		doSendMagicEffect(lastPosition, CONST_ME_TELEPORT)
	end

	doTeleportThing(cid, lastPosition, true)
	return true
end
