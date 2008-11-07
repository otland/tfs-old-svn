function onSay(cid, words, param)
	local players = getPlayersOnline()
	local IPs = {}
	local multiClients = {}
	local multiClientIPs = {}
	for i, pid in ipairs(players) do
		local ip = getIPByName(getCreatureName(pid))
		local pos = table.find(IPs, ip)
		if(pos ~= nil) then
			if(isInArray(multiClients, players[pos]) == TRUE) then
				table.insert(multiClients, players[pos])
				table.insert(multiClientIPs, ip)
			end
			table.remove(players, pos)
			table.remove(IPs, pos)
			table.insert(multiClients, pid)
			table.insert(multiClientIPs, ip)
		end
		table.insert(IPs, ip)
	end

	if(table.maxn(multiClients) > 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Currently online players with same IP address(es):")
		for i, pid in ipairs(multiClients) do
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, getCreatureName(pid) .. " (" .. convertIntToIP(multiClientIPs[i]) .. ")")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Currently there aren't any players with same IP address(es).")
	end
	return TRUE
end
