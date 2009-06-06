function onAddItem(item, tileItem, position, cid)
	if(item.itemid == 7956) then --Waterpolo Ball
		return true
	end

	doRemoveItem(item.uid)
	doSendMagicEffect(pos, CONST_ME_LOSEENERGY)
	return true
end
