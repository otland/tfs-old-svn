local HOTD = {5461, 12541}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local food = SPECIAL_FOODS[item.itemid]
	if(food == nil) then
		return false
	end
	
	local helmet = getPlayerSlotItem(cid, CONST_SLOT_HEAD)
	if(not isInArray(HOTD, helmet.itemid) or not isUnderWater(cid)) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_SMALL, "You should only eat this dish when wearing a helmet of the deep and walking underwater.")
		return true
	end

	doRemoveItem(helmet.uid)
	helmet = doPlayerAddItem(cid, HOTD[2], false, CONST_SLOT_HEAD)
	doDecayItem(helmet.uid)
	
	doRemoveItem(item.uid, 1)
	doCreatureSay(cid, food, TALKTYPE_MONSTER)
	return true
end
