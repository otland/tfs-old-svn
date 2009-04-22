function onSay(cid, words, param, channel)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local tid = cid
	local t = string.explode(param, ",")
	if(t[2]) then
		tid = getPlayerByNameWildcard(t[2])
		if(tid == 0 or (isPlayerGhost(tid) == TRUE and getPlayerAccess(tid) > getPlayerAccess(cid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return TRUE
		end
	end

	local tmp = t[1]
	if(not tonumber(tmp)) then
		tmp = getTownId(tmp)
		if(tmp == 0) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Town " .. t[1] .. " does not exists.")
			return TRUE
		end
	end

	local pos = getTownTemplePosition(tmp, FALSE)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Town " .. t[1] .. " does not exists or has invalid temple position.")
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
