local condition = createConditionObject(CONDITION_OTHER)
setConditionParam(condition, CONDITION_PARAM_TICKS, 6 * 60 * 60 * 1000)
setConditionParam(condition, CONDITION_PARAM_SUBID, 1)

local HOTD = {5461,12541}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local food = SPECIAL_FOODS[item.itemid]
	if(food == nil) then
		return false
	end
	
	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(not isInArray(HOTD, helmet.itemid) --[[or TODO: Check to make sure player is underwater]]) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_SMALL, "You should only eat this dish when wearing a helmet of the deep and walking underwater.")
		return true
	end

	--TODO: Condition is not saving on log out?
	if(not doAddCondition(cid, condition)) then
		return true
	end
	
	if(helmet.itemid ~= HOTD[2]) then
		--Replace the current helmet to reset duration.
		doRemoveItem(helmet.uid)
		doPlayerAddItem(cid, HOTD[2])
		helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
		doDecayItem(helmet.uid)
	end
	
	doRemoveItem(item.uid, 1)
	doCreatureSay(cid, food, TALKTYPE_MONSTER)
	return true
end
