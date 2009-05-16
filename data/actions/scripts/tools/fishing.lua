local config = {
	useWorms = true
}

local waterIds = {493, 4608, 4609, 4610, 4611, 4612, 4613, 4614, 4615, 4616, 4617, 4618, 4619, 4620, 4621, 4622, 4623, 4624, 4625}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(not isInArray(waterIds, itemEx.itemid)) then
		return false
	end

	if(itemEx.itemid ~= 493) then
		if(math.random(1, (100 + (getPlayerSkill(cid, SKILL_FISHING) / 10))) <= getPlayerSkill(cid, SKILL_FISHING) and
			(not config.useWorms or (getPlayerItemCount(cid, ITEM_WORM) > 0 and doPlayerRemoveItem(cid, ITEM_WORM, 1))))
		then
				doPlayerAddItem(cid, ITEM_FISH, 1)
		end
		doPlayerAddSkillTry(cid, SKILL_FISHING, 1)
	end

	doSendMagicEffect(toPosition, CONST_ME_LOSEENERGY)
	return true
end
