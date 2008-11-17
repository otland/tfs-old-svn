function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local t = string.explode(param, ";")
	if(not t[2]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "No position specified.")
		return TRUE
	end

	local tmp = string.explode(t[2], ",")
	if(not tmp[3] or tmp[1] == 0 or tmp[2] == 0 or tmp[3] == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Specified position is not valid.")
		return TRUE
	end

	local pid = getPlayerByNameWildcard(t[1])
	if(pid == 0 or (isPlayerGhost(pid) == TRUE and getPlayerAccess(pid) > getPlayerAccess(cid))) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[1] .. " not found.")
		return TRUE
	end

	local pos = {x = tmp[1], y = tmp[2], z = tmp[3], stackpos = STACKPOS_GROUND}
	if(getTileThingByPos(pos).uid == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination tile does not exists.")
		return TRUE
	end

	pos = getClosestFreeTile(pid, pos)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y, pos.z}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return TRUE
	end

	tmp = getCreaturePosition(pid)
	if(doTeleportThing(pid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(pid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
