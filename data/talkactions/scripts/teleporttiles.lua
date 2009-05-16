function onSay(cid, words, param, channel)
	local n = 1
	if(param ~= "" and tonumber(param)) then
		n = tonumber(param)
	end

	local pos = getPosByDir(getCreaturePosition(cid), getPlayerLookDir(cid), n)
	pos.stackpos = STACKPOS_GROUND
	if(getTileThingByPos(pos).uid == 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You cannot teleport there.")
		return true
	end

	pos = getClosestFreeTile(cid, pos, false, false)
	if(not pos or isInArray({pos.x, pos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return true
	end

	local tmp = getCreaturePosition(cid)
	if(doTeleportThing(cid, pos, true) and not isPlayerGhost(cid)) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return true
end
