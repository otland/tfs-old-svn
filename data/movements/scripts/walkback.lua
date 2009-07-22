function onStepIn(cid, item, position, fromPosition)
	if((item.actionid == 2001 or (item.uid > 0 and item.uid <= 65535)) and isPlayer(cid)) then
		if(fromPosition.x == 0) then -- player just logged in on chest
			doTeleportThing(cid, getTownTemplePosition(getPlayerTown(cid)))
			return true
		end

		doTeleportThing(cid, fromPosition, false)
	end
	return true
end
