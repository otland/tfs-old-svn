local condition = createConditionObject(CONDITION_DROWN)
setConditionParam(condition, CONDITION_PARAM_PERIODICDAMAGE, -20)
setConditionParam(condition, CONDITION_PARAM_TICKS, -1)
setConditionParam(condition, CONDITION_PARAM_TICKINTERVAL, 2000)

local condition_speed = createConditionObject(CONDITION_HASTE)
setConditionParam(condition_speed, CONDITION_PARAM_SPEED, 120)
setConditionParam(condition_speed, CONDITION_PARAM_SUBID, 1)
setConditionParam(condition_speed, CONDITION_PARAM_TICKS, -1)

function onStepIn(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end
	doAddCondition(cid, condition)
	--Coconut Shrimp Bake food condition check
	if(getCreatureCondition(cid, CONDITION_OTHER)) then
		doAddCondition(cid, condition_speed)
	end
	
	return true
end

function onStepOut(cid, item, position, fromPosition)
	doRemoveCondition(cid, CONDITION_DROWN)
	doRemoveCondition(cid, CONDITION_HASTE, 1)
	return true
end
