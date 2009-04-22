function onSay(cid, words, param, channel)
	local tid = cid
	if(param ~= "") then
		tid = getPlayerByNameWildcard(param)
		if(tid == 0 or (isPlayerGhost(tid) == TRUE and getPlayerAccess(tid) > getPlayerAccess(cid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
			return TRUE
		end
	end

	local pos = getPlayerTown(tid)
	local tmp = getTownName(pos)
	if(tmp == LUA_ERROR) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Home town does not exists.")
		return TRUE
	end

	pos = getTownTemplePosition(pos)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Wrong temple position for town " .. tmp .. ".")
		return TRUE
	end

	pos = getClosestFreeTile(tid, pos)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return TRUE
	end

	tmp = getCreaturePosition(tid)
	if(doTeleportThing(tid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(tid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
