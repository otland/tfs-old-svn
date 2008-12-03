function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return TRUE
	end

	local t = string.explode(param, ",")
	t[1] = tonumber(t[1])
	if(not t[1]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires numeric param.")
		return TRUE
	end

	if(t[2]) then
		cid = getPlayerByNameWildcard(t[2])
		if(cid == 0 or (isPlayerGhost(pid) == TRUE and getPlayerAccess(pid) > getPlayerAccess(cid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return TRUE
		end
	end

	if(t[1] < 0 or t[1] == 1 or t[1] == 135 or (t[1] > 160 and t[1] < 192) or t[1] > 326) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such outfit does not exist.")
		return TRUE
	end

	local tmp = getCreatureOutfit(cid)
	tmp.lookType = t[1]
	doCreatureChangeOutfit(cid, tmp)
	return TRUE
end
