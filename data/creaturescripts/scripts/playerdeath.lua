dofile("./config.lua")

function onDeath(cid, corpse, killer)
	doPlayerSendTextMessage(cid, MESSAGE_EVENT_ADVANCE, "You are dead.")
	if deathListEnabled == "yes" then
		if sqlType == "mysql" then
			env = assert(luasql.mysql())
			con = assert(env:connect(mysqlDatabase, mysqlUser, mysqlPass, mysqlHost, mysqlPort))
		else -- sqlite
			env = assert(luasql.sqlite3())
			con = assert(env:connect(sqliteDatabase))
		end
		local byPlayer = FALSE
		if killer == FALSE then
			killerName = "field item"
		else
			if isPlayer(killer) == TRUE then
				byPlayer = TRUE
			end
			killerName = getCreatureName(killer)
		end
		assert(con:execute("INSERT INTO `player_deaths` (`player_id`, `time`, `level`, `killed_by`, `is_player`) VALUES (" .. getPlayerGUID(cid) .. ", " .. os.time() .. ", " .. getPlayerLevel(cid) .. ", '" .. escapeString(killerName) .. "', " .. byPlayer .. ");"))
		local cursor = assert(con:execute("SELECT `player_id` FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. ";"))
		local deathRecords = numRows(cursor)
		if sqlType == "mysql" then
			while deathRecords > maxDeathRecords do
				delete = assert(con:execute("DELETE FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. " ORDER BY `time` LIMIT 1;"))
				deathRecords = deathRecords - 1
			end
		else
			while deathRecords > maxDeathRecords do
				delete = assert(con:execute("DELETE FROM `player_deaths` WHERE `rowid` = (SELECT `rowid` FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. " ORDER BY `time` LIMIT 1);"))
				deathRecords = deathRecords - 1
			end
		end
		con:close()
		env:close()
	end
end