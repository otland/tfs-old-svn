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

	doAddCondition(cid, conditionDrown)
	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(helmet.itemid == HOTD) then
		if(getCreatureCondition(cid, CONDITION_OTHER, 1) or getItemAttribute(helmet.uid, "duration") < 21600) then
			doTransformItem(helmet.uid, HOTD_SPEED)
			doDecayItem(helmet.uid)
		end
	end
	
	return true
end

function onStepOut(cid, item, position, fromPosition)
	if(not isPlayer(cid)) then
		return false
	end

	doRemoveCondition(cid, CONDITION_DROWN)
	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(helmet.itemid == HOTD_SPEED) then
		doTransformItem(helmet.uid, HOTD)
		doDecayItem(helmet.uid)
	end

	return true
end
