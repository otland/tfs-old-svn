function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(fromPosition.x ~= CONTAINER_POSITION) then
		doSendMagicEffect(fromPosition, CONST_ME_EXPLOSIONAREA)
		doRemoveItem(item.uid, 1)
		doCreatureSay(cid, "KABOOOOOOOOOOM!", TALKTYPE_ORANGE_1)
	else
		doSendMagicEffect(fromPosition, CONST_ME_EXPLOSIONAREA)
		doRemoveItem(item.uid, 1)
		doCreatureSay(cid, "KABOOOOOOOOOOM!", TALKTYPE_ORANGE_1)
	end

	return TRUE
end
