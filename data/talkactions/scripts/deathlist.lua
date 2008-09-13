dofile("./config.lua")

function onSay(cid, words, param)
	if sqlType == "mysql" then
		env = assert(luasql.mysql())
		con = assert(env:connect(mysqlDatabase, mysqlUser, mysqlPass, mysqlHost, mysqlPort))
	else -- sqlite
		env = assert(luasql.sqlite3())
		con = assert(env:connect(sqliteDatabase))
	end
	local cur = assert(con:execute("SELECT `name`, `id` FROM `players` WHERE `name` = '" .. escapeString(param) .. "';"))
	local row = cur:fetch({}, "a")
	cur:close()
	if row ~= nil then
		local targetName = row.name
		local targetGUID = row.id
		local str = ""
		local breakline = ""
		for time, level, killed_by, is_player in rows(con, "SELECT `time`, `level`, `killed_by`, `is_player` FROM `player_deaths` WHERE `player_id` = " .. targetGUID .. " ORDER BY `time` DESC;") do
			if str ~= "" then
				breakline = "\n"
			end
			local date = os.date("*t", time)

			local article = ""
			if tonumber(is_player) ~= TRUE then
				killed_by = string.lower(killed_by)
				article = getArticle(killed_by) .. " "
			end

			if date.day < 10 then	date.day = "0" .. date.day	end
			if date.hour < 10 then	date.hour = "0" .. date.hour	end
			if date.min < 10 then	date.min = "0" .. date.min	end
			if date.sec < 10 then	date.sec = "0" .. date.sec	end
			str = str .. breakline .. " " .. date.day .. getMonthDayEnding(date.day) .. " " .. getMonthString(date.month) .. " " .. date.year .. " " .. date.hour .. ":" .. date.min .. ":" .. date.sec .. "   Died at Level " .. level .. " by " .. article .. killed_by .. "."
		end
		if str == "" then
			str = "No deaths."
		end
		doPlayerPopupFYI(cid, "Deathlist for player, " .. targetName .. ".\n\n" .. str)
	else
		doPlayerSendCancel(cid, "A player with that name does not exist.")
	end
	con:close()
	env:close()
end