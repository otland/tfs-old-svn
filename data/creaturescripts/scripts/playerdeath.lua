dofile("./config.lua")

function onDeath(cid, corpse, killer)
	doPlayerSendTextMessage(cid, MESSAGE_EVENT_ADVANCE, "You are dead.")
	if deathListEnabled == "yes" then
		if sqlType == "mysql" then
			env = luasql.mysql()
			sql = env:connect(mysqlDatabase, mysqlUser, mysqlPass, mysqlHost, mysqlPort)
		else -- sqlite
			env = luasql.sqlite3()
			sql = env:connect(sqliteDatabase)
		end
		local byPlayer = 0
		if killer == 0 then
			killerName = "field item"
		else
			if isPlayer(killer) == TRUE then
				byPlayer = 1
			end
			killerName = getCreatureName(killer)
		end
		sql:execute("INSERT INTO `player_deaths` (`player_id`, `time`, `level`, `killed_by`, `is_player`) VALUES (" .. getPlayerGUID(cid) .. ", " .. os.time() .. ", " .. getPlayerLevel(cid) .. ", '" .. escapeString(killerName) .. "', " .. byPlayer .. ");")
		local result = sql:execute("SELECT `player_id` FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. ";")
		local deathRecords = numRows(result)
		if sqlType == "mysql" then
			while deathRecords > maxDeathRecords do
				delete = sql:execute("DELETE FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. " ORDER BY `time` LIMIT 1;")
				deathRecords = deathRecords - 1
			end
		else
			while deathRecords > maxDeathRecords do
				delete = sql:execute("DELETE FROM `player_deaths` WHERE `rowid` = (SELECT `rowid` FROM `player_deaths` WHERE `player_id` = " .. getPlayerGUID(cid) .. " ORDER BY `time` LIMIT 1);")
				deathRecords = deathRecords - 1
			end
		end
		sql:close()
		env:close()
	end
end