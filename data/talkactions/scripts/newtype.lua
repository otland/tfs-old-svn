function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local t, pid = string.explode(param, ","), cid
	if(t[2]) then
		pid = getPlayerByNameWildcard(t[2])
		if(not pid or (isPlayerGhost(pid) and getPlayerGhostAccess(pid) > getPlayerGhostAccess(cid))) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. t[2] .. " not found.")
			return true
		end
	end

	local period, tmp = -1, tonumber(t[3])
	if(t[3] and tmp) then
		period = tmp
	end

	if(not isNumeric(t[1])) then
		if(getMonsterInfo(t[1])) then
			doSetMonsterOutfit(pid, t[1], period)
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such outfit does not exist.")
		end

		return true
	end

	t[1] = tonumber(t[1])
	if(t[1] <= 1 or t[1] == 135 or (t[1] > 160 and t[1] < 192) or (t[1] > 351 and t[1] < 366) or t[1] > 367) then
		local item = getItemInfo(t[1])
		if(item) then
			doSetItemOutfit(pid, t[1], period)
			return true
		end

		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such outfit does not exist.")
		return true
	end

	local tmp = getCreatureOutfit(pid)
	tmp.lookType = t[1]

	doCreatureChangeOutfit(pid, tmp)
	return true
end
