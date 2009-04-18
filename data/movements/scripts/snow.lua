function onStepOut(cid, item, position, fromPosition)
	if isPlayerGhost(cid) == TRUE then
		return TRUE
	end

	addEvent(transformBack, 10000, {oldItemID = item.itemid, _position = position})
	if item.itemid == 670 then
		doTransformItem(item.uid, 6594)
	else
		doTransformItem(item.uid, item.itemid + 15)
	end
	return TRUE
end

function transformBack(parameters)
	parameters._position.stackpos = 0
	doTransformItem(getThingfromPos(parameters._position).uid, parameters.oldItemID)
	return TRUE
end