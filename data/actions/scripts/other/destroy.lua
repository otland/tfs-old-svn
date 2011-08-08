function onUse(cid, item, fromPosition, itemEx, toPosition)

	if(isInArray(SPIDER_WEB, itemEx.itemid)) then
		doTransformItem(itemEx.uid, (itemEx.itemid + 6))
		doDecayItem(itemEx.uid)
		return true
	end
	
	if(isInArray(BAMBOO_FENCE, itemEx.itemid)) then
		if(itemEx.itemid == BAMBOO_FENCE[1]) then
			doTransformItem(itemEx.uid, (itemEx.itemid + 161))
		elseif(itemEx.itemid == BAMBOO_FENCE[2]) then
			doTransformItem(itemEx.uid, (itemEx.itemid + 159))
		end
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doDecayItem(itemEx.uid)
		return true
	end
	
	return destroyItem(cid, itemEx, toPosition)
end
