function onStepIn(cid, item, position, fromPosition)
	if((item.actionid == 2001 or (item.uid > 0 and item.uid <= 65535)) and isPlayer(cid)) then
		doTeleportThing(cid, fromPosition, false)
	end
	return true
end
