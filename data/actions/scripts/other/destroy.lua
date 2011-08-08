function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(SPIDER_WEB, itemEx.itemid) then
		doTransformItem(itemEx.uid, (itemEx.itemid + 6))
		doDecayItem(itemEx.uid)
		return true
	end
	
	return destroyItem(cid, itemEx, toPosition)
end
