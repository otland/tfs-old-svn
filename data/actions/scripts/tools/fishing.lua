local config = {
	waterIds = {493, 4608, 4609, 4610, 4611, 4612, 4613, 4614, 4615, 4616, 4617, 4618, 4619, 4620, 4621, 4622, 4623, 4624, 4625},
	rateSkill = getConfigValue("rateSkill"),
	allowFromPz = false,
	useWorms = true
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(not isInArray(config.waterIds, itemEx.itemid)) then
		return false
	end

	if((config.allowFromPz or not getTileInfo(getCreaturePosition(cid)).protection) and itemEx.itemid ~= 493 and
		math.random(1, (100 + (getPlayerSkill(cid, SKILL_FISHING) / 10))) < getPlayerSkill(cid, SKILL_FISHING) and
		(not config.useWorms or (getPlayerItemCount(cid, ITEM_WORM) > 0 and doPlayerRemoveItem(cid, ITEM_WORM, 1)))) then
		doPlayerAddItem(cid, ITEM_FISH, 1)
		doPlayerAddSkillTry(cid, SKILL_FISHING, config.rateSkill)
	end

	doSendMagicEffect(toPosition, CONST_ME_LOSEENERGY)
	return true
end
