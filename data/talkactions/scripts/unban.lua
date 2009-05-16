function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local tmp = getAccountIdByName(param)
	if(tmp == 0) then
		tmp = getAccountIdByAccount(param)
		if(tmp == 0) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player or account '" .. param .. "' does not exists.")
			return true
		end
	end

	if(isAccountBanished(tmp) and doRemoveBanishment(tmp)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, tmp .. " has been unbanned.")
	end

	if(isAccountDeleted(tmp) and doRemoveDeletion(tmp)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, tmp .. " has been undeleted.")
	end

	if(getPlayerViolationAccess(cid) > 2) then
		local ip = getIpByName(param)
		if(isIpBanished(ip) and doRemoveIpBanishment(ip)) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "IP Banishment on " .. doConvertIntegerToIp(ip) .. " has been lifted.")
		end

		if(isPlayerNamelocked(param) and doRemoveNamelock(param)) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Namelock from " .. param .. " has been removed.")
		end
	end
	return true
end
