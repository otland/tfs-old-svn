function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(fromPosition.x ~= CONTAINER_POSITION) then
		doSendMagicEffect(fromPosition, CONST_ME_CRAPS)
	end

	local value = math.random(5792, 5797)
	doTransformItem(item.uid, value)
	doCreatureSay(cid, getCreatureName(cid) .. ' rolled a ' .. value - 5791 .. '.', TALKTYPE_ORANGE_1)
	return true
end
