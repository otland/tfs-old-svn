function onSay(cid, words, param, channel)
	local list = {}
	local ips = {}
	local players = getPlayersOnline()
	for i, pid in ipairs(players) do
		local ip = getPlayerIp(pid)
		local tmp = table.find(ips, ip)
		if(tmp ~= nil) then
			if(table.countElements(list, ip) == 0) then
				list[players[tmp]] = ip
			end

			list[pid] = ip
		end

		table.insert(ips, ip)
	end

	if(table.maxn(list) > 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Currently online players with same IP address(es):")
		for pid, ip in pairs(list) do
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, getCreatureName(pid) .. " (" .. doConvertIntegerToIp(ip) .. ")")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Currently there aren't any players with same IP address(es).")
	end

	return TRUE
end
