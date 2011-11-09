function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(item.aid > 0 and itemEx.aid > 0) then
		if(isPlayerPzLocked(cid) and getTileInfo(toPosition).protection) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_ACTIONNOTPERMITTEDINPROTECTIONZONE)
			return true
		end

		local doors = DOORS[itemEx.itemid]
		if(not doors) then
			for k, v in pairs(DOORS) do
				if(v == itemEx.itemid) then
					doors = k
					break
				end
			end
		end

		if(doors) then
			if(item.actionid ~= itemEx.actionid) then
				doPlayerSendCancel(cid, "The key does not match.")
			else
				doTransformItem(itemEx.uid, doors)
			end

			return true
		end
	end

	return false
end