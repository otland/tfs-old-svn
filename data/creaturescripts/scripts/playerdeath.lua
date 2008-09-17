local config = {
	deathListEnabled = getConfigInfo('deathListEnabled'),
	sqlType = getConfigInfo('sqlType'),
	maxDeathRecords = getConfigInfo('maxDeathRecords')
}

function onDeath(cid, corpse, killer)
	doPlayerSendTextMessage(cid, MESSAGE_EVENT_ADVANCE, "You are dead.")
	if(config.deathListEnabled == "yes") then
		if(killer ~= FALSE) then
			if(isPlayer(killer) == TRUE) then
				killerName = getPlayerGUID(killer)
			else
				killerName = getCreatureName(killer)
			end
		else
			killerName = "field item"
		end

		db.executeQuery("INSERT INTO `player_deaths` (`player_id`, `time`, `level`, `killed_by`) VALUES (" .. getPlayerGUID(cid) .. ", " .. os.time() .. ", " .. getPlayerLevel(cid) .. ", " .. db.escapeString(killerName) .. ");")
		local rows = db.getResult("SELECT `player_id` FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. ";")
		if(rows:getID() ~= -1) then
			local deathRecords = rows:numRows(true)
			if(config.sqlType == "sqlite") then
				while(deathRecords > config.maxDeathRecords) do
					db.executeQuery("DELETE FROM `player_deaths` WHERE `rowid` = (SELECT `rowid` FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. " ORDER BY `time` LIMIT 1);")
					deathRecords = deathRecords - 1
				end
			else
				while(deathRecords > config.maxDeathRecords) do
					db.executeQuery("DELETE FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. " ORDER BY `time` LIMIT 1;")
					deathRecords = deathRecords - 1
				end
			end
		end
	end
end