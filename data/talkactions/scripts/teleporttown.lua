function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return true
	end

	local tid = cid
	local t = string.explode(param, ",")
	if(t[2]) then
		tid = getPlayerByNameWildcard(t[2])
		if(not tid or (isPlayerGhost(tid) and getPlayerGhostAccess(tid) > getPlayerGhostAccess(cid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return true
		end
	end

	local tmp = t[1]
	if(not tonumber(tmp)) then
		tmp = getTownId(tmp)
		if(not tmp) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Town " .. t[1] .. " does not exists.")
			return true
		end
	end

	local pos = getTownTemplePosition(tmp, false)
	if(not pos or isInArray({pos.x, pos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Town " .. t[1] .. " does not exists or has invalid temple position.")
		return true
	end

	pos = getClosestFreeTile(tid, pos)
	if(not pos or isInArray({pos.x, pos.y}, 0)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Destination not reachable.")
		return true
	end

	tmp = getCreaturePosition(tid)
	if(doTeleportThing(tid, pos, true) and not isPlayerGhost(tid)) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return true
end
