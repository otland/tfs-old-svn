local JUNGLE_GRASS = {2782, 3985}
local SPIDER_WEB = {7538, 7539}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(JUNGLE_GRASS, itemEx.itemid)) then
		doTransformItem(itemEx.uid, itemEx.itemid - 1)
		doDecayItem(itemEx.uid)
		return true
	end

	if(isInArray(SPIDER_WEB, itemEx.itemid)) then
		doTransformItem(itemEx.uid, (itemEx.itemid + 6))
		doDecayItem(itemEx.uid)
		return true
	end

	if(itemEx.itemid == 1499) then
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doRemoveItem(itemEx.uid)
		return true
	end

	return destroyItem(cid, itemEx, toPosition)
end
