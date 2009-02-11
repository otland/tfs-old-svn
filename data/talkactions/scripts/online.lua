local config = {
	showGamemasters = getBooleanFromString(getConfigInfo('displayGamemastersWithOnlineCommand'))
}

function onSay(cid, words, param)
	local players = getPlayersOnline()
	local strings = {}

	local i = 1
	local position = 1
	for _, pid in ipairs(players) do
		if(i > (position * 7)) then
			strings[position] = strings[position] .. ","
			position = position + 1
			strings[position] = ""
		else
			strings[position] = i == 1 and "" or strings[position] .. ", "
		end

		if((config.showGamemasters == TRUE or getPlayerCustomFlagValue(cid, PlayerCustomFlag_GamemasterPrivileges) == TRUE or getPlayerCustomFlagValue(pid, PlayerCustomFlag_GamemasterPrivileges) ~= TRUE) and (isPlayerGhost(pid) ~= TRUE or getPlayerAccess(cid) > getPlayerAccess(pid))) then
			strings[position] = strings[position] .. getCreatureName(pid) .. " [" .. getPlayerLevel(pid) .. "]"
			i = i + 1
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, (i - 1) .. " player(s) online:")
	for i, str in ipairs(strings) do
		if(str:sub(str:len()) ~= ",") then
			str = str .. "."
		end

		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	end

	return TRUE
end
