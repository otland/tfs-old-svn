function onSay(cid,words,param)
	local players = getOnlinePlayers()
	local strings = {}
	local curStr = 1
	local nrGMs = 0
	for i, player in ipairs(players) do
		if i > curStr * 7 then
			curStr = curStr + 1
		end
		if strings[curStr] == nil then
			strings[curStr] = ""
			breakline = ""
		elseif strings[curStr] ~= "" then
			breakline = ", "
		end
		if getConfigInfo('displayGamemastersWithOnlineCommand') == "no" then
			if getPlayerCustomFlagValue(getPlayerByName(player), PlayerCustomFlag_GamemasterPrivileges) == FALSE or getPlayerCustomFlagValue(cid, PlayerCustomFlag_GamemasterPrivileges) == TRUE then
				strings[curStr] = strings[curStr] .. breakline .. player .. " [" .. getPlayerLevel(getPlayerByName(player)) .. "]"
			else
				nrGMs = nrGMs + 1
			end
		else
			strings[curStr] = strings[curStr] .. breakline .. player .. " [" .. getPlayerLevel(getPlayerByName(player)) .. "]"
		end
	end
	doPlayerSendTextMessage(cid,MESSAGE_STATUS_CONSOLE_BLUE, #players - nrGMs .. " player(s) online:")
	for i,string in ipairs(strings) do
		if string ~= "" then
			doPlayerSendTextMessage(cid,MESSAGE_STATUS_CONSOLE_BLUE,string .. ".")
		end
	end
end