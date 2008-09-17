local config = {
	rateExp = getConfigInfo('rateExp'),
	rateSkill = getConfigInfo('rateSkill'),
	rateLoot = getConfigInfo('rateLoot'),
	rateMagic = getConfigInfo('rateMagic'),
	rateSpawn = getConfigInfo('rateSpawn'),
	protectionLevel = getConfigInfo('protectionLevel')
}

function onSay(cid, words, param)
	local str = "Server Information:\n\nExperience rate: x" .. config.rateExp .. "\nSkills rate: x" .. config.rateSkill .. "\nLoot rate: x" .. config.rateLoot .. "\nMagic rate: x" .. config.rateMagic .. "\nSpawns rate: x" .. config.rateSpawn .. "\nProtection level: " .. config.protectionLevel
	doPlayerPopupFYI(cid, str)
	return FALSE
end