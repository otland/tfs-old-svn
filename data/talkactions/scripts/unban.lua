function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	local tmp = getAccountIdByName(param)
	if(tmp == 0) then
		tmp = getAccountIdByAccount(param)
		if(tmp == 0) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player or account '" .. param .. "' does not exists.")
			return TRUE
		end
	end

	if(isAccountBanished(tmp) == TRUE and doRemoveBanishment(tmp) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, tmp .. " has been unbanned.")
	end

	if(isAccountDeleted(tmp) == TRUE and doRemoveDeletion(tmp) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, tmp .. " has been undeleted.")
	end

	if(getPlayerViolationAccess(cid) > 2) then
		local ip = getIpByName(param)
		if(isIpBanished(ip) == TRUE and doRemoveIpBanishment(ip) == TRUE) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "IpBanishment on " .. doConvertIntegerToIp(ip) .. " has been lifted.")
		end

		if(isPlayerNamelocked(param) == TRUE and doRemoveNamelock(param) == TRUE) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Namelock from " .. param .. " has been removed.")
		end
	end
	return TRUE
end
