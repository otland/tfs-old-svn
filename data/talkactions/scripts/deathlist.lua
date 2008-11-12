function onSay(cid, words, param)
	local target = db.getResult("SELECT `name`, `id` FROM `players` WHERE `name` = " .. db.escapeString(param) .. ";")
	if(target:getID() ~= -1) then
		local targetName = target:getDataString("name")
		local targetGUID = target:getDataInt("id")
		target:free()

		local str = ""
		local deaths = db.getResult("SELECT `time`, `level`, `killed_by`, `altkilled_by` FROM `player_deaths` WHERE `player_id` = " .. targetGUID .. " ORDER BY `time` DESC;")
		if(deaths:getID() ~= -1) then
			local breakline = ""

			while(true) do
				if(str ~= "") then
					breakline = "\n"
				end

				local time = os.date("%d %B %Y %X ", deaths:getDataInt("time"))
				local level = deaths:getDataInt("level")
				local killed = ""
				local lastHitKiller = deaths:getDataString("killed_by")
				local mostDamageKiller = deaths:getDataString("altkilled_by")

				if(tonumber(lastHitKiller)) then
					killed = getPlayerNameByGUID(tonumber(lastHitKiller))
				else
					killed = getArticle(lastHitKiller) .. " " .. string.lower(lastHitKiller)
				end

				if(mostDamageKiller ~= "") then
					if(tonumber(mostDamageKiller)) then
						killed = killed .. " and by " .. getPlayerNameByGUID(tonumber(mostDamageKiller))
					else
						killed = killed .. " and by " .. getArticle(mostDamageKiller) .. " " .. string.lower(mostDamageKiller)
					end
				end

				str = str .. breakline .. " " .. time .. "  Died at Level " .. level .. " by " .. killed .. "."
				if not(deaths:next()) then
					break
				end
			end
			deaths:free()
		else
			str = "No deaths recorded."
		end
		doPlayerPopupFYI(cid, "Deathlist for player: " .. targetName .. ".\n\n" .. str)
	else
		doPlayerSendCancel(cid, "A player with that name does not exist.")
	end
	return TRUE
end
