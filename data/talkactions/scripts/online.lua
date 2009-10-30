local config = {
	showGamemasters = getBooleanFromString(getConfigValue('displayGamemastersWithOnlineCommand'))
}

function onSay(cid, words, param, channel)
	local players = getPlayersOnline()
	local strings = {""}

	local i, position = 1, 1
	local added = false
	for _, pid in ipairs(players) do
		if(added) then
			if(i > (position * 7)) then
				strings[position] = strings[position] .. ","
				position = position + 1
				strings[position] = ""
			else
				strings[position] = i == 1 and "" or strings[position] .. ", "
			end
		end

		if((config.showGamemasters or getPlayerCustomFlagValue(cid, PLAYERCUSTOMFLAG_GAMEMASTERPRIVILEGES) or not getPlayerCustomFlagValue(pid, PLAYERCUSTOMFLAG_GAMEMASTERPRIVILEGES)) and (not isPlayerGhost(pid) or getPlayerGhostAccess(cid) >= getPlayerGhostAccess(pid))) then
			strings[position] = strings[position] .. getCreatureName(pid) .. " [" .. getPlayerLevel(pid) .. "]"
			i = i + 1
			added = true
		else
			added = false
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, (i - 1) .. " player(s) online:")
	for i, str in ipairs(strings) do
		if(str:sub(str:len()) ~= ",") then
			str = str .. "."
		end

		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str)
	end

	return true
end
