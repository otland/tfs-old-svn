function onSay(cid, words, param)
	if(param == "" or not tonumber(param)) then
		doPlayerSendCancel(cid, "Command requires numeric param.")
		return TRUE
	end

	prepareShutdown(tonumber(param))
	return TRUE
end

function prepareShutdown(minutes)
	if(minutes == 0) then
		doSetGameState(GAMESTATE_SHUTDOWN)
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
