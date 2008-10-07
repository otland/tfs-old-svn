function onSay(cid, words, param)
	local players = getOnlinePlayers()
	local IPs = {}
	local multiClients = {}
	local multiClientIPs = {}
	for i, player in ipairs(players) do
		local ip = getIPByName(player)
		local pos = table.find(IPs, ip)
		if(pos ~= nil) then
			if(not table.isStrIn(players[pos], multiClients)) then
				table.insert(multiClients, players[pos])
				table.insert(multiClientIPs, ip)
			end
			table.remove(players, pos)
			table.remove(IPs, pos)
			table.insert(multiClients, player)
			table.insert(multiClientIPs, ip)
		end
		table.insert(IPs, ip)
	end

	if(table.maxn(multiClients) > 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Currently online players with same IP address(es):")
		for i, multiClient in ipairs(multiClients) do
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, multiClient .. " (" .. convertIntToIP(multiClientIPs[i]) .. ")")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Currently there aren't any players with same IP address(es).")
	end
	return FALSE
end
