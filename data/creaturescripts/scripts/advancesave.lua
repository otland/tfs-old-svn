local config = {
	savePlayersOnAdvance = true
}

function onAdvance(cid, skill, oldLevel, newLevel)
	if(config.savePlayersOnAdvance) then
		doPlayerSave(cid, true)
	end

	return true
end
