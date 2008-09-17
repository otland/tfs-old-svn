function onSay(cid, words, param)
	if param ~= nil then
		if isNumber(param) == TRUE then
			prepareShutdown(tonumber(param))
		else
			doPlayerSendCancel(cid, "Command param must be an integer.")
		end
	else
		doPlayerSendCancel(cid, "Command requires param for action.")
	end
end

function prepareShutdown(minutes)
	if minutes == 0 then
		shutdown()
	else
		local message = ""
		if minutes == 1 then
			message = "Server is going down in " .. minutes .. " minute, please log out now!"
		elseif minutes <= 3 then
			message = "Server is going down in " .. minutes .. " minutes, please log out."
		else
			message = "Server is going down in " .. minutes .. " minutes."
		end
		doBroadcastMessage(message)
		addEvent(prepareShutdown, 60000, minutes - 1)
	end
end
