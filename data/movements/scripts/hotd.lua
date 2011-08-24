local HOTD_DECAYING = 12541

function onEquip(cid, item, slot, boolean)
	--[[ the point of this is to call event ONLY when the duration was started,
	so it means the helmet was 'filled up' with the coconut shrimp bake, testing around ]]--
	if(isUnderWater(cid) and (item.itemid == HOTD_DECAYING or getItemAttribute(item.uid, "duration") ~= nil)) then
		callFunction(cid, item, slot, boolean)
	end

	return true
end