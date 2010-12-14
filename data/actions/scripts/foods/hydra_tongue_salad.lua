local conditions = {
	CONDITION_POISON, CONDITION_FIRE, CONDITION_ENERGY,
	CONDITION_PARALYZE, CONDITION_DRUNK, CONDITION_DROWN,
	CONDITION_FREEZING, CONDITION_DAZZLED, CONDITION_CURSED
}

local exhaust = createConditionObject(CONDITION_EXHAUST)
setConditionParam(exhaust, CONDITION_PARAM_TICKS, (getConfigInfo('timeBetweenExActions') - 100))

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local food = SPECIAL_FOODS[item.itemid]
	if(food == nil) then
		return false
	end

	if(not doAddCondition(cid, exhaust)) then
		return true
	end

	for _, condition in ipairs(conditions) do
		if(getCreatureCondition(cid, condition)) then
			doRemoveCondition(cid, condition)
		end
	end

	doRemoveItem(item.uid, 1)
	doCreatureSay(cid, food, TALKTYPE_MONSTER)
	return true
end
