local conditionDrown = createConditionObject(CONDITION_DROWN)
setConditionParam(conditionDrown, CONDITION_PARAM_PERIODICDAMAGE, -20)
setConditionParam(conditionDrown, CONDITION_PARAM_TICKS, -1)
setConditionParam(conditionDrown, CONDITION_PARAM_TICKINTERVAL, 2000)

local HOTD = 5461
local HOTD_SPEED = 12541

function onStepIn(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end

	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(helmet.itemid == HOTD and getCreatureCondition(cid, CONDITION_OTHER, 1)) then
		doTransformItem(helmet.uid, HOTD_SPEED)
		doDecayItem(helmet.uid)
	end
	
	doAddCondition(cid, conditionDrown)
	return true
end

function onStepOut(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end
	
	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(helmet.itemid == HOTD_SPEED) then
		doTransformItem(helmet.uid, HOTD)
		doDecayItem(helmet.uid)
	end
	
	doRemoveCondition(cid, CONDITION_DROWN)
	return true
end
