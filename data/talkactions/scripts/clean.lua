function onSay(cid, words, param)
	if param ~= nil then
		if isNumber(param) == TRUE then
			prepareClean(tonumber(param), cid)
		else
			doPlayerSendCancel(cid, "Command param must be an integer.")
		end
	else
		local count = cleanMap()
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Collected " .. count .. " items.")
	end
end

function prepareClean(minutes, cid)
	if minutes == 0 then
		local count = cleanMap()
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cleaned " .. count .. " items.")
		doBroadcastMessage("Game map cleaned.")
	else
		local message = ""
		if minutes == 1 then
			message = "Game map cleaning in " .. minutes .. " minute, please pick up all your items."
		else
			message = "Game map cleaning in " .. minutes .. " minutes."
		end
		doBroadcastMessage(message)
		addEvent(prepareClean, 60000, minutes - 1, cid)
	end
end