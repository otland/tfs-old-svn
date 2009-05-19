local config = {
	deathAssistCount = getConfigValue('deathAssistCount'),
	maxDeathRecords = getConfigValue('maxDeathRecords'),
	limit = ""
}
if(deathAssistCount > -1) then
	limit = " LIMIT 0, " .. deathAssistCount + 1
end

function onSay(cid, words, param, channel)
	local target = db.getResult("SELECT `name`, `id` FROM `players` WHERE `name` = " .. db.escapeString(param) .. ";")
	if(target:getID() == -1) then
		doPlayerSendCancel(cid, "A player with that name does not exist.")
		return true
	end

	local targetName = target:getDataString("name")
	local targetId = target:getDataInt("id")
	target:free()

	local str = ""
	local deaths = db.getResult("SELECT `id`, `date`, `level` FROM `player_deaths` WHERE `player_id` = " .. targetId .." ORDER BY `date` DESC LIMIT 0, " .. config.maxDeathRecords)
	if(deaths:getID() ~= -1) then
		repeat
			local killers = db.getResult("SELECT environment_killers.name AS monster_name, players.name AS player_name FROM killers LEFT JOIN environment_killers ON killers.id = environment_killers.kill_id LEFT JOIN player_killers ON killers.id = player_killers.kill_id LEFT JOIN players ON players.id = player_killers.player_id WHERE killers.death_id = " .. deaths:getDataInt("id") .. " ORDER BY killers.final_hit DESC, killers.id ASC" .. config.limit)
			if(killers:getID() ~= -1) then
				if(str ~= "") then
					str = str .. "\n" .. os.date("%d %B %Y %X ", deaths:getDataLong("date"))
				else
					str = os.date("%d %B %Y %X ", deaths:getDataLong("date"))
				end

				local count = killers:getRows(false)
				local i = 0
				repeat
					i = i + 1
					if(killers:getDataString("player_name") ~= "") then
						if(i == 1) then
							str = str .. "Killed at level " .. deaths:getDataInt("level")
						elseif(i == count) then
							str = str .. " and"
						else
							str = str .. ","
						end

						str = str .. " by "
						if(killers:getDataString("monster_name") ~= "") then
							str = str .. killers:getDataString("monster_name") .. " summoned by "
						end

						str = str .. killers:getDataString("player_name")
					else
						if(i == 1) then
							str = str .. "Died at level " .. deaths:getDataInt("level")
						elseif(i == count) then
							str = str .. " and"
						else
							str = str .. ","
						end

						str = str .. " by " .. killers:getDataString("monster_name")
					end

					if(i == count) then
						str = str .. "."
					end
				until not(killers:next())
				killers:free()
			end
		until not(deaths:next())
		deaths:free()
	else
		str = "No deaths recorded."
	end

	doPlayerPopupFYI(cid, "Deathlist for player: " .. targetName .. ".\n\n" .. str)
	return true
end
