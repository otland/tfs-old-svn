function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires param.")
		return FALSE
	end

	local t = string.explode(param, ",")
	t[1] = tonumber(t[1])
	if(not t[1]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command requires numeric param.")
		return FALSE
	end

	if(t[2]) then
		cid = getPlayerByNameWildcard(t[2])
		if(cid == 0 or isPlayerGhost(cid) == TRUE) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return FALSE
		end
	end

	if(param < 0 or param == 1 or param == 135 or (param > 160 and param < 192) or param > 302) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such outfit does not exist.")
		return FALSE
	end

	local tmp = getCreatureOutfit(cid)
	tmp.lookType = param
	doCreatureChangeOutfit(cid, tmp)
	return TRUE
end
