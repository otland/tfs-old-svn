function onSay(cid, words, param, channel)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local target = getPlayerByNameWildcard(param)
	if(target == 0) then
		target = getCreatureByName(param)
		if(target == 0) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature not found.")
			return TRUE
		end
	end

	if(isPlayer(target) == TRUE and isPlayerGhost(target) == TRUE and getPlayerAccess(target) > getPlayerAccess(cid)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature not found.")
		return TRUE
	end

	local pos = getClosestFreeTile(target, getCreaturePosition(cid))
	if(pos == LUA_ERROR or isInArray({pos.x, pos.y}, 0) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cannot perform action.")
		return TRUE
	end

	local tmp = getCreaturePosition(target)
	if(doTeleportThing(target, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(target) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
