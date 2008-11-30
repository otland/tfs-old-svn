function onSay(cid, words, param)
	local t = string.explode(param, ",")
	t[1] = tonumber(t[1])
	if(t[1] == nil) then
		if(t[2] and getBooleanFromString(t[2]) == TRUE) then
			t[1] = getAccountIdByName(param)
		else
			t[1] = getAccountIdByAccount(param)
		end
	end

	if(t[1] <= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid account data or player name.")
		return TRUE
	end

	local list = getBanList(BANTYPE_NOTATION, tmp)
	if(type(list) ~= "table" or table.maxn(list) <= 0) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Account " .. t[1] .. " does not have any notation.")
		return TRUE
	end

	local tmp = "Notations for account: " .. t[1] .. "\n"
	for i, ban in ipairs(list) do
		tmp = "\nAdded at " .. os.date("%c", ban.added) .. " by " .. getPlayerNameByGUID(ban.adminId) .. " with reason: " .. getBanReason(ban.reason) .. " and comment: " .. ban.comment .. "."
	end

	doPlayerPopupFYI(cid, tmp)
	return TRUE
end
