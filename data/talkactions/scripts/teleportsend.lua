function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return FALSE
	end

	local t = string.explode(param, ";")
	if(not t[2]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "No position specified.")
		return FALSE
	end

	local tmp = string.explode(t[2], ",")
	if(not tmp[3] or tmp[1] == 0 or tmp[2] == 0 or tmp[3] == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid position specified.")
		return FALSE
	end

	local pid = getPlayerByNameWildcard(t[1])
	if(not pid) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player not found.")
		return FALSE
	end

	local pos = getClosestFreeTile(pid, {x = tmp[1], y = tmp[2], z = tmp[3]})
	tmp = getCreaturePosition(pid)
	if(doTeleportThing(pid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(pid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
