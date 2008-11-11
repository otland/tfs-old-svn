function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return FALSE
	end

	local creature = getCreatureByName(param)
	local player = getPlayerByNameWildcard(param)
	local pos = string.explode(param, ",")
	local target = {}

	if(player ~= 0) then
		target = getCreaturePosition(tmpPlayer)
	elseif(creature ~= 0) then
		target = getCreaturePosition(tmpCreature)
	elseif(pos[3]) then
		target = {x = pos[1], y = pos[2], z = pos[3]}
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid param specified.")
		return FALSE
	end

	target = getClosestFreeTile(cid, target)
	local tmp = getCreaturePosition(cid)
	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(target, CONST_ME_TELEPORT)
	end

	return TRUE
end
