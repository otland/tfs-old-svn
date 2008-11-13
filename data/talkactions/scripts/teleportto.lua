function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return FALSE
	end

	local creature = getCreatureByName(param)
	local player = getPlayerByNameWildcard(param)
	local tile = string.explode(param, ",")
	local pos = {x = 0, y = 0, z = 0}

	if(player ~= 0) then
		pos = getCreaturePosition(player)
	elseif(creature ~= 0) then
		pos = getCreaturePosition(creature)
	elseif(tile[3]) then
		pos = {x = tile[1], y = tile[2], z = tile[3]}
	end

	if(pos == LUA_ERROR or isInArray({pos.x, pos.y, pos.z}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid param specified.")
		return FALSE
	end

	pos = getClosestFreeTile(cid, pos)
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y, pos.z}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cannot perform action.")
		return FALSE
	end

	local tmp = getCreaturePosition(cid)
	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
