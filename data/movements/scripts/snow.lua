function onStepOut(cid, item, position, fromPosition)
	if(isPlayerGhost(cid)) then
		return true
	end

	addEvent(transformBack, 10000, position, item.itemid)
	if(item.itemid == 670) then
		doTransformItem(item.uid, 6594)
	else
		doTransformItem(item.uid, item.itemid + 15)
	end
	return true
end

function transformBack(position, oldItemID)
	position.stackpos = 0
	doTransformItem(getThingFromPos(position).uid, oldItemID)
	return true
end
