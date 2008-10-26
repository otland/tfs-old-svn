function onSay(cid, words, param)
	local money = getPlayerMoney(cid)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You have " .. money .. " gold.")
	return TRUE
end
