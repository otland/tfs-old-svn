function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return true
	end

	local t = string.explode(param, ",")
	if(not t[2]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Not enough params.")
		return true
	end

	local multifloor = false
	if(t[3]) then
		multifloor = getBooleanFromString(t[3])
	end

	local tmp = 0
	for i, tid in ipairs(getSpectators(getCreaturePosition(cid), t[1], t[2], multifloor)) do
		if(isPlayer(tid) and tid ~= cid and getPlayerAccess(tid) < getPlayerAccess(cid)) then
			doRemoveCreature(tid)
			tmp = tmp + 1
		end
	end

	if(tmp > 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Kicked " .. tmp .. " players.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Could not kick any player.")
	end

	return true
end
