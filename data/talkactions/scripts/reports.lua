local config = {
	expireReportsAfterReads = getConfigValue('expireReportsAfterReads')
}

function onSay(cid, words, param, channel)
	local reportId = tonumber(param)
	if(reportId ~= nil) then
		local report = db.getResult("SELECT `r`.*, `p`.`name` AS `player_name` FROM `server_reports` r LEFT JOIN `players` p ON `r`.`player_id` = `p`.`id` WHERE `r`.`id` = " .. reportId)
		if(report:getID() ~= -1) then
			db.executeQuery("UPDATE `server_reports` SET `reads` = `reads` + 1 WHERE `id` = " .. reportId)
			doPlayerPopupFYI(cid, "Report no. " .. reportId .. "\n\nName: " .. report:getDataString("player_name") .. "\nPosition: [X: " .. report:getDataInt("posx") .. " | Y: " .. report:getDataInt("posy") .. " | Z: " .. report:getDataInt("posz") .. "]\nDate: " .. os.date("%c", report:getDataInt("timestamp")) .. "\nReport:\n" .. report:getDataString("report"))
			report:free()
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Report with no. " .. reportId .. " does not exists.")
		end
	else
		local list = db.getResult("SELECT `r`.`id`, `r`.`player_id`, `p`.`name` AS `player_name` FROM `server_reports` r LEFT JOIN `players` p ON `r`.`player_id` = `p`.`id` WHERE `r`.`reads` < " .. config.expireReportsAfterReads)
		if(list:getID() ~= -1) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "New reports:")
			repeat
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, list:getDataInt("id") .. ", by " .. list:getDataString("player_name") .. ".")
			until not list:next()
			list:free()
		else
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "There are no active reports.")
		end
	end

	return true
end
