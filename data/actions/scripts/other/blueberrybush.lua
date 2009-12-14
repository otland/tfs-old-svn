function onUse(cid, item, fromPosition, itemEx, toPosition)
	doCreateItem(2677, 3, fromPosition)
	doTransformItem(item.uid, 2786)

	doDecayItem(item.uid)
	return true
end
