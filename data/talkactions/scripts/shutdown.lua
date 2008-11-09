function onSay(cid, words, param)
	if(param == "") then
		if(isNumber(param) == TRUE) then
			prepareShutdown(tonumber(param))
		else
			doPlayerSendCancel(cid, "Command requires numeric param.")
		end
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
	end
	return TRUE
end

function prepareShutdown(minutes)
	if(minutes == 0) then
		shutdown()
	else
		if(minutes == 1) then
			doBroadcastMessage("Server is going down in " .. minutes .. " minute, please log out now!")
		elseif(minutes <= 3) then
			doBroadcastMessage("Server is going down in " .. minutes .. " minutes, please log out.")
		else
			doBroadcastMessage("Server is going down in " .. minutes .. " minutes.")
		end
		addEvent(prepareShutdown, 60000, minutes - 1)
	end
end
