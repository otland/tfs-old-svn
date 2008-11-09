function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return FALSE
	end

	local target = getPlayerByNameWildcard(param)
	if(not target) then
		target = getCreatureByName(param)
		if(not target) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Creature not found.")
			return FALSE
		end
	end

	local tmp = getCreaturePosition(target)
	local pos = getClosestFreeTile(target, getCreaturePosition(cid))
	if(doTeleportThing(target, pos, TRUE) ~= LUA_ERROR and isPlayer(target) == TRUE and isPlayerGhost(target) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
