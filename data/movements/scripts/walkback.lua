local SPECIAL_QUESTS = {2001}
function onStepIn(cid, item, position, lastPosition, fromPosition, toPosition, actor)
	if((isInArray(SPECIAL_QUESTS, item.actionid) or (item.uid > 0 and item.uid <= 65535)) and isPlayer(cid)) then
		if(fromPosition.x == 0) then -- player just logged in on chest
			fromPosition = getTownTemplePosition(getPlayerTown(cid))
			doSendMagicEffect(fromPosition, CONST_ME_TELEPORT)
		end

		doTeleportThing(cid, fromPosition, false)
	end
	return true
end
