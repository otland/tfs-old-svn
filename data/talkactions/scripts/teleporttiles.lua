function onSay(cid, words, param, channel)
	local t = {}
	if(param ~= '') then
		t = string.explode(param, ",")
	end

	local n = tonumber(t[1])
	if(not n) then
		n = 1
	end

	local pid = cid
	if(t[2]) then
		pid = getPlayerByNameWildcard(t[2])
		if(not pid or (isPlayerGhost(pid) and getPlayerAccess(pid) > getPlayerAccess(cid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return true
		end
	end

	local pos = getPosByDir(getCreaturePosition(pid), getPlayerLookDir(pid), n)
	pos.stackpos = STACKPOS_GROUND
	if(getTileThingByPos(pos).uid == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot teleport there.")
		return true
	end

	pos = getClosestFreeTile(pid, pos, false)
	if(not pos or isInArray({pos.x, pos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return true
	end

	local tmp = getCreaturePosition(pid)
	if(doTeleportThing(pid, pos, true) and not isPlayerGhost(pid)) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return true
end
