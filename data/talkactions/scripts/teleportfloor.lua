function onSay(cid, words, param, channel)
	local n = 1
	if(param ~= "" and tonumber(param)) then
		n = math.max(0, tonumber(param))
	end

	local tmp = getCreaturePosition(cid)
	local pos = getCreaturePosition(cid)
	if(string.sub(words, 2, 2) == "u") then
		pos.z = pos.z - n
	else
		pos.z = pos.z + n
	end

	pos.stackpos = STACKPOS_GROUND
	if(getTileThingByPos(pos).uid == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot teleport there.")
		return TRUE
	end

	pos = getClosestFreeTile(cid, pos, FALSE, FALSE)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return TRUE
	end

	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
