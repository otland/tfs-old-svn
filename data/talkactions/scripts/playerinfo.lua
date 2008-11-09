function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return FALSE
	end

	local pid = getPlayerByNameWildcard(param)
	if(not pid) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " is not currently online.")
		return FALSE
	end

	local tmp = getCreaturePosition(pid)
	doPlayerPopupFYI(cid, "Name: " .. getCreatureName(pid) .. "\nGUID: " .. getPlayerGUID(pid) .. "\nAccess: " .. getPlayerAccess(pid) .. "\nLevel: " .. getPlayerLevel(pid) .. "\nMagic Level: " .. getPlayerMagLevel(pid) .. "\nSpeed: " .. getCreatureSpeed(pid) .. "\nPosition: [X: " .. tmp.x .. " | Y: " .. tmp.y .. " | Z: " .. tmp.z .. "]\nNotations: " .. getNotationsCount(getPlayerAccountId(pid)) .. "\nIP: " .. convertIntToIP(getPlayerIp(pid)) .. " (" .. getPlayerIp(pid) .. ")")
	return TRUE
end
