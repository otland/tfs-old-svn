local cleanEvent = 0

function onSay(cid, words, param)
	if getPlayerAccess(cid) ~= 0 then
		if(param == '') then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Collected " .. cleanMap() .. " items.")
			return
		end

		if(param == 'tile') then
			local removeLoaded, t = false, string.explode(param, ",")
			if(t[2]) then
				removeLoaded = getBooleanFromString(t[2])
			end

			cleanTile(getCreaturePosition(cid), removeLoaded)
			return
		end

		if(not tonumber(param)) then
			doPlayerSendCancel(cid, "Command requires numeric param.")
			return
		end

		stopEvent(cleanEvent)
		prepareClean(tonumber(param), cid)

	end

end

function prepareClean(minutes, cid)
	if(minutes == 0) then
		if(isPlayer(cid)) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Cleaned " .. cleanMap() .. " items.")
		end

		broadcastMessage("Game map cleaned.", MESSAGE_STATUS_CONSOLE_BLUE)
	elseif(minutes > 0) then
		if(minutes == 1) then
			broadcastMessage("Game map cleaning in " .. minutes .. " minute, please pick up all your items.", MESSAGE_STATUS_CONSOLE_BLUE)
		else
			broadcastMessage("Game map cleaning in " .. minutes .. " minutes.", MESSAGE_STATUS_CONSOLE_BLUE)
		end

		cleanEvent = addEvent(prepareClean, 60000, minutes - 1, cid)
	end
end
