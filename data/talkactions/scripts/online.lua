local config = {
	showGamemasters = getBooleanFromString(getConfigInfo('displayGamemastersWithOnlineCommand'))
}

function onSay(cid, words, param)
	local players = getPlayersOnline()
	local strings = {}

	local i = 0
	local position = 1
	for _, pid in ipairs(players) do
		if(i > (position * 7)) then
			strings[position] = strings[position] .. ","
			position = position + 1
			strings[position] = ""
		else
			strings[position] = i == 0 and "" or strings[position] .. ", "
		end

		if((config.showGamemasters == TRUE or getPlayerCustomFlagValue(cid, PlayerCustomFlag_GamemasterPrivileges) == TRUE or getPlayerCustomFlagValue(pid, PlayerCustomFlag_GamemasterPrivileges) ~= TRUE) and (isPlayerGhost(pid) ~= TRUE or getPlayerAccess(cid) > getPlayerAccess(pid))) then
			strings[position] = strings[position] .. getCreatureName(pid) .. " [" .. getPlayerLevel(pid) .. "]"
			i = i + 1
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, i .. " player(s) online:")
	for i, string in ipairs(strings) do
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, string .. ".")
	end

	return TRUE
end
