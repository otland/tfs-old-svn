function onSay(cid, words, param)
	local ret = tonumber(param)
	if(ret == nil) then
		local t = string.explode(param, ",")
		if(t[2] and getBooleanFromString(t[2]) == TRUE) then
			ret = getAccountIdByName(t[1])
		else
			ret = getAccountIdByAccount(t[1])
		end
	end

	if(ret <= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid account data or player name.")
		return TRUE
	end

	local list = getBanList(BANTYPE_NOTATION, ret)
	if(type(list) ~= "table" or table.maxn(list) <= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Such account or player does not have any notation.")
		return TRUE
	end

	ret = "Notations for account " .. ret .. "\n"
	for i, ban in ipairs(list) do
		local tmp = ban.adminId ~= 0 and getPlayerNameByGUID(ban.adminId) or "unknown"
		ret = ret .. "\nAdded at " .. os.date("%c", ban.added) .. " by " .. tmp .. " with reason: " .. getBanReason(ban.reason) .. " and comment: " .. ban.comment .. "."
	end

	doPlayerPopupFYI(cid, ret)
	return TRUE
end
