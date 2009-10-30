function onUse(cid, item, fromPosition, itemEx, toPosition)
	if((itemEx.uid <= 65535 or itemEx.actionid > 0) and isInArray({354, 355}, itemEx.itemid)) then
		doTransformItem(itemEx.uid, 392)
		doDecayItem(itemEx.uid)
		doSendMagicEffect(toPosition, CONST_ME_POFF)
		return true
	end

	if(itemEx.itemid == 7200) then
		doTransformItem(itemEx.uid, 7236)
		doSendMagicEffect(toPosition, CONST_ME_BLOCKHIT)
		return true
	end

	return false
end
