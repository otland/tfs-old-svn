function onAddItem(item, tileItem, position, cid)
	if(isInArray({7711, 7956}, item.itemid)) then --Waterpolo Ball(s)
		return true
	end

	doRemoveItem(item.uid)
	doSendMagicEffect(position, CONST_ME_LOSEENERGY)
	return true
end
