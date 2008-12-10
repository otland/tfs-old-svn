function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(itemEx.itemid == 2782) then
		doTransformItem(itemEx.uid, 2781)
		doDecayItem(itemEx.uid)
		return TRUE
	elseif(isInArray({7538, 7539}, itemEx.itemid) == TRUE) then
		doTransformItem(itemEx.uid, (itemEx.itemid + 6))
		doDecayItem(itemEx.uid)
		return TRUE
	elseif(itemEx.itemid == 1499) then
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		doRemoveItem(itemEx.uid)
		return TRUE
	end

	return destroyItem(cid, itemEx, toPosition)
end
