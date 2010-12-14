local config = {
	guildTalksEnabled = getBooleanFromString(getConfigValue('ingameGuildManagement'))
}

function onSay(cid, words, param, channel)
	local playerAccess, t = getPlayerAccess(cid), {}
	for i, talk in ipairs(getTalkActionList()) do
		if(not talk.hide and playerAccess >= talk.access) then
			local tmp = talk.words:sub(1, 1):trim()
			if((guildTalksEnabled or (talk.words ~= "!joinguild" and talk.words ~= "!createguild")) and (tmp == "!" or tmp == "/")) then
				table.insert(t, talk)
			end
		end
	end

	table.sort(t, function(a, b) return a.access > b.access end)
	local lastAccess, str = -1, ""
	for i, talk in ipairs(t) do
		local line = ""
		if(lastAccess ~= talk.access) then
			if(i ~= 1) then
				line = "\n"
			end
			lastAccess = talk.access
		end
		str = str .. line .. talk.words .. "\n"
	end

	doShowTextDialog(cid, 2160, str)
	return true
end
