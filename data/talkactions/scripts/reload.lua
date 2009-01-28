local reloadInfo = {
	{RELOAD_ACTIONS, "actions", "action"},
	{RELOAD_CONFIG, "config", "configuration"},
	{RELOAD_CREATUREEVENTS, "creatureevents", "creature events", "creaturescripts", "creature scripts"},
	{RELOAD_GAMESERVERS, "gameservers", "game servers", "servers"},
	{RELOAD_GLOBALEVENTS, "globalevents", "global events"},
	{RELOAD_HIGHSCORES, "highscores", "scores"},
	{RELOAD_HOUSEPRICES, "houseprices", "house prices", "prices"},
	{RELOAD_ITEMS, "items", "item"},
	{RELOAD_MONSTERS, "monsters", "monster"},
	{RELOAD_MOVEEVENTS, "moveevents", "move events", "movements"},
	{RELOAD_NPCS, "npcs", "npc"},
	{RELOAD_OUTFITS, "outfits", "outfit"},
	{RELOAD_QUESTS, "quests", "quest"},
	{RELOAD_RAIDS, "raids", "raid"},
	{RELOAD_SPELLS, "spells", "spell"},
	{RELOAD_STAGES, "stages", "experience"},
	{RELOAD_TALKACTIONS, "talkactions", "talk actions", "talk", "commands"},
	{RELOAD_VOCATIONS, "vocations", "vocation"},
	{RELOAD_WEAPONS, "weapons", "weapon"}
}

function onSay(cid, words, param)
	param = param:lower()
	local str = "Failed to reload"
	if(param == "all") then
		local tmp = true
		for i = RELOAD_ACTIONS, RELOAD_WEAPONS do
			if(doReloadInfo(table[1]) ~= TRUE) then
				if(tmp) then
					str = str .. ":"
					tmp = false
				end

				str = str .. " " .. table[2]
			end
		end

		if(tmp) then
			str = "Reloaded everything successfully"
		end
	else
		local tmp = true
		for _, v in ipairs(reloadInfo) do
			if(table.isStrIn(param, v)) then
				if(doReloadInfo(v[1]) == TRUE) then
					str = "Reloaded " .. v[2] .. " successfully"
				else
					str = str .. " " .. v[2]
				end

				tmp = false
				break
			end
		end

		if(tmp) then
			str = "Reload type '" .. param .. "' not found"
		end
	end

	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, str .. ".")
	return TRUE
end
