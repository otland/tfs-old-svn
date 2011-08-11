local conditionDrown = createConditionObject(CONDITION_DROWN)
setConditionParam(conditionDrown, CONDITION_PARAM_PERIODICDAMAGE, -20)
setConditionParam(conditionDrown, CONDITION_PARAM_TICKS, -1)
setConditionParam(conditionDrown, CONDITION_PARAM_TICKINTERVAL, 2000)

function onStepIn(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end

	doAddCondition(cid, conditionDrown)
	return true
end

function onStepOut(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end
	
	doRemoveCondition(cid, CONDITION_DROWN)
	return true
end
