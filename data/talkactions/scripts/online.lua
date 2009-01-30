local config = {
	showGamemasters = getConfigInfo('displayGamemastersWithOnlineCommand')
}

function onSay(cid, words, param)
	local players = getPlayersOnline()
	local strings = {}
	local pos = 1
	local count = 0
	local tmp = TRUE
	for i, pid in ipairs(players) do
		local line = ", "
		if(tmp == TRUE) then
			if(i > pos * 7) then
				pos = pos + 1
			end

			if(strings[pos] == nil) then
				strings[pos] = ""
				line = ""
			end
		end

		tmp = TRUE
		if((getBooleanFromString(config.showGamemasters) == FALSE and getPlayerCustomFlagValue(pid, PlayerCustomFlag_GamemasterPrivileges) == TRUE and getPlayerCustomFlagValue(cid, PlayerCustomFlag_GamemasterPrivileges) == FALSE) or (isPlayerGhost(pid) == TRUE and getPlayerAccess(pid) > getPlayerAccess(cid))) then
			count = count + 1
			tmp = FALSE
		else
			strings[pos] = strings[pos] .. line .. getCreatureName(pid) .. " [" .. getPlayerLevel(pid) .. "]"
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, (#players - count) .. " player(s) online:")
	for i, string in ipairs(strings) do
		if(string ~= "") then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, string .. ".")
		end
	end
	return TRUE
end
