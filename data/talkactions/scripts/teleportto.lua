function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return FALSE
	end

	local tmpCreature = getCreatureByName(param)
	local tmpPlayer = getPlayerByNameWildcard(param)
	local tmpPos = string.explode(param, ",")
	local pos = {}

	if(tmpPlayer ~= 0) then
		pos = getCreaturePosition(tmpPlayer)
	elseif(tmpCreature ~= 0) then
		pos = getCreaturePosition(tmpCreature)
	elseif(tmpPos[3]) then
		pos = {x = tmpPos[1], y = tmpPos[2], z = tmpPos[3]}
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid param specified.")
		return FALSE
	end

	pos = getClosestFreeTile(cid, pos)
	local tmp = getCreaturePosition(cid)
	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
