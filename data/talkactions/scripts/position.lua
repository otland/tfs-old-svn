function onSay(cid, words, param, channel)
	local position = getPlayerPosition(cid)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You current position is [X: " .. position.x .. " | Y: " .. position.y .. " | Z: " .. position.z .. "]")
	return TRUE
end
