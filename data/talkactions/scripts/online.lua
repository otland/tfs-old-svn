local config = {
	showGamemasters = getBooleanFromString(getConfigInfo('displayGamemastersWithOnlineCommand'))
}

function onSay(cid, words, param)
	local players = getPlayersOnline()
	local strings = {}
	local position = 1
	local count = 0

	local tmp = false
	for i, pid in ipairs(players) do
		if(strings[position] == nil) then
			strings[position] = ""
		end

		if(tmp) then
			if(i > (position * 7)) then
				strings[position] = strings[position] .. ","
				position = position + 1
			else
				strings[position] = strings[position] .. ", "
			end
		end

		tmp = true
		if((config.showGamemasters == TRUE or getPlayerCustomFlagValue(cid, PlayerCustomFlag_GamemasterPrivileges) == TRUE or getPlayerCustomFlagValue(pid, PlayerCustomFlag_GamemasterPrivileges) ~= TRUE) and (isPlayerGhost(pid) ~= TRUE or getPlayerAccess(cid) > getPlayerAccess(pid))) then
			strings[position] = strings[position] .. getCreatureName(pid) .. " [" .. getPlayerLevel(pid) .. "]"
		else
			count = count + 1
			tmp = false
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, (table.maxn(players) - count) .. " player(s) online:")
	for i, string in ipairs(strings) do
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, string .. ".")
	end

	return TRUE
end
