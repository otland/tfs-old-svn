function onSay(cid, words, param)
	if(param ~= "") then
		local tid = cid
		cid = getPlayerByNameWildcard(param)
		if(cid == 0 or (isPlayerGhost(cid) == TRUE and getPlayerAccess(cid) > getPlayerAccess(tid)) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
			return FALSE
		end
	end

	local pos = getTownTemplePosition(getPlayerTown(cid))
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y, pos.z}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Temple town does not exists.")
		return FALSE
	end

	pos = getClosestFreeTile(cid, pos)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y, pos.z}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return FALSE
	end

	local tmp = getCreaturePosition(cid)
	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
