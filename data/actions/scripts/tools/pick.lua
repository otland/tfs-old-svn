function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(itemEx.uid <= 65535 or itemEx.actionid > 0) and (itemEx.itemid == 354 or itemEx.itemid == 355) then
		doTransformItem(itemEx.uid, 392)
		doDecayItem(itemEx.uid)
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return TRUE
	end

	if(itemEx.itemid == 7200) then
		doTransformItem(itemEx.uid, 7236)
		doSendMagicEffect(toPosition, CONST_ME_BLOCKHIT)
		return TRUE
	end
	return FALSE
end
