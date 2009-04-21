local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_TICKS, 10 * 60 * 1000) -- 10 minutes
setConditionParam(condition, CONDITION_PARAM_SKILL_MELEE, 5)
setConditionParam(condition, CONDITION_PARAM_SKILL_SHIELD, -10)

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isPlayer(itemEx.uid) ~= TRUE) then
		return TRUE
	end

	if(doAddCondition(itemEx.uid, condition) ~= LUA_ERROR) then
		doSendMagicEffect(getCreaturePosition(itemEx.uid), CONST_ME_MAGIC_RED)
		doRemoveItem(item.uid)
	end

	return TRUE
end
