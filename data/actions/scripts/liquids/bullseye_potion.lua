local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10 * 60 * 1000) -- 10 minutes
setConditionParam(condition, CONDITION_PARAM_SKILL_DISTANCE, 5)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELD, -10)

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(not isPlayer(itemEx.uid)) then
		return true
	end

	if(doAddCondition(itemEx.uid, condition)) then
		doSendMagicEffect(getCreaturePosition(itemEx.uid), CONST_ME_MAGIC_RED)
		doRemoveItem(item.uid)
	end

	return true
end
