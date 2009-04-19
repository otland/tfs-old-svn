function onSay(cid, words, param)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You have " .. getPlayerMoney(cid) .. " gold.")
	return TRUE
end
