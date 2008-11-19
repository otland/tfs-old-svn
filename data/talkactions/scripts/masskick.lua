function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local t = string.explode(param, ",")
	if(not t[2]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Not enough params.")
		return TRUE
	end

	local multifloor = FALSE
	if(t[3]) then
		clean = getBooleanFromString(t[3])
	end

	local tmp = 0
	local players = getSpectators(getCreaturePosition(cid), t[1], t[2], multifloor)
	for i, pid in ipairs(players) do
		if(pid ~= cid and getPlayerAccess(pid) < getPlayerAccess(cid)) then
			doRemoveCreature(pid)
			tmp = tmp + 1
		end
	end

	if(tmp > 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Kicked " .. kicked .. " players.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Could not kick any player.")
	end

	return TRUE
end
