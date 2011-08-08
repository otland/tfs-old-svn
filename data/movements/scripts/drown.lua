local condition_drown = createConditionObject(CONDITION_DROWN)
setConditionParam(condition_drown, CONDITION_PARAM_PERIODICDAMAGE, -20)
setConditionParam(condition_drown, CONDITION_PARAM_TICKS, -1)
setConditionParam(condition_drown, CONDITION_PARAM_TICKINTERVAL, 2000)

local condition_speed = createConditionObject(CONDITION_HASTE)
setConditionParam(condition_speed, CONDITION_PARAM_SPEED, 300)
setConditionParam(condition_speed, CONDITION_PARAM_SUBID, 1)
setConditionParam(condition_speed, CONDITION_PARAM_TICKS, -1)

function onStepIn(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end
	doAddCondition(cid, condition_drown)
	
	--Coconut Shrimp Bake food condition check
	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(getCreatureCondition(cid, CONDITION_OTHER) and not getCreatureCondition(cid, CONDITION_HASTE, 1)) then
		--TODO: Change the item id to 12541 rather than add the condition_speed.
		if(helmet.itemid == 5461) then
			doAddCondition(cid, condition_speed)
		end
	end
	
	return true
end

function onStepOut(cid, item, position, fromPosition)
	doRemoveCondition(cid, CONDITION_DROWN)
	doRemoveCondition(cid, CONDITION_HASTE, 1)
	return true
end
