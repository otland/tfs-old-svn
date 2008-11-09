function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return FALSE
	end

	local tmp = getAccountByName(param)
	if(tmp == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " does not exists.")
		return FALSE
	end

	if(isPlayerNamelocked(param) == TRUE and doRemoveNamelock(param) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Namelock from " .. param .. " has been removed.")
	end

	local ip = getIpByName(param)
	if(isIpBanished(ip) == TRUE and doRemoveIpBanishment(ip) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "IpBan on " .. convertIntToIP(ip) .. " has been lifted.")
	end

	local account = getAccountIdByName(param)
	if(isAccountBanished(account) == TRUE and doRemoveBanishment(account) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, tmp .. " has been unbanned.")
	end

	if(isAccountDeleted(account) == TRUE and doRemoveDeletion(account) == TRUE) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, tmp .. " has been undeleted.")
	end
	return TRUE
end
